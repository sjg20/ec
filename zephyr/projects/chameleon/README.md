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

Connect your JLink to the JTAG, using either the Altera or Xilinx adapter.
Connect 12V power to the Chameleon.

```
cd ~/zephyrproject
west flash --erase --reset-after-load
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
