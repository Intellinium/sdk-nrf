/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2022 INTELLINIUM <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LTE_UART_DFU_H_
#define _LTE_UART_DFU_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Add comment about function
 *
 *
 *
 * @return Add comment about possible return values
 */
int lte_uart_dfu_start(const struct device *uart);

#ifdef __cplusplus
}
#endif

#endif /* _LTE_UART_DFU_H_ */
