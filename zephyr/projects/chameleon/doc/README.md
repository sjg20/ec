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

See [PR#28335](https://github.com/zephyrproject-rtos/zephyr/pull/28335) and
[PR#28336](https://github.com/zephyrproject-rtos/zephyr/pull/28336) for
STM32F103ZG support added in Zephyr 2.4.

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

### Jlink

As of Proto 0, you can program via the JTAG interface only if the FPGA board
is not installed.


Connect your JLink to the JTAG, using either an
[Altera FPGA adapter](https://www.segger.com/products/debug-probes/j-link/accessories/adapters/intel-fpga-adapter/)
or a [Xilinx FPGA adapter](https://www.segger.com/products/debug-probes/j-link/accessories/adapters/xilinx-adapter/).

Connect 12V power to the Chameleon.

```
cd ~/zephyrproject
west flash --erase --reset-after-load
```

### Serial Bootloader

To use the serial bootloader to update the STM32's firmware, you must install
`stm32flash` and build `stm32reset`.

```
sudo apt-get install stm32flash
sudo apt-get install gpiod
sudo apt-get install libgpiod-dev
cd ~/chromiumos/src/platform/chameleon/utils/stm32reset
make
```

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

Instructions to build and run remain the same.

When the Zephyr kernel boots, the POWERGOOD LED (red LED near the PCIe
connector, previously the "blinky" LED in the hello, world! app) turns on and
stays lit. The shell prompt (`uart:~$`) appears after the startup messages.

## POWERGOOD LED

`led off` and `led on` control the POWERGOOD LED.

## Micro SD Card

The micro SD card can be connected to the PC through the same USB connection
as the serial console, connected to the FPGA's SD card controller, or
completely disconnected. The `sd` commands takes a sub-command:
* `off` - Disconnect the SD card from the USB port and the FPGA
* `usb` - Connect the SD card to the USB port
* `fpga` - Connect the SD card to the FPGA's SD card controller
* `status` - Show the current setting for the SD mux and the status of the
  CD_DET line.

Start with the uSD slot empty.
```
uart:~$ sd status
The SD mux is set to disconnected
CD_DET = 1
```
Push the card into the slot.
```
uart:~$ sd status
The SD mux is set to disconnected
CD_DET = 0
```
Physically eject the card.

Connect the mux to the USB controller.
```
uart:~$ sd usb
```
Push the card into the slot.
```
uart:~$ sd status
The SD mux is set to USB
CD_DET = 0
```
Your computer will show a new removable drive for the uSD card.

Use the operating system to "safely eject" the removable drive.

Physically eject the card.

Connect the mux to the FPGA.
```
uart:~$ sd fpga
```
Push the card into the slot.
```
uart:~$ sd status
The SD mux is set to FPGA
CD_DET = 0
```
Your computer will not show a new removable drive for the uSD card.
```
uart:~$ sd off
```
Physically eject the card.

## System Monitor

The System Monitor reads several different voltages on the Chameleon and
can provide a human-readable summary of the various voltages and the current
monitors.

The command `sysmon` prints a list of the various voltages being monitored:

```
uart:~$ sysmon
12V: 12.242871
CMON_SOM: 0.294067
CMON_BOARD: 0.235254
PP1200: 1.217358
PP1360: 1.372046
PP1260: 1.280200
PP1000: 1.005469
VMON_9V: 9.174902
VMON_5V: 4.908105
VMON_3V3: 3.232727
PP1800: 1.813550
SOM_VMON: 1.527539
SOM_VMON_1V2: 1.959375
SOM_VMON_MGT: 1.735400
SOM_VMON_1V8A: 1.886865
SOM_VMON_C8: 1.913452
SOM_VREF_CS: 0.001611
```

The SOM_VMON voltages will not be accurate unless the FPGA SOM is installed
and its power supplies are enabled. In this example, the FPGA SOM is not
installed.

Note that on proto0, the current monitors (CMON_SOM and CMON_BOARD) are not
accurate due to a parts issue.

Because of the use of a timer to drive the sampling sequence, you can get
zero values if you run the `sysmon` command soon after start-up:
```
sysmon
12V: 12.091406
CMON_SOM: 0.275537
CMON_BOARD: 0.229614
PP1200: 1.207690
PP1360: 1.374463
PP1260: 1.304370
PP1000: 1.030444
VMON_9V: 9.139453
VMON_5V: 5.017676
VMON_3V3: 3.295569
PP1800: 1.774878
SOM_VMON: 0.000000
SOM_VMON_1V2: 0.000000
SOM_VMON_MGT: 0.000000
SOM_VMON_1V8A: 0.000000
SOM_VMON_C8: 0.000000
SOM_VREF_CS: 0.000000
```
Even a second later, all of the values are non-zero:
```
$ sysmon
12V: 12.117188
CMON_SOM: 0.281177
CMON_BOARD: 0.248145
PP1200: 1.200439
PP1360: 1.374463
PP1260: 1.304370
PP1000: 1.030444
VMON_9V: 9.139453
VMON_5V: 5.017676
VMON_3V3: 3.295569
PP1800: 1.774878
SOM_VMON: 1.290674
SOM_VMON_1V2: 1.688672
SOM_VMON_MGT: 1.513843
SOM_VMON_1V8A: 1.596020
SOM_VMON_C8: 1.546875
SOM_VREF_CS: 0.001611
```

## Clock Generator

The Si5338 clock generator chip provides a 100 MHz clock to the FPGA SOM.
There is no API for this; the initialization is done at startup using a
`SYS_INIT` function.

The Si5338 is fairly complex, and configuration is best done with Silicon
Labs' [Clock Builder Pro software](https://www.silabs.com/products/development-tools/software/clockbuilder-pro-software).
This is an MS Windows-only tool, so I ran it on my personal laptop, saved
the project (it uses a binary format) and exported a C header file with all
of the register settings. Note that the header file is not modified, and
`checkpatch.pl` hates the way it's formatted.

The clock output can be verified with an oscilloscope and either a very steady
hand or some short wires to attach probes. When the board is in reset, the
clock output is idle, but as soon as the `SYS_INIT` function runs, the clock
output changes to a 100 MHz signal.

## FPGA power/boot sequence

The STM32 controls the power enable and power-on-reset for the FPGA, and
monitors the signals for power good and configuration done. With an FPGA
module installed, the FPGA module power good LED (on proto0 this is D21,
just to the right and above the PCIe connector) will briefly turn on and
then off again as the STM32 asserts control over the signal.
[Chameleon FPGA Power/Boot Sequence State Machine](./fpgaboot_sm.md)
provides details of the state machine.

The shell provides the `fpga` command, which has three subcommands:
* `on` to start the power-on and boot sequence for the FPGA module
* `off` to turn off the FPGA module immediately
* `status` to report status
* `bootmode` to set the bootmode to `emmc`, `qspi`, or `sdio`

A new FPGA module will not have a bitstream on it, so a failure to reach
the power-on state is expected behavior.

* Remove power from the Chameleon v3
* Install the FPGA module
* Apply power
* Open the serial console
* Set the boot mode and query the status
```
$ fpga bootmode qspi
FPGA boot mode set to qspi
$ fpga status
FPGA steady status: true
BOOT_MODE = 01
PWR_EN = 0
PWR_GOOD = 0
POR_L_LOAD_L = 0
FPGA_DONE = 0
```
* Start the power-on and boot sequence
```
$ fpga on
FPGA power on ...
```
* The PWR_GOOD LED (leftmost, closest to the PCIe connector) will turn on.
* Set the boot mode. It should fail
```
$ fpga bootmode emmc
FPGA is busy. Try again in a few seconds.
$ fpga status
FPGA steady status: false
BOOT_MODE = 01
PWR_EN = 1
PWR_GOOD = 1
POR_L_LOAD_L = 1
FPGA_DONE = 0
```
* After 10 seconds, the PWR_GOOD LED will turn off; the state machine didn't see
FPGA_DONE assert (because there is no bitstream on the FPGA), so it turned off
power.
* Query the status again
```
$ fpga status
FPGA steady status: true
BOOT_MODE = 01
PWR_EN = 0
PWR_GOOD = 0
POR_L_LOAD_L = 0
FPGA_DONE = 0
$
```

## I/O expanders

The Chameleon v3 has three 16-bit I/O expanders on I2C2. The signals connected
to these I/O expanders are one-time configuration signals, operational mode
and reset signals that are changed very infrequently, or fixed signals for
board identification.

The `misc_io` module exposes an API to read the three BOARD_VERSION signals
as an integer. It also provides debugging commands in the shell to read the
BOARD_VERSION signals, and to read and write two debug signals that are
connected to TP125 and TP126.

After programming the latest firmware, experiment with these commands in the
shell:

```
$ io get ver
ver = 0
$ io get tp125
tp125 = 0
$ io get tp126
tp126 = 0
$ io set tp125 on
$ io get tp125
tp125 = 1
$ io set tp126 on
$ io get tp126
tp126 = 1
$ io set tp125 off
$ io get tp125
tp125 = 0
$ io set tp126 off
$ io get tp126
tp126 = 0
```

If you hook up a logic analyzer to I2C2 SCL and SDA, TP125, and TP126, you can
see the I2C bus transactions between the STM32 and U78 (which has device
address 0x21) to read and write the individual bits, as well as watching TP125
and TP126 change when they are written.
