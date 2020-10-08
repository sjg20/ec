#!/usr/bin/python3.6
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import hashlib
import os
import subprocess
import tempfile

# SDK install path
SDK_INSTALL_PATH = '/opt/zephyr-sdk'

# SDK version
SDK_VERSION = '0.11.4'

# SDK installer URL
SDK_INSTALLER_URL = (
    'https://github.com/zephyrproject-rtos/sdk-ng/releases' +
    '/download/v{version}/zephyr-sdk-{version}-setup.run'
).format(version=SDK_VERSION)

# SDK installer expected MD5 checksum
SDK_INSTALLER_MD5 = 'ca6cc42573f6548cf936b2a60df9a125'


def verify_zephyr_sdk():
    """Verify that the Zephyr SDK is installed.

    Returns:
        True if the Zephyr SDK matching the version specified in
        SDK_VERSION is believed to be installed.
    """
    try:
        with open('%s/sdk_version' % SDK_INSTALL_PATH) as sdk_version:
            current_version = sdk_version.read().replace('\n', '')
            return current_version == SDK_VERSION
    except IOError as e:
        return False


def install_zephyr_sdk(installer_file_fd, installer_file_name):
    """Install the Zephyr SDK using the provided installer file.

    Args:
        installer_file_fd: File descriptor for the installer file.
        installer_file_name: File name for the installer file.
    """
    # Download the installer
    print('Downloading installer from: %s' % SDK_INSTALLER_URL)
    subprocess.run(['wget', '-nv', '--show-progress', '-O', installer_file_name,
                    SDK_INSTALLER_URL])
    os.close(installer_file_fd)

    # Validate the installer
    print('Validating installer...', end='')
    with open(installer_file_name, 'rb') as installer_file:
        data = installer_file.read()
        md5_checksum = hashlib.md5(data).hexdigest()

    if not md5_checksum == SDK_INSTALLER_MD5:
        print('\nFailed to verify installer with MD5: %s' % md5_checksum)
        exit(1)

    print('SUCCESS')

    # Run the installer
    os.chmod(installer_file_name, 0o744)
    subprocess.run([installer_file_name, '--', '-y', '-d', SDK_INSTALL_PATH])


def main():
    # Only run this if the Zephyr SDK isn't already installed or if the version
    # doesn't match.
    if verify_zephyr_sdk():
        print('Zephyr SDK already found in %s' % SDK_INSTALL_PATH)
        exit(0)

    # Create the install directory
    os.makedirs(SDK_INSTALL_PATH, exist_ok=True)

    # Create a temporary file to hold the installer
    installer_file_fd, installer_file_name = tempfile.mkstemp(
        suffix='.run', prefix='zephyr-sdk-setup-', text=False)
    try:
        install_zephyr_sdk(installer_file_fd, installer_file_name)
    finally:
        os.remove(installer_file_name)

    # Exit with 1 and print error if verify_zephyr_sdk returns False
    if not verify_zephyr_sdk():
        print("Failed to verify SDK installation")
        exit(1)


if __name__ == "__main__":
    main()
