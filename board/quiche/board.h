/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Quiche board configuration */

#ifndef __CROS_EC_BOARD_H
#define __CROS_EC_BOARD_H

/* Baseboard features */
#include "baseboard.h"

/* Optional features */
#define CONFIG_SYSTEM_UNLOCKED /* Allow dangerous commands while in dev. */

#undef CONFIG_UART_TX_BUF_SIZE
#define CONFIG_UART_TX_BUF_SIZE 4096

/* Keyboard features */

/* Sensors */

/* USB Type C and USB PD defines */

/* USB Type A Features */

/* BC 1.2 */

/* Volume Button feature */

/* Fan features */


#ifndef __ASSEMBLER__

#include "registers.h"

#endif /* !__ASSEMBLER__ */

#endif /* __CROS_EC_BOARD_H */
