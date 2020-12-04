/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <shell/shell.h>
#include "gpio_signal.h"
#include "videomux.h"

DECLARE_GPIOS_FOR(videomux);

int videomux_select(int port, enum videomux_select_t sel)
{
	if (port == 1) {
		gpio_pin_set(GPIO_LOOKUP(videomux, gp213_it68051p1_ch_sel),
			     sel == HDMI_TO_FPGA_DP_TO_RECEIVER ? 1 : 2);
		gpio_pin_set(GPIO_LOOKUP(videomux, dp1_ps8468_sw),
			     sel == HDMI_TO_FPGA_DP_TO_RECEIVER ? 1 : 2);
		gpio_pin_set(GPIO_LOOKUP(videomux, hdmi1_gp213_ch_sel),
			     sel == HDMI_TO_FPGA_DP_TO_RECEIVER ? 2 : 1);
		gpio_pin_set(GPIO_LOOKUP(videomux, somp1_mode_sel),
			     sel == HDMI_TO_FPGA_DP_TO_RECEIVER ? 2 : 1);
	} else if (port == 2) {
		gpio_pin_set(GPIO_LOOKUP(videomux, gp213_it68051p0_ch_sel),
			     sel == HDMI_TO_FPGA_DP_TO_RECEIVER ? 1 : 2);
		gpio_pin_set(GPIO_LOOKUP(videomux, dp2_ps8468_sw),
			     sel == HDMI_TO_FPGA_DP_TO_RECEIVER ? 1 : 2);
		gpio_pin_set(GPIO_LOOKUP(videomux, hdmi2_gp213_ch_sel),
			     sel == HDMI_TO_FPGA_DP_TO_RECEIVER ? 2 : 1);
		gpio_pin_set(GPIO_LOOKUP(videomux, somp2_mode_sel),
			     sel == HDMI_TO_FPGA_DP_TO_RECEIVER ? 2 : 1);
	} else {
		return -EINVAL;
	}
	return 0;
}

/**
 * @brief Set port 1 to route HDMI to the FPGA, DisplayPort to the Receiver
 */
static int cmd_videomux_set_port1_hdmi_fpga(const struct shell *shell,
					    size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
		      "Connect HDMI1 to FPGA, DisplayPort1 to receiver\n");
	return videomux_select(1, HDMI_TO_FPGA_DP_TO_RECEIVER);
}

/**
 * @brief Set port 1 to route HDMI to the Receiver, DisplayPort to the FPGA
 */
static int cmd_videomux_set_port1_dp_fpga(const struct shell *shell,
					  size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
		      "Connect DisplayPort1 to FPGA, HDMI1 to receiver\n");
	return videomux_select(1, HDMI_TO_RECEIVER_DP_TO_FGPA);
}

/**
 * @brief Set port 2 to route HDMI to the FPGA, DisplayPort to the Receiver
 */
static int cmd_videomux_set_port2_hdmi_fpga(const struct shell *shell,
					    size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
		      "Connect HDMI2 to FPGA, DisplayPort2 to receiver\n");
	return videomux_select(2, HDMI_TO_FPGA_DP_TO_RECEIVER);
}

/**
 * @brief Set port 2 to route HDMI to the Receiver, DisplayPort to the FPGA
 */
static int cmd_videomux_set_port2_dp_fpga(const struct shell *shell,
					  size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
		      "Connect DisplayPort2 to FPGA, HDMI2 to receiver\n");
	return videomux_select(2, HDMI_TO_RECEIVER_DP_TO_FGPA);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	port1_cmds,
	SHELL_CMD(hdmi, NULL, "Route HDMI to FPGA, DisplayPort to receiver",
		  cmd_videomux_set_port1_hdmi_fpga),
	SHELL_CMD(dp, NULL, "Route DisplayPort to FPGA, HDMI to receiver",
		  cmd_videomux_set_port1_dp_fpga),
	SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(
	port2_cmds,
	SHELL_CMD(hdmi, NULL, "Route HDMI to FPGA, DisplayPort to receiver",
		  cmd_videomux_set_port2_hdmi_fpga),
	SHELL_CMD(dp, NULL, "Route DisplayPort to FPGA, HDMI to receiver",
		  cmd_videomux_set_port2_dp_fpga),
	SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(videomux_cmds,
			       SHELL_CMD(1, &port1_cmds, "Port 1", NULL),
			       SHELL_CMD(2, &port2_cmds, "Port 2", NULL),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(videomux, &videomux_cmds, "Control the video mux", NULL);
