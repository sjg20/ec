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
import zmake.project
import zmake.toolchains as toolchains
import zmake.util as util


class Zmake:
    """Wrapper class encapsulating zmake's supported operations."""
    def __init__(self, checkout=None, jobserver=None, jobs=0):
        if checkout:
            self.checkout = pathlib.Path(checkout)
        else:
            self.checkout = util.locate_cros_checkout()
        assert self.checkout.exists()

        if jobserver:
            self.jobserver = jobserver
        else:
            try:
                self.jobserver = zmake.jobserver.GNUMakeJobClient.from_environ()
            except OSError:
                self.jobserver = zmake.jobserver.GNUMakeJobServer(jobs=jobs)

        self.logger = logging.getLogger(self.__class__.__name__)

    def configure(self, project_dir, build_dir,
                  version=None, zephyr_base=None, module_paths=None,
                  toolchain=None, ignore_unsupported_zephyr_version=False,
                  build_after_configure=False, test_after_configure=False):
        """Set up a build directory to later be built by "zmake build"."""
        project = zmake.project.Project(project_dir)
        if version:
            # Ignore the patchset.
            version = version[:2]
            if (not ignore_unsupported_zephyr_version
                    and version not in project.config.supported_zephyr_versions):
                raise ValueError(
                    'Requested version (v{}.{}) is not supported by the '
                    'project.  You may wish to either configure zmake.yaml to '
                    'support this version, or pass '
                    '--ignore-unsupported-zephyr-version.'.format(*version))
        else:
            # Assume the highest supported version by default.
            version = max(project.config.supported_zephyr_versions)
        if not zephyr_base:
            zephyr_base = util.locate_zephyr_base(self.checkout, version)
        zephyr_base = zephyr_base.resolve()

        if not module_paths:
            module_paths = zmake.modules.locate_modules(self.checkout, version)

        if not module_paths['zephyr-chrome']:
            raise OSError("Missing zephyr-chrome module")

        base_config = zmake.build_config.BuildConfig(
            environ_defs={'ZEPHYR_BASE': str(zephyr_base),
                          'PATH': '/usr/bin'},
            cmake_defs={'DTS_ROOT': module_paths['zephyr-chrome']})
        module_config = zmake.modules.setup_module_symlinks(
            build_dir / 'modules', module_paths)

        if not toolchain:
            toolchain = project.config.toolchain
        toolchain_config = toolchains.get_toolchain(toolchain, module_paths)
        if not build_dir.exists():
            build_dir = build_dir.mkdir()
        processes = []
        self.logger.info('Building %s in %s.', project_dir, build_dir)
        for build_name, build_config in project.iter_builds():
            self.logger.info('Configuring %s:%s.', project_dir, build_name)
            config = (base_config
                      | toolchain_config
                      | module_config
                      | build_config)
            output_dir = build_dir / 'build-{}'.format(build_name)
            kconfig_file = build_dir / 'kconfig-{}.conf'.format(build_name)
            processes.append(
                config.popen_cmake(self.jobserver, project_dir, output_dir,
                                   kconfig_file, stdin=subprocess.DEVNULL,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, encoding='utf-8',
                                   errors='replace'))
        for proc in processes:
            stdout, _ = proc.communicate()
            if proc.returncode:
                util.log_multi_line(self.logger, logging.ERROR, stdout)
                raise OSError(
                    "Execution of {} failed (return code={})!\n".format(
                        util.repr_command(proc.args), proc.returncode))
            util.log_multi_line(self.logger, logging.DEBUG, stdout)

        # Create symlink to project
        util.update_symlink(project_dir, build_dir / 'project')

        if test_after_configure:
            return self.test(build_dir=build_dir)
        elif build_after_configure:
            return self.build(build_dir=build_dir)

    def build(self, build_dir, output_files_out=None):
        """Build a pre-configured build directory."""
        project = zmake.project.Project(build_dir / 'project')

        procs = []
        dirs = {}
        for build_name, build_config in project.iter_builds():
            self.logger.info('Building %s:%s.', build_dir, build_name)
            dirs[build_name] = build_dir / 'build-{}'.format(build_name)
            procs.append(self.jobserver.popen(
                ['/usr/bin/ninja', '-C', dirs[build_name]],
                # Ninja will connect as a job client instead and claim
                # many jobs.
                claim_job=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                encoding='utf-8',
                errors='replace'))

        for proc in procs:
            stdout, _ = proc.communicate()
            if proc.returncode:
                util.log_multi_line(self.logger, logging.ERROR, stdout)
                raise OSError(
                    "Execution of {} failed (return code={})!\n".format(
                        util.repr_command(proc.args), proc.returncode))
            util.log_multi_line(self.logger, logging.DEBUG, stdout)

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
            procs.append(self.jobserver.popen(
                [output_file],
                claim_job=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                encoding='utf-8',
                errors='replace'))

        for idx, proc in enumerate(procs):
            stdout, _ = proc.communicate()
            if proc.returncode:
                util.log_multi_line(self.logger, logging.ERROR, stdout)
                raise OSError(
                    "Execution of {} failed (return code={})!\n".format(
                        util.repr_command(proc.args), proc.returncode))
            util.log_multi_line(self.logger, logging.DEBUG, stdout)
        return 0

    def testall(self, fail_fast=False):
        """Test all the valid test targets"""
        modules = zmake.modules.locate_modules(self.checkout, version=None)
        root_dirs = [modules['zephyr-chrome'] / 'projects',
                     modules['zephyr-chrome'] / 'tests',
                     modules['ec-shim'] / 'zephyr/test']
        project_dirs = []
        for root_dir in root_dirs:
            self.logger.info('Finding zmake target under \'%s\'.', root_dir)
            for path in pathlib.Path(root_dir).rglob('zmake.yaml'):
                project_dirs.append(path.parent)

        # Find the longest path string + 3 (for '...') + 8 (for padding).
        max_project_dir_len = max(len(str(f)) for f in project_dirs) + 11
        for project_dir in project_dirs:
            is_test = zmake.project.Project(project_dir).config.is_test
            with tempfile.TemporaryDirectory(
                    suffix='-{}'.format(os.path.basename(project_dir)),
                    prefix='zbuild-') as temp_build_dir:
                # Configure and run the test.
                rv = self.configure(project_dir=pathlib.Path(project_dir),
                                    build_dir=pathlib.Path(temp_build_dir),
                                    build_after_configure=True,
                                    test_after_configure=is_test)
                if rv and fail_fast:
                    return rv
        return 0
