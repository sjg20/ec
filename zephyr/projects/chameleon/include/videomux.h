/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __VIDEOMUX_H
#define __VIDEOMUX_H

#include <zephyr.h>

/*
 * The Chameleon v3 hardware has a series of muxes to route the HDMI or the
 * DisplayPort (after going through a DP/HDMI converter) signals to the
 * FPGA's MGTs or to an HDMI receiver (IT68051). There are two sets of
 * circuitry, one for each of the HDMI/DP combo jacks.
 * The selections for the muxes are shown in the table below, along with the
 * reference designator on Proto0 hardware for the specific mux.
 *
 *  GPU_SEL on  SW on       GPU_SEL on  GPU_SEL on
 *  CBTL06GP213 PS8468      CBTL06GP213 CBTL06DP213
 *  mux         demux       demux       mux
 *  U18         U3          U10         U11         HDMI1   DP1
 *  U14         U22         U29         U31         HDMI2   DP2
 *  ----------- ----------- ----------- ----------- ------- -------
 *  1           2           1           2           n/c     n/c
 *  1           1           2           2           FPGA    IT68051
 *  2           2           1           1           IT68051 FPGA
 *
 * So, to connect the HDMI signal to the FPGA's MGTs and the DisplayPort
 * signal to the HDMI receiver, set:
 *  1. GP213_IT68051P1_CH_SEL (GPU_SEL on U18) low to select channel 1
 *  2. DP1_PS8468_SW (SW on U3) low  to select channel 1
 *  3. HDMI1_GP213_CH_SEL (GPU_SEL on U10) high to select channel 2
 *  4. SOMP1_MODE_SEL (GPU_SEL on U11) to select channel 2
 *
 * The muxes *can* be configured so that neither signal is routed to either
 * destination, but in practice, this would not be wanted, and so it isn't
 * offered as an option in the API.
 */

enum videomux_select_t {
	HDMI_TO_FPGA_DP_TO_RECEIVER,
	HDMI_TO_RECEIVER_DP_TO_FGPA
};

/**
 * @brief Configure the muxes to route the vidoe signals
 *
 * @param port Port number to route, either 1 or 2
 *
 * @param sel HDMI_TO_FPGA_DP_TO_RECEIVER to route the HDMI signals to
 * the FPGA's MGTS and the DisplayPort signals to the HDMI receiver, or
 * HDMI_TO_RECEIVER_DP_TO_FGPA to route the HDMI signals to the HDMI
 * receiver, and the Display signals to the FPGA's MGTs.
 *
 * @retval
 * 0 Everything OK
 * @retval
 * -EINVAL Invalid port number
 */
int videomux_select(int port, enum videomux_select_t sel);

#endif
