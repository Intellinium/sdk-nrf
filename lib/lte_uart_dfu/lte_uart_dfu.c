/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2022 INTELLINIUM <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/uart.h>
#include <string.h>
#include <lte_uart_dfu.h>
#include <dfu/dfu_target_mcuboot.h>
#include <dfu/dfu_target_uart.h>
#include <dfu/dfu_target.h>
#include <logging/log.h>
#include <stdnoreturn.h>

LOG_MODULE_REGISTER(lte_uart_dfu, CONFIG_LTE_UART_DFU_LOG_LEVEL);

K_THREAD_STACK_DEFINE(uart_handler_stack,
		      CONFIG_LTE_UART_DFU_HANDLER_STACK_SIZE);
static struct k_thread uart_handler_thread;

static uint8_t in_buf[CONFIG_LTE_UART_DFU_IN_BUFFER_SIZE];
static uint8_t out_buf[CONFIG_LTE_UART_DFU_OUT_BUFFER_SIZE];

static struct dfu_uart_buffer output_buffer = {
	.buffer = out_buf,
	.offs = 0,
	.buf_size = ARRAY_SIZE(out_buf)
};

static struct dfu_uart_buffer input_buffer = {
	.buffer = in_buf,
	.offs = 0,
	.buf_size = ARRAY_SIZE(in_buf)
};

static const struct device *uart_dev;

K_SEM_DEFINE(lte_uart_sem, 0, 1);

static int lte_uart_dfu_send_out_buffer(void)
{
	if (output_buffer.offs == 0) {
		LOG_WRN("No data to send");
		return 0;
	}

	for (int i = 0; i < output_buffer.offs; i++) {
		uart_poll_out(uart_dev, output_buffer.buffer[i]);
	}

	output_buffer.offs = 0;

	return 0;
}

static int lte_uart_dfu_send_error(enum dfu_uart_target_func func, int error)
{
	int err;
	struct dfu_uart_packet packets[] = {
		DFU_PACKET_MAGIC_START,
		DFU_PACKET(&func, sizeof(func)),
		DFU_PACKET(&error, sizeof(error)),
		DFU_PACKET_MAGIC_STOP
	};

	err = dfu_uart_fill_out_buffer(packets,
				       ARRAY_SIZE(packets),
				       &output_buffer);
	if (err) {
		LOG_ERR("Could not fill buffer, error %d", err);
		return err;
	}

	return lte_uart_dfu_send_out_buffer();
}

static int lte_uart_dfu_send_offset(int error, size_t offset)
{
	int err;
	int val = error == 0 ? (int) offset : error;

	enum dfu_uart_target_func func = DFU_UART_OFFSET;
	struct dfu_uart_packet packets[] = {
		DFU_PACKET_MAGIC_START,
		DFU_PACKET(&func, sizeof(func)),
		DFU_PACKET(&val, sizeof(val)),
		DFU_PACKET_MAGIC_STOP
	};

	err = dfu_uart_fill_out_buffer(packets,
				       ARRAY_SIZE(packets),
				       &output_buffer);
	if (err) {
		return err;
	}

	return lte_uart_dfu_send_out_buffer();
}

struct write_func {
	char *buf;
	int len;
	int ret_val;
};

struct done_func {
	bool successful;
	int ret_val;
};

struct init_func {
	size_t file_size;
	uint8_t image_num;
	int ret_val;
};

struct offset_func {
	size_t offset;
	int ret_val;
};

struct lte_uart_type {
	struct write_func write;
	struct done_func done;
	struct offset_func offset;
	struct init_func init;
};

static struct lte_uart_type lte_uart;
static bool download_started;
static enum dfu_uart_target_func dfu_func;

static void unused_func(enum dfu_target_evt_id evt)
{
	ARG_UNUSED(evt);
}

static int expected_write_len = -1;
static void uart_cb(const struct device *x, void *p)
{
	ARG_UNUSED(p);

	int rx;
	uint8_t byte;

	uart_irq_update(x);

	if (!uart_irq_rx_ready(x)) {
		return;
	}

	while (true) {
		rx = uart_fifo_read(x, &byte, 1);
		if (rx != 1) {
			break;
		}

		if (input_buffer.offs < input_buffer.buf_size) {
			input_buffer.buffer[input_buffer.offs] = byte;
		}

		if (input_buffer.offs < 4) {
			goto next;
		}

		if (input_buffer.offs == 4) {
			dfu_func = (int) input_buffer.buffer[4];
		}

		if (dfu_func == DFU_UART_WRITE && input_buffer.offs >= 9) {
			if (expected_write_len == -1) {
				memcpy(&expected_write_len,
				       &input_buffer.buffer[5],
				       sizeof(expected_write_len));
				LOG_DBG("DFU_UART_WRITE length: %d", expected_write_len);
			}
			if (input_buffer.offs < 8 + expected_write_len) {
				goto next;
			}
		}

		if (strncmp(&input_buffer.buffer[input_buffer.offs - 3],
			    DFU_UART_MAGIC_STOP, 4) == 0) {
			expected_write_len = -1;
			k_sem_give(&lte_uart_sem);
			break;
		}

next:
		input_buffer.offs++;
	}
}

