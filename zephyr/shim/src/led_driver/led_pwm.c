/* Copyright 2022 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * PWM LED control.
 */

#include "ec_commands.h"
#include "led.h"
#include "util.h"

#include <devicetree.h>
#include <drivers/pwm.h>
#include <logging/log.h>

#if DT_HAS_COMPAT_STATUS_OKAY(COMPAT_PWM_LED)

LOG_MODULE_REGISTER(pwm_led, LOG_LEVEL_ERR);

/*
 * Struct defining LED PWM pin and duty cycle to set.
 */
struct pwm_pin_t {
	const struct device *pwm;
	uint8_t channel;
	pwm_flags_t flags;
	uint32_t pulse_ns; /* PWM Duty cycle ns */
};

/*
 * Pin node contains LED color and array of PWM channels
 * to alter in order to enable the given color.
 */
struct led_pins_node_t {
	/* Link between color and pins node */
	int led_color;

	/*
	 * Link between color and pins node in case of multiple LEDs
	 * Also required for ectool funcs support
	 */
	enum ec_led_id led_id;

	/* Brightness Range color, only used by ectool funcs for testing */
	enum ec_led_colors br_color;

	/* Array of PWM pins to set to enable particular color */
	struct pwm_pin_t *pwm_pins;

	/* Number of pins per color */
	uint8_t pins_count;
};

/*
 * Period in ns from frequency(Hz) defined in pins node
 * period in sec = 1/freq
 * period in nsec = (1*nsec_per_sec)/freq
 * This value is also used calculate duty_cycle in ns (pulse_ns below).
 * Duty cycle in perct defined in pin node is used to calculate pulse_ns
 * pulse_ns = (period_ns*duty_cycle_in_perct)/100
 * e.g. freq = 500 Hz, period_ns = 1000000000/500 = 2000000ns
 * duty_cycle = 50 %, pulse_ns  = (2000000*50)/100 = 1000000ns
 */
const uint32_t period_ns =
		(NSEC_PER_SEC / DT_PROP(PWM_LED_PINS_NODE, pwm_frequency));

#define SET_PIN(node_id, prop, i)					\
{									\
	.pwm = DEVICE_DT_GET(						\
		DT_PWMS_CTLR(DT_PHANDLE_BY_IDX(node_id, prop, i))),	\
	.channel = DT_PWMS_CHANNEL(					\
			DT_PHANDLE_BY_IDX(node_id, prop, i)),		\
	.flags = DT_PWMS_FLAGS(DT_PHANDLE_BY_IDX(node_id, prop, i)),	\
	.pulse_ns = DIV_ROUND_NEAREST(					\
	   period_ns * DT_PHA_BY_IDX(node_id, prop, i, value), 100),	\
},

#define SET_PWM_PIN(node_id)						\
{									\
	DT_FOREACH_PROP_ELEM(node_id, led_pins, SET_PIN)		\
};

#define GEN_PINS_ARRAY(id)						\
struct pwm_pin_t PINS_ARRAY(id)[] = SET_PWM_PIN(id)

DT_FOREACH_CHILD(PWM_LED_PINS_NODE, GEN_PINS_ARRAY)

#define SET_PIN_NODE(node_id)						\
{									\
	.led_color = GET_PROP(node_id, led_color),			\
	.led_id = GET_PROP(node_id, led_id),				\
	.br_color = GET_PROP_NVE(node_id, br_color),			\
	.pwm_pins = PINS_ARRAY(node_id),				\
	.pins_count = DT_PROP_LEN(node_id, led_pins)			\
},

struct led_pins_node_t pins_node[] = {
	DT_FOREACH_CHILD(PWM_LED_PINS_NODE, SET_PIN_NODE)
};

/*
 * Iterate through LED pins nodes to find the color matching node.
 * Set all the PWM channels defined in the node to the defined value,
 * to enable the color. Defined value is duty cycle in percentage
 * converted to duty cycle in ns (pulse_ns)
 */
void led_set_color(enum led_color color, enum ec_led_id led_id)
{
	for (int i = 0; i < ARRAY_SIZE(pins_node); i++) {
		if ((pins_node[i].led_color == color) &&
		    (pins_node[i].led_id == led_id)) {
			for (int j = 0; j < pins_node[i].pins_count; j++) {
				pwm_set(
					pins_node[i].pwm_pins[j].pwm,
					pins_node[i].pwm_pins[j].channel,
					period_ns,
					pins_node[i].pwm_pins[j].pulse_ns,
					pins_node[i].pwm_pins[j].flags);
			}
			break; /* Found the matching pin node, break here */
		}
	}
}

void led_get_brightness_range(enum ec_led_id led_id, uint8_t *brightness_range)
{
	for (int i = 0; i < ARRAY_SIZE(pins_node); i++) {
		int br_color = pins_node[i].br_color;

		if (br_color != -1)
			brightness_range[br_color] = 100;
	}
}

int led_set_brightness(enum ec_led_id led_id, const uint8_t *brightness)
{
	bool color_set = false;

	for (int i = 0; i < ARRAY_SIZE(pins_node); i++) {
		int br_color = pins_node[i].br_color;

		if ((br_color != -1) && (brightness[br_color] != 0)) {
			color_set = true;
			led_set_color(pins_node[i].led_color, led_id);
		}
	}

	/* If no color was set, turn off the LED */
	if (!color_set)
		led_set_color(LED_OFF, led_id);

	return EC_SUCCESS;
}

__override int led_is_supported(enum ec_led_id led_id)
{
	static int supported_leds = -1;

	if (supported_leds == -1) {
		supported_leds = 0;

		for (int i = 0; i < ARRAY_SIZE(pins_node); i++)
			supported_leds |= (1 << pins_node[i].led_id);
	}

	return ((1 << (int)led_id) & supported_leds);
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(COMPAT_PWM_LED) */
