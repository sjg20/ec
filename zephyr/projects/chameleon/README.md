# Chameleon Zephyr hello, world!

This document contains information about how to build the hello, world app
for the Chameleon board using the Zephyr RTOS.

## Copied from ...

The hello, world app was based heavily on `experimental/scarlet`, in that I
copied the tree over, renamed some files, and then edited all the files to
change references to the STM32F0 series to the proper chip for Chameleon.

## Chip support

The Chameleon has an STM32F103ZG (1 MB flash, 96 KB RAM), but the ZG doesn't
have a board support file in Zephyr yet. However, the ZE (STM32F103ZE, 512 KB
flash, 64 KB RAM) does, so I just used that chip for this proof of concept.

## Build steps

```
cd ~/zephyrproject
west build -b chameleon -s ~/chromiumos/src/platform/zephyr-chrome/projects/chameleon
```

Output files are in `~/zephyrproject/build/zephyr`
* zephyr.bin
* zephyr.elf
* zephyr.hex

## Load firmware

You can use a
[SEGGER JLink](https://www.segger.com/products/debug-probes/j-link/) probe
to program the firmware, or you can use the serial bootloader.

If you use a Jlink, you will need an
[Altera FPGA adapter](https://www.segger.com/products/debug-probes/j-link/accessories/adapters/intel-fpga-adapter/)
or a [Xilinx FPGA adapter](https://www.segger.com/products/debug-probes/j-link/accessories/adapters/xilinx-adapter/)
to connect to the Chameleon board. As of Proto 0, you can program via the
JTAG interface only if the FPGA board is not installed.

If you want to use the serial bootloader, you will need a a USB-A to USB-C
cable, and you need to install `stm32flash` and build `stm32reset`.

### Install `stm32flash` and build `stm32reset`

If you want to use the serial bootloader to update the STM32's firmware, you
will need to install `stm32flash` and build `stm32reset`.

```
sudo apt-get install stm32flash
sudo apt-get install gpiod
sudo apt-get install libgpiod-dev
cd ~/chromiumos/src/platform/chameleon/utils/stm32reset
make
```

### Jlink

Connect your JLink to the JTAG, using either the Altera or Xilinx adapter.
Connect 12V power to the Chameleon.

```
cd ~/zephyrproject
west flash --erase --reset-after-load
```

### Serial Bootloader

This section assumes that the chromiumos chroot environment is set up in
~/chromiumos, and that zephyr is set up in ~/zephyrproject.

Connect the USB-C cable to the USB-C port closest to the HDMI/DisplayPort
connectors.

Connect 12V power to the Chameleon.

```
sudo ~/chromiumos/src/platform/chameleon/utils/stm32reset/stm32reset --bootloader
stm32flash -o -f /dev/ttyUSB0
stm32flash -w ~/zephyrproject/zephyr/build/zephyr/zephyr.bin -v -S 0x8000000 -f /dev/ttyUSB0
sudo ~/chromiumos/src/platform/chameleon/utils/stm32reset/stm32reset --user
```

## Testing

Open miniterm

```
$ miniterm.py /dev/ttyUSB0 115200
--- Miniterm on /dev/ttyUSB0  115200,8,N,1 ---
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
```

Press the reset button, located between the STM32 and the edge of the PCB.
Observe the following text on the terminal:
```
*** Booting Zephyr OS version 2.3.0  ***
Hello World! arm
```

The red LED at the bottom edge of the board (near the PCIe connector) will
blink at 0.5 Hz.

# Chameleon Application

Modify the Hello World application to turn on the POWERGOOD LED (that was
blinking in the "hello, blinky world") app.

Building and running remains the same.

When the Zephyr kernel boots, the POWERGOOD LED (red LED near the PCIe
connector) will turn on and stay lit.