static int lte_uart_dfu_parse_init(void)
{
	int length;
	uint8_t image_num;

	if (input_buffer.offs < 10) {
		return -EINVAL;
	}

	memcpy(&length, &input_buffer.buffer[5], sizeof(length));
	image_num = input_buffer.buffer[9];

	download_started = false;
	lte_uart.init.file_size = length;
	lte_uart.init.image_num = image_num;

	return 0;
}

static int lte_uart_dfu_parse_write(void)
{
	int length;

	if (input_buffer.offs < 9) {
		return -EINVAL;
	}

	memcpy(&length, &input_buffer.buffer[5], sizeof(length));

	if (input_buffer.offs < 9 + length) {
		return -EINVAL;
	}

	lte_uart.write.len = length;
	lte_uart.write.buf = &input_buffer.buffer[9];

	if (!download_started) {
		download_started = true;
	}

	return 0;
}

static int lte_uart_dfu_parse_done(void)
{
	if (input_buffer.offs < 6) {
		return -EINVAL;
	}

	download_started = false;
	lte_uart.done.successful = (int) input_buffer.buffer[5];

	return 0;
}

static void lte_uart_dfu_parse_input_data(void)
{
	int err;
	size_t offs_get;

	if (input_buffer.offs < 5) {
		LOG_ERR("Not enough bytes received (%d bytes)",
			input_buffer.offs);
		return;
	}

	LOG_DBG("DFU function called: %d", dfu_func);

	switch (dfu_func) {
	case DFU_UART_INIT:
		err = lte_uart_dfu_parse_init();
		if (err) {
			LOG_ERR("Could not parse init, error %d", err);
			lte_uart_dfu_send_error(DFU_UART_INIT, err);
			break;
		}

		err = dfu_target_mcuboot_init(lte_uart.init.file_size,
					      lte_uart.init.image_num,
					      unused_func);
		lte_uart_dfu_send_error(DFU_UART_INIT, err);
		break;
	case DFU_UART_WRITE:
		err = lte_uart_dfu_parse_write();
		if (err) {
			lte_uart_dfu_send_error(DFU_UART_WRITE, err);
			break;
		}

		err = dfu_target_mcuboot_write(lte_uart.write.buf,
					       lte_uart.write.len);
		lte_uart_dfu_send_error(DFU_UART_WRITE, err);
		break;
	case DFU_UART_OFFSET:
		err = dfu_target_mcuboot_offset_get(&offs_get);
		lte_uart_dfu_send_offset(err, offs_get);
		break;
	case DFU_UART_DONE:
		err = lte_uart_dfu_parse_done();
		if (err) {
			lte_uart_dfu_send_error(DFU_UART_DONE, err);
			break;
		}

		err = dfu_target_mcuboot_done(lte_uart.done.successful);
		if (err) {
			lte_uart_dfu_send_error(DFU_UART_DONE, err);
			break;
		}

		err = dfu_target_mcuboot_schedule_update(lte_uart.init.image_num);
		lte_uart_dfu_send_error(DFU_UART_DONE, err);
		break;
	default:
		break;
	}

	input_buffer.offs = 0;
	memset(input_buffer.buffer, 0, input_buffer.buf_size);
}

noreturn static void uart_input_handler(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		k_sem_take(&lte_uart_sem, K_FOREVER);
		/* Protect the input buffer */
		uart_irq_rx_disable(uart_dev);
		/* Process the input buffer */
		lte_uart_dfu_parse_input_data();
		/* Re-enable rx as the input buffer has been processed */
		uart_irq_rx_enable(uart_dev);
	}
}

int lte_uart_dfu_start(const struct device *uart)
{
	if (!uart) {
		return -EINVAL;
	}

	input_buffer.offs = 0;
	output_buffer.offs = 0;
	uart_dev = uart;

	k_thread_create(&uart_handler_thread, uart_handler_stack,
			K_THREAD_STACK_SIZEOF(uart_handler_stack),
			uart_input_handler, NULL, NULL, NULL,
			CONFIG_LTE_UART_DFU_HANDLER_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&uart_handler_thread, "dfu_uart");

	uart_irq_callback_set(uart, uart_cb);
	uart_irq_rx_enable(uart);

	return 0;
}
