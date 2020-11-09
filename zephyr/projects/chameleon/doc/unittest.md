# Chameleon Unit Tests

The unit tests for Chameleon live in the `tests` directory. These tests are
built to run on the host PC and do not require the use of actual Chameleon v3
hardware.

Build the unit tests inside the `chroot` environment using `cmake` and `ninja`:

```
(cr) $ export ZEPHYR_BASE=/mnt/host/source/src/third_party/zephyr/main/v2.4
(cr) $ cd /mnt/host/source/src/platform/zephyr-chrome/projects/chameleon
(cr) $ cmake -S tests/fpgaboot_sm -B build/fpgaboot_sm \
 -D ZEPHYR_TOOLCHAIN_VARIANT=host -D BOARD=native_posix -G Ninja
(cr) $ ninja -C build/fpgaboot_sm
(cr) $ build/fpgaboot_sm/zephyr/zephyr.exe
```

## fpgaboot_sm

This is the test for the state machine for the FPGA power/boot sequence. It
covers the normal start-up and shut-down flows, as well as timeouts while
waiting for power to stabilize or for the FPGA to complete its configuration.
It adds cases where the start-up sequence is interrupted by a request to
shut down or by a failure.
