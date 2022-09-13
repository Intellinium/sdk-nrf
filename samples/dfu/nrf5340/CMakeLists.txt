#
# Copyright (c) 2022 INTELLINIUM <giuliano.franchetto@intellinium.com>
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(lte_uart_update_53)

target_sources(app PRIVATE src/main.c)
