/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2022 INTELLINIUM <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/zephyr.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <dfu/dfu_target_uart.h>
#include <string.h>

LOG_MODULE_REGISTER(dfu_target_uart, CONFIG_DFU_TARGET_LOG_LEVEL);

#define UART_INPUT_BUF_SIZE        CONFIG_DFU_TARGET_UART_INPUT_BUF_SIZE
#define UART_OUTPUT_BUF_SIZE       CONFIG_DFU_TARGET_UART_OUTPUT_BUF_SIZE

#define RESP_TIMEOUT               K_MSEC(CONFIG_DFU_TARGET_UART_TIMEOUT)

K_SEM_DEFINE(dfu_targ_uart_sem, 0, 1);

static uint8_t in_buffer[UART_INPUT_BUF_SIZE];
static uint8_t out_buffer[UART_OUTPUT_BUF_SIZE];

static struct dfu_uart_buffer output_buffer = {
	.buffer = out_buffer,
	.offs = 0,
	.buf_size = ARRAY_SIZE(out_buffer)
};

static struct dfu_uart_buffer input_buffer = {
	.buffer = in_buffer,
	.offs = 0,
	.buf_size = ARRAY_SIZE(in_buffer)
};

static const struct device *uart_dev;

struct dfu_target_uart_input {
	int active_func;
	int error;
	int offset;
};

static struct dfu_target_uart_input input_fields;
static bool first_packet;

static int dfu_target_uart_send_out_buffer(void)
{
	if (output_buffer.offs == 0) {
		return 0;
	}

	for (int i = 0; i < output_buffer.offs; i++) {
		uart_poll_out(uart_dev, output_buffer.buffer[i]);
	}

	output_buffer.offs = 0;

	return 0;
}

static int dfu_target_uart_send_fragment(const void *buf, size_t size)
{
	int err;
	const void *payload;
	enum dfu_uart_target_func write = DFU_UART_WRITE;

	if (first_packet) {
		payload = buf + 4;
		size -= 4;
		first_packet = false;
	} else {
		payload = buf;
	}

	struct dfu_uart_packet packets[] = {
		DFU_PACKET_MAGIC_START,
		DFU_PACKET(&write, sizeof(write)),
		DFU_PACKET(&size, sizeof(size)),
		DFU_PACKET(payload, size),
		DFU_PACKET_MAGIC_STOP
	};

	err = dfu_uart_fill_out_buffer(packets,
				       ARRAY_SIZE(packets),
				       &output_buffer);
	if (err) {
		return err;
	}

	return dfu_target_uart_send_out_buffer();
}

static int dfu_target_uart_send_init(size_t file_size, uint8_t image_num)
{
	int err;
	enum dfu_uart_target_func init = DFU_UART_INIT;
	struct dfu_uart_packet packets[] = {
		DFU_PACKET_MAGIC_START,
		DFU_PACKET(&init, sizeof(init)),
		DFU_PACKET(&file_size, sizeof(file_size)),
		DFU_PACKET(&image_num, sizeof(image_num)),
		DFU_PACKET_MAGIC_STOP
	};

	err = dfu_uart_fill_out_buffer(packets,
				       ARRAY_SIZE(packets),
				       &output_buffer);
	if (err) {
		return err;
	}

	return dfu_target_uart_send_out_buffer();
}

static int dfu_target_uart_send_offset(void)
{
	int err;
	enum dfu_uart_target_func offset = DFU_UART_OFFSET;
	struct dfu_uart_packet packets[] = {
		DFU_PACKET_MAGIC_START,
		DFU_PACKET(&offset, sizeof(offset)),
		DFU_PACKET_MAGIC_STOP
	};

	err = dfu_uart_fill_out_buffer(packets,
				       ARRAY_SIZE(packets),
				       &output_buffer);
	if (err) {
		return err;
	}

	return dfu_target_uart_send_out_buffer();
}

