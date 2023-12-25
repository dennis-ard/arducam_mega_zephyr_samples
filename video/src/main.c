/*
 * Copyright (c) 2022, Kumar Gala <galak@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/uart.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

void uart_buffer_send(const struct device * dev,uint8_t* buf, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++) {
        uart_poll_out(dev,buf[i]);
    }
}

int main(void)
{
	struct video_buffer *buffers[2], *vbuf;
	struct video_format fmt;
	// struct video_caps caps;
	size_t bsize;
	const struct device *const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (!device_is_ready(console)) {
		LOG_ERR("%s: device not ready.", console->name);
		return 0;
	}
	
	const struct device *const video = DEVICE_DT_GET(DT_NODELABEL(arducam_mega0));
	
	if (!device_is_ready(video)) {
		LOG_ERR("%s: device not ready.", video->name);
		return 0;
	}

	LOG_INF("Mega star");

	printk("- Device name: %s\n", video->name);

	/* Get default/native format */
	if (video_get_format(video, VIDEO_EP_OUT, &fmt)) {
		LOG_ERR("Unable to retrieve video format");
		return 0;
	}

	printk("- Default format: %c%c%c%c %ux%u\n", (char)fmt.pixelformat,
	       (char)(fmt.pixelformat >> 8),
	       (char)(fmt.pixelformat >> 16),
	       (char)(fmt.pixelformat >> 24),
	       fmt.width, fmt.height);

	/* Size to allocate for each buffer */
	bsize = fmt.pitch * fmt.height;

	/* Alloc video buffers and enqueue for capture */
	for (uint8_t i = 0; i < ARRAY_SIZE(buffers); i++) {
		buffers[i] = video_buffer_alloc(bsize);
		if (buffers[i] == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return 0;
		}
		video_enqueue(video, VIDEO_EP_OUT, buffers[i]);
	}

	/* Start video capture */
	int err = video_stream_start(video);
	if (err!=0 && err!=1) {
		LOG_ERR("Unable to start capture (interface), %d", err);
		video_stream_stop(video);
		return 0;
	}
	
	LOG_INF("Capture started\n");

	while (1) {
		int err;
		uint8_t c=0;

		uart_poll_in(console, &c);
		if (c != 0x0a){
			k_msleep(1);
			continue;
		}

		err = video_dequeue(video, VIDEO_EP_OUT, &vbuf, K_MSEC(33));
		if (err == (-EAGAIN)){
		 	continue;
		}
		if (err!=0) {
			LOG_ERR("Unable to dequeue video buf, %d", err);
			return 0;
		}

		// printk("\rGot frame %u! size: %u; timestamp %u ms",
		// 	frame++, vbuf->bytesused, vbuf->timestamp);
		uart_buffer_send(console, vbuf->buffer,vbuf->bytesused);

		err = video_enqueue(video, VIDEO_EP_OUT, vbuf);
		if (err) {
			LOG_ERR("Unable to requeue video buf, %d", err);
			return 0;
		}
	}
	return 0;
}
