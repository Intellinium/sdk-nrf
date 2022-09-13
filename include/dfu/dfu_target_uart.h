/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2022 INTELLINIUM <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DFU_TARGET_UART_H__
#define DFU_TARGET_UART_H__

#include <stddef.h>
#include <stdbool.h>
#include <dfu/dfu_target.h>

#ifdef __cplusplus
extern "C" {
#endif

enum dfu_uart_target_func {
	DFU_UART_INIT,
	DFU_UART_WRITE,
	DFU_UART_OFFSET,
	DFU_UART_DONE
};

struct dfu_uart_packet {
	const void *data;
	size_t len;
};

struct dfu_uart_buffer {
	uint8_t *buffer;
	size_t offs;
	size_t buf_size;
};

#define DFU_UART_HEADER_MAGIC               0x85f3d83a

#define DFU_UART_MAGIC_START                "xogq"
#define DFU_UART_MAGIC_STOP                 "foqs"

#define DFU_PACKET(d, s) {.data= (d), .len = (s)}
#define DFU_PACKET_MAGIC_START \
        DFU_PACKET(DFU_UART_MAGIC_START, sizeof(DFU_UART_MAGIC_START) - 1)
#define DFU_PACKET_MAGIC_STOP  \
        DFU_PACKET(DFU_UART_MAGIC_STOP, sizeof(DFU_UART_MAGIC_STOP) - 1)

static inline int dfu_uart_append_buffer(const struct dfu_uart_packet *packet,
					 struct dfu_uart_buffer *buffer)
{
	if (buffer->offs + packet->len > buffer->buf_size) {
		return -ENOMEM;
	}

	memcpy(buffer->buffer + buffer->offs, packet->data, packet->len);
	buffer->offs += (int) packet->len;

	return 0;
}

static inline int
dfu_uart_fill_out_buffer(const struct dfu_uart_packet *packet,
			 size_t count,
			 struct dfu_uart_buffer *buffer)
{
	int err = 0;

	buffer->offs = 0;

	for (int i = 0; i < count; i++) {
		err = dfu_uart_append_buffer(packet + i, buffer);
		if (err) {
			break;
		}
	}

	if (err) {
		buffer->offs = 0;
	}

	return err;
}

bool dfu_target_uart_identify(const void *buf);

int dfu_target_uart_init(size_t file_size, int img_num,
			 dfu_target_callback_t cb);

int dfu_target_uart_offset_get(size_t *offset);

int dfu_target_uart_write(const void *buf, size_t len);

int dfu_target_uart_done(bool successful);

int dfu_target_uart_schedule_update(int img_num);

#ifdef __cplusplus
}
#endif

#endif /* DFU_TARGET_UART_H__ */
