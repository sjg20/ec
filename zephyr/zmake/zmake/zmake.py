# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module encapsulating Zmake wrapper object."""
import logging
import os
import pathlib
import shutil
import subprocess
import tempfile

import zmake.build_config
import zmake.modules
import zmake.jobserver
import zmake.multiproc
import zmake.project
import zmake.toolchains as toolchains
import zmake.util as util


# Output lines from Zephyr that are not normally useful
# Send any lines that start with these strings to INFO
ZEPHYR_SUPRESS = [
    '-- ',    # device tree messages
    'Loaded configuration',
    'Including boilerplate',
    'Parsing ',
    'No change to configuration',
    'No change to Kconfig header',
    ]

def ninja_log_level_override(line, default_log_level):
    """Update the log level for ninja builds if we hit an error.

    Ninja builds print everything to stdout, but really we want to start
    logging things to CRITICAL

    Args:
        line: The line that is about to be logged.
        default_log_level: The default logging level that will be used for the
          line.
    """
    # Herewith a long list of things which are really for debugging, not
    # development. Return logging.DEBUG for each of these.

    # ninja puts progress information on stdout
    if line.startswith("["):
        return logging.DEBUG
    # we don't care about entering directories since it happens every time
    elif line.startswith("ninja: Entering directory"):
        return logging.DEBUG
    # we know the build stops from the compiler messages and ninja return code
    elif line.startswith("ninja: build stopped"):
        return logging.DEBUG
    # someone prints a *** SUCCESS *** message which we don't need
    elif line.startswith("***"):
        return logging.DEBUG
    # this message is a bit like make failing. We already got the error output.
    elif line.startswith("FAILED: CMakeFiles"):
        return logging.INFO
    # if a particular file fails it shows the build line used, but that is not
    # useful except for debugging.
    elif line.startswith("ccache"):
        return logging.DEBUG
    # Try to drop output about the device tree
    elif any([line.startswith(x) for x in ZEPHYR_SUPRESS]):
        return logging.INFO
    # stupid ninja puts errors on stdout, so fix that
    elif line.split()[0] not in ["Memory", "FLASH:", "SRAM:", "IDT_LIST:"]:
        return logging.ERROR
    return default_log_level


def cmake_log_level_override(line, default_log_level):
    """Update the log level for cmake builds if we hit an error.

    Cmake prints some messages that are less than useful during development.

    Args:
        line: The line that is about to be logged.
        default_log_level: The default logging level that will be used for the
          line.
    """
    # Strange output from Zephyr that we normally ignore
    if line.startswith("Including boilerplate"):
        return logging.DEBUG
    return default_log_level


def get_failure_msg(proc):
    """Creates a suitable failure message if something exits badly

    Args:
        proc: subprocess.Popen object containing the thing that failed

    Returns:
        Failure message as a string:
    """
    return "Execution failed (return code={}): {}\n".format(
        proc.returncode, util.repr_command(proc.args))


