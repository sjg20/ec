# Using GPIOs in Zephyr

As part of the effort to move the EC to Zephyr, and to bring up the Chameleon
board with Zephyr, we need a good approach for using GPIOs.

The EC has simple `gpio_get_level` and `gpio_set_level` functions that can
operate on any of the GPIOs. We want something like this for Zephyr-based
code.

Zephyr is a little more complicated. We first need a `struct device*` for the
GPIO port. We can get this by using `device_get_binding` and some `DT`
macros. Once we have a `struct device*`, we can call `gpio_pin_configure`
with the `struct device*`, a pin number of type `gpio_pin_t`, and a set of
flags such as `GPIO_OUTPUT_LOW` or `GPIO_INPUT | GPIO_PULL_UP`. We can also
call `gpio_pin_get` to read the value of an input pin.

The GPIO device, pin number, and initial flags are usually stored in the
devicetree. For example, the STM32 Nucleo F103RB evaluation board has an
LED connected to A5 and a pushbutton connected to C13:
```
    leds {
        compatible = "gpio-leds";
        green_led_2: led_2 {
            gpios = <&gpioa 5 GPIO_ACTIVE_HIGH>;
            label = "User LD2";
        };
    };

    gpio_keys {
        compatible = "gpio-keys";
        user_button: button {
            label = "User";
            gpios = <&gpioc 13 GPIO_ACTIVE_LOW>;
        };

```

Getting the GPIO device, pin number, and flags requires using `DT` macros
(usually with an `alias` node in the devicetree to make the name shorter).

To make it easier to use GPIOs, `gpio_signal.h` provides macros and functions
that wrap the various Zephyr functions you would otherwise need to call.

## Declare GPIOs in devicetree

In the devicetree file, add a node with a logical name, like the name of the
module that will use the GPIOs. For example, Chameleon's SD card signal mux
has 5 GPIOs. Here is what the devicetree looks like:

```
    sd-mux {
        compatible = "gpio-keys";

        sd_mux_sel {
            gpios = <&gpioe 0 0>;
            label = "SD_MUX_SEL";
        };

        sd_mux_en_l {
        	gpios = <&gpioe 1 0>;
        	label = "SD_MUX_EN_L";
        };

        usd_pwr_sel {
        	gpios = <&gpiob 13 0>;
        	label = "USD_PWR_SEL";
        };

        usd_pwr_en {
        	gpios = <&gpioe 4 0>;
        	label = "USD_PWR_EN";
        };

        usd_cd_det {
        	gpios = <&gpioe 2 0>;
        	label = "USD_CD_DET";
        };
    };
```

Note that we need to declare a `compatible` so that the `gpios` and `label`
are properly typed and recognized. In Zephyr's devicetree schema, a GPIO
is either an output (`gpio-leds`) or an input (`gpio_keys`), so we have
arbitrarily chosen to use `gpio-keys` for our custom GPIOs, even though in this
case, `usd_cd_det` will be an input.

## Include gpio_signal.h

To use the GPIOs from the devicetree, a module includes `gpio_signal.h` and
instantiates a single macro with the name of the node in the devicetree.

Macros wrap up the array definitions, an anonymous enum, and a lookup macro,
so that the user only has to provide the path in the devicetree for the
GPIOs:
```
#include "gpio_signal.h"

DECLARE_GPIOS_FOR(sd_mux);
```

## Using GPIOs

Use `GPIO_LOOKUP(devicetree_path)` to get both the device pointer and the
pin number. `GPIO_LOOKUP` can be used directly in `gpio_pin` API calls:

```
gpio_pin_configure(GPIO_LOOKUP(sd_mux, sd_mux_sel), flags);
gpio_pin_set(GPIO_LOOKUP(sd_mux, sd_mux_sel), value);
gpio_pin_get(GPIO_LOOKUP(sd_mux, sd_mux_sel));
```

## gpio_signal.h Internals

`DECLARE_GPIOS_FOR` defines three arrays to hold the pin number, device
pointer, and device name.
* `static const gpio_pin_t gpio_pins[]`
* `static const struct device *gpio_devs[]`
* `static const char *const gpio_dev_names[]`

A number of support macros allow these arrays to be initialized through
`DT_FOREACH_CHILD` macros. Note that the order in which `DT_FOREACH_CHILD`
can vary from one build to another, but it will always be the same within
a single module, so we can be assured that the name and pin correspond.

`gpio_pins` and `gpio_dev_names` can be initialized `const` at compile time.
`gpio_devs` has to be initialized with `NULL` pointers and in the same size
as the other two arrays.

`DECLARE_GPIOS_FOR` also declares a function that walks the `gpio_dev_names`
array and for each element, does a `device_get_binding` and stores the result
in the corresponding element of `gpio_devs`. This function gets called at
startup through a `SYS_INIT` call at the `APPLICATION` level with a
priority of 1.

To make it easier to access the arrays, `DECLARE_GPIOS_FOR` also creates an
anonymous enumeration, which the macro `GPIO_LOOKUP` uses to index into
each of the arrays. `GPIO_LOOKUP` expands to two values, the device pointer
out of `gpio_devs` and the pin number out of `gpio_pins`. For this reason,
`GPIO_LOOKUP` can only be used to pass the `struct device *` and `gpio_pin_t`
to an API function that expects those two parameters in that order; there
is no variable type (not in C, anyway) that could be assigned the result
of `GPIO_LOOKUP`.
