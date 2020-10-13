# base32 unit test under Zephyr ztest

Enter the chroot and execute the following commands:
```
export ZEPHYR_BASE=/mnt/host/source/src/third_party/zephyr/main/v2.4
cd /mnt/host/source/src/platform/zephyr-chrome
cmake -S /mnt/host/source/src/platform/zephyr-chrome/tests/base32 \
	-B /mnt/host/source/src/platform/zephyr-chrome/tests/build/base32 \
	-D ZEPHYR_MODULES=/mnt/host/source/src/platform/ec \
	-D ZEPHYR_TOOLCHAIN_VARIANT=host -D BOARD=native_posix -G Ninja
ninja -C tests/build/base32
tests/build/base32/zephyr/zephyr.exe
```