class Zmake:
    """Wrapper class encapsulating zmake's supported operations.

    The invocations of the constructor and the methods actually comes
    from the main function.  The command line arguments are translated
    such that dashes are replaced with underscores and applied as
    keyword arguments to the constructor and the method, and the
    subcommand invoked becomes the method run.

    As such, you won't find documentation for each method's parameters
    here, as it would be duplicate of the help strings from the
    command line.  Run "zmake --help" for full documentation of each
    parameter.
    """
    def __init__(self, checkout=None, jobserver=None, jobs=0, modules_dir=None,
                 zephyr_base=None):
        self._checkout = checkout
        self._zephyr_base = zephyr_base

        if modules_dir:
            self.module_paths = zmake.modules.locate_from_directory(modules_dir)
        else:
            self.module_paths = zmake.modules.locate_from_checkout(
                self.checkout)

        if jobserver:
            self.jobserver = jobserver
        else:
            try:
                self.jobserver = zmake.jobserver.GNUMakeJobClient.from_environ()
            except OSError:
                self.jobserver = zmake.jobserver.GNUMakeJobServer(jobs=jobs)

        self.logger = logging.getLogger(self.__class__.__name__)

    @property
    def checkout(self):
        if not self._checkout:
            self._checkout = util.locate_cros_checkout()
        return self._checkout.resolve()

    def locate_zephyr_base(self, version):
        """Locate the Zephyr OS repository.

        Args:
            version: If a Zephyr OS base was not supplied to Zmake,
                which version to search for as a tuple of integers.
                This argument is ignored if a Zephyr base was supplied
                to Zmake.
        Returns:
            A pathlib.Path to the found Zephyr OS repository.
        """
        if self._zephyr_base:
            return self._zephyr_base

        return util.locate_zephyr_base(self.checkout, version)

    def configure(self, project_dir, build_dir=None,
                  toolchain=None, ignore_unsupported_zephyr_version=False,
                  build_after_configure=False, test_after_configure=False,
                  bringup=False, coverage=False):
        """Set up a build directory to later be built by "zmake build"."""
        project = zmake.project.Project(project_dir)
        supported_versions = project.config.supported_zephyr_versions

        zephyr_base = self.locate_zephyr_base(max(supported_versions)).resolve()

        # Ignore the patchset from the Zephyr version.
        zephyr_version = util.read_zephyr_version(zephyr_base)[:2]

        if (not ignore_unsupported_zephyr_version
                and zephyr_version not in supported_versions):
            raise ValueError(
                'The Zephyr OS version (v{}.{}) is not supported by the '
                'project.  You may wish to either configure zmake.yaml to '
                'support this version, or pass '
                '--ignore-unsupported-zephyr-version.'.format(*zephyr_version))

        # Resolve build_dir if needed.
        build_dir = util.resolve_build_dir(
            platform_ec_dir=self.module_paths['ec'],
            project_dir=project_dir,
            build_dir=build_dir)
        # Make sure the build directory is clean.
        if os.path.exists(build_dir):
            self.logger.info("Clearing old build directory %s", build_dir)
            shutil.rmtree(build_dir)

        base_config = zmake.build_config.BuildConfig(
            environ_defs={'ZEPHYR_BASE': str(zephyr_base),
                          'PATH': '/usr/bin'},
            cmake_defs={
                'DTS_ROOT': str(self.module_paths['ec'] / 'zephyr'),
                'SYSCALL_INCLUDE_DIRS': str(
                    self.module_paths['ec'] / 'zephyr' / 'include' / 'drivers'),
            })

        # Prune the module paths to just those required by the project.
        module_paths = project.prune_modules(self.module_paths)

        module_config = zmake.modules.setup_module_symlinks(
            build_dir / 'modules', module_paths)

        dts_overlay_config = project.find_dts_overlays(module_paths)

        if not toolchain:
            toolchain = project.config.toolchain

        toolchain_config = toolchains.get_toolchain(toolchain, module_paths)

        if bringup:
            base_config |= zmake.build_config.BuildConfig(
                kconfig_defs={'CONFIG_PLATFORM_EC_BRINGUP': 'y'})
        if coverage:
            base_config |= zmake.build_config.BuildConfig(
                kconfig_defs={'CONFIG_COVERAGE': 'y'})

        if not build_dir.exists():
            build_dir = build_dir.mkdir()
        processes = []
        self.logger.info('Building %s in %s.', project_dir, build_dir)
        for build_name, build_config in project.iter_builds():
            self.logger.info('Configuring %s:%s.', project_dir, build_name)
            config = (base_config
                      | toolchain_config
                      | module_config
                      | dts_overlay_config
                      | build_config)
            output_dir = build_dir / 'build-{}'.format(build_name)
            kconfig_file = build_dir / 'kconfig-{}.conf'.format(build_name)
            proc = config.popen_cmake(self.jobserver, project_dir, output_dir,
                                      kconfig_file, stdin=subprocess.DEVNULL,
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE,
                                      encoding='utf-8',
                                      errors='replace')
            zmake.multiproc.log_output(self.logger, logging.DEBUG, proc.stdout)
            zmake.multiproc.log_output(self.logger, logging.ERROR, proc.stderr)
            processes.append(proc)
        for proc in processes:
            if proc.wait():
                raise OSError(get_failure_msg(proc))

        # Create symlink to project
        util.update_symlink(project_dir, build_dir / 'project')

        if test_after_configure:
            return self.test(build_dir=build_dir)
        elif build_after_configure:
            return self.build(build_dir=build_dir)

    def build(self, build_dir, output_files_out=None, fail_on_warnings=False,
              sequential=False):
        """Build a pre-configured build directory."""
        def wait_and_check_success():
            for proc in procs:
                if proc.wait():
                    raise OSError(get_failure_msg(proc))

            if (fail_on_warnings and
                zmake.multiproc.get_output_flag_and_clear(logging.ERROR)):
                self.logger.warning(
                    "zmake: Warnings detected in build: aborting")
                return False
            return True

        project = zmake.project.Project(build_dir / 'project')

        procs = []
        dirs = {}
        for build_name, build_config in project.iter_builds():
            dirs[build_name] = build_dir / 'build-{}'.format(build_name)
            cmd = ['/usr/bin/ninja', '-C', dirs[build_name].as_posix()]
            self.logger.info('Building %s:%s: %s', build_dir, build_name,
                             ' '.join(cmd))
            proc = self.jobserver.popen(
                cmd,
                # Ninja will connect as a job client instead and claim
                # many jobs.
                claim_job=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                encoding='utf-8',
                errors='replace')

            zmake.multiproc.log_output(
                logger=self.logger,
                log_level=logging.INFO,
                file_descriptor=proc.stdout,
                log_level_override_func=ninja_log_level_override)
            zmake.multiproc.log_output(
                self.logger,
                logging.ERROR,
                proc.stderr,
                log_level_override_func=cmake_log_level_override)
            procs.append(proc)

            if sequential:
                if not wait_and_check_success():
                    return 2

        if not wait_and_check_success():
            return 2

        # Run the packer.
        packer_work_dir = build_dir / 'packer'
        output_dir = build_dir / 'output'
        for d in output_dir, packer_work_dir:
            if not d.exists():
                d.mkdir()

        if output_files_out is None:
            output_files_out = []
        for output_file, output_name in project.packer.pack_firmware(
                packer_work_dir, self.jobserver, **dirs):
            shutil.copy2(output_file, output_dir / output_name)
            self.logger.info('Output file \'%r\' created.', output_file)
            output_files_out.append(output_file)

        return 0

    def test(self, build_dir):
        """Test a build directory."""
        procs = []
        output_files = []
        self.build(build_dir, output_files_out=output_files)

        # If the project built but isn't a test, just bail.
        project = zmake.project.Project(build_dir / 'project')
        if not project.config.is_test:
            return 0

        for output_file in output_files:
            self.logger.info('Running tests in %s.', output_file)
            proc = self.jobserver.popen(
                [output_file],
                claim_job=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                encoding='utf-8',
                errors='replace')
            zmake.multiproc.log_output(self.logger, logging.DEBUG, proc.stdout)
            zmake.multiproc.log_output(self.logger, logging.ERROR, proc.stderr)
            procs.append(proc)

        for idx, proc in enumerate(procs):
            if proc.wait():
                raise OSError(get_failure_msg(proc))
        return 0

    def _run_pytest(self, executor, directory):
        """Run pytest on a given directory.

        This is a utility function to help parallelize running pytest on
        multiple directories.

        Args:
            executor: a multiproc.Executor object.
            directory: The directory that we should search for tests in.
        """
        def get_log_level(line, current_log_level):
            matches = [
                ('PASSED', logging.INFO),
                ('FAILED', logging.ERROR),
                ('warnings summary', logging.WARNING),
            ]

            for text, lvl in matches:
                if text in line:
                    return lvl

            return current_log_level

        def run_test(test_file):
            with self.jobserver.get_job():
                proc = self.jobserver.popen(
                    ['pytest', '--verbose', test_file],
                    claim_job=False,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    encoding='utf-8',
                    errors='replace')
                zmake.multiproc.log_output(
                    self.logger, logging.DEBUG,
                    proc.stdout, log_level_override_func=get_log_level)
                rv = proc.wait()
                if rv:
                    self.logger.error((get_failure_msg(proc)))
                return rv

        for test_file in directory.glob('test_*.py'):
            executor.append(func=lambda: run_test(test_file))

    def testall(self, fail_fast=False):
        """Test all the valid test targets"""
        root_dirs = [self.module_paths['ec'] / 'zephyr']
        project_dirs = []
        for root_dir in root_dirs:
            self.logger.info('Finding zmake target under \'%s\'.', root_dir)
            for path in pathlib.Path(root_dir).rglob('zmake.yaml'):
                project_dirs.append(path.parent)

        executor = zmake.multiproc.Executor(fail_fast=fail_fast)
        tmp_dirs = []
        for project_dir in project_dirs:
            is_test = zmake.project.Project(project_dir).config.is_test
            temp_build_dir = tempfile.mkdtemp(
                    suffix='-{}'.format(os.path.basename(project_dir.as_posix())),
                    prefix='zbuild-')
            tmp_dirs.append(temp_build_dir)
            # Configure and run the test.
            executor.append(
                func=lambda: self.configure(
                    project_dir=pathlib.Path(project_dir),
                    build_dir=pathlib.Path(temp_build_dir),
                    build_after_configure=True,
                    test_after_configure=is_test))

        # Run pytest on platform/ec/zephyr/zmake/tests.
        self._run_pytest(
            executor, self.module_paths['ec'] / 'zephyr' / 'zmake' / 'tests')

        rv = executor.wait()
        for tmpdir in tmp_dirs:
            shutil.rmtree(tmpdir)
        return rv
