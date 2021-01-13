# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Encapsulation of a build configuration."""


import zmake.util as util


class BuildConfig:
    """A container for build configurations.

    A build config is a tuple of environment variables, cmake
    variables, kconfig definitons, and kconfig files.
    """
    def __init__(self, environ_defs={}, cmake_defs={}, kconfig_defs={},
                 kconfig_files=[]):
        self.environ_defs = dict(environ_defs)
        self.cmake_defs = dict(cmake_defs)
        self.kconfig_defs = dict(kconfig_defs)
        self.kconfig_files = kconfig_files

    def popen_cmake(self, jobclient, project_dir, build_dir, kconfig_path=None,
                    **kwargs):
        """Run Cmake with this config using a jobclient.

        Args:
            jobclient: A JobClient instance.
            project_dir: The project directory.
            build_dir: Directory to use for Cmake build.
            kconfig_path: The path to write out Kconfig definitions.
            kwargs: forwarded to popen.
        """
        kconfig_files = list(self.kconfig_files)
        if kconfig_path:
            util.write_kconfig_file(kconfig_path, self.kconfig_defs)
            kconfig_files.append(kconfig_path)
        elif self.kconfig_defs:
            raise ValueError(
                'Cannot start Cmake on a config with Kconfig items without a '
                'kconfig_path')

        if kconfig_files:
            base_config = BuildConfig(environ_defs=self.environ_defs,
                                      cmake_defs=self.cmake_defs)
            conf_file_config = BuildConfig(
                cmake_defs={'CONF_FILE': ';'.join(
                    str(p.resolve()) for p in kconfig_files)})
            return (base_config | conf_file_config).popen_cmake(
                jobclient, project_dir, build_dir, **kwargs)

        kwargs['env'] = dict(**kwargs.get('env', {}), **self.environ_defs)
        return jobclient.popen(
            ['/usr/bin/cmake', '-S', project_dir, '-B', build_dir, '-GNinja',
             *('-D{}={}'.format(*pair) for pair in self.cmake_defs.items())],
            **kwargs)

    def __or__(self, other):
        """Combine two BuildConfig instances."""
        if not isinstance(other, BuildConfig):
            raise TypeError("Unsupported operation | for {} and {}".format(
                type(self), type(other)))

        return BuildConfig(
            environ_defs=dict(**self.environ_defs, **other.environ_defs),
            cmake_defs=dict(**self.cmake_defs, **other.cmake_defs),
            kconfig_defs=dict(**self.kconfig_defs, **other.kconfig_defs),
            kconfig_files=list({*self.kconfig_files, *other.kconfig_files}))

    def __repr__(self):
        return 'BuildConfig({})'.format(', '.join(
            '{}={!r}'.format(name, getattr(self, name))
            for name in ['environ_defs', 'cmake_defs', 'kconfig_defs',
                         'kconfig_files']
            if getattr(self, name)))