static int dfu_target_uart_send_done(bool success)
{
	int err;
	enum dfu_uart_target_func done = DFU_UART_DONE;
	struct dfu_uart_packet packets[] = {
		DFU_PACKET_MAGIC_START,
		DFU_PACKET(&done, sizeof(done)),
		DFU_PACKET(&success, sizeof(success)),
		DFU_PACKET_MAGIC_STOP
	};

	err = dfu_uart_fill_out_buffer(packets,
				       ARRAY_SIZE(packets),
				       &output_buffer);
	if (err) {
		return err;
	}

	return dfu_target_uart_send_out_buffer();
}

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
		} else {
			LOG_WRN("RX buffer overflow");
			input_buffer.offs = 0;
			break;
		}

		if (input_buffer.offs >= 4 &&
			strncmp(&input_buffer.buffer[input_buffer.offs - 3],
				DFU_UART_MAGIC_STOP, 4) == 0) {
			k_sem_give(&dfu_targ_uart_sem);
			break;
		}

		input_buffer.offs++;
	}
}

bool dfu_target_uart_identify(const void *const buf)
{
	return *((const uint32_t *) buf) == DFU_UART_HEADER_MAGIC;
}

static int dfu_target_uart_check_input(enum dfu_uart_target_func func)
{
	/* TODO check size before setting to 0 */
	input_buffer.offs = 0;

	input_fields.active_func = (int) input_buffer.buffer[4];

	if (input_fields.active_func != func) {
		LOG_ERR("Wrong function type received: %d instead of %d",
			input_fields.active_func, func);
		return -EBADMSG;
	}

	if (input_fields.active_func != DFU_UART_OFFSET) {
		input_fields.error = (int) input_buffer.buffer[5];
	} else {
		input_fields.offset = (int) input_buffer.buffer[5];
		if (input_fields.offset < 0) {
			input_fields.error = input_fields.offset;
		} else {
			input_fields.error = 0;
		}
	}

	return input_fields.error;
}

int
dfu_target_uart_init(size_t file_size, int img_num, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);

	int err;

	/* This flag needs to be reset so the write function knows it
	 * must remove the first 4 bytes of the next packet */
	first_packet = true;

	uart_dev = device_get_binding(CONFIG_DFU_TARGET_UART_DEV_NAME);
	if (!device_is_ready(uart_dev)) {
		LOG_ERR("Could not get " CONFIG_DFU_TARGET_UART_DEV_NAME);
		return -ENOSYS;
	}

	uart_irq_callback_set(uart_dev, uart_cb);
	uart_irq_rx_enable(uart_dev);

	err = dfu_target_uart_send_init(file_size, img_num);
	if (err) {
		LOG_ERR("Could not send init message, error %d", err);
		return err;
	}

	err = k_sem_take(&dfu_targ_uart_sem, RESP_TIMEOUT);
	if (err) {
		LOG_ERR("Did not receive response from other side");
		return -EAGAIN;
	}

	return dfu_target_uart_check_input(DFU_UART_INIT);
}

int dfu_target_uart_offset_get(size_t *out)
{
	int err;

	err = dfu_target_uart_send_offset();
	if (err) {
		LOG_ERR("Could not send offset message, error %d", err);
		return err;
	}

	err = k_sem_take(&dfu_targ_uart_sem, RESP_TIMEOUT);
	if (err) {
		LOG_ERR("Did not receive response from other side");
		return -EAGAIN;
	}

	err = dfu_target_uart_check_input(DFU_UART_OFFSET);
	if (err) {
		return err;
	}

	*out = input_fields.offset;

	return 0;
}

int dfu_target_uart_write(const void *const buf, size_t len)
{
	int err;

	LOG_INF("Sending firmware fragment of length: %d", len);
	err = dfu_target_uart_send_fragment(buf, len);
	if (err) {
		LOG_ERR("Could not send fragment message, error %d", err);
		return err;
	}

	err = k_sem_take(&dfu_targ_uart_sem, RESP_TIMEOUT);
	if (err) {
		LOG_ERR("Did not receive response from other side");
		return -EAGAIN;
	}

	return dfu_target_uart_check_input(DFU_UART_WRITE);
}

int dfu_target_uart_done(bool successful)
{
	int err;

	err = dfu_target_uart_send_done(successful);
	if (err) {
		LOG_ERR("Could not send done message, error %d", err);
		return err;
	}

	err = k_sem_take(&dfu_targ_uart_sem, RESP_TIMEOUT);
	if (err) {
		LOG_ERR("Did not receive response from other side");
		return -EAGAIN;
	}

	return dfu_target_uart_check_input(DFU_UART_DONE);
}

int dfu_target_uart_schedule_update(int img_num)
{
	ARG_UNUSED(img_num);

	return 0;
}
