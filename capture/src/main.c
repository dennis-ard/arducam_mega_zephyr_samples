/*
 * Copyright (c) 2022, Kumar Gala <galak@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video/arducam_mega.h>

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
	struct video_buffer *vbuf;
	struct video_format fmt;
	// struct video_caps caps;
	// uint8_t i = 0;
	size_t bsize;

	const struct device *const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (!device_is_ready(console)) {
		LOG_ERR("%s: device not ready.", console->name);
		return 0;
	}
	
	const struct device *const video = DEVICE_DT_GET(DT_NODELABEL(arducam_mega0));
	
	if (!device_is_ready(video)) {
		LOG_ERR("Video device %s not ready.", video->name);
		return 0;
	}

	LOG_INF("Mega star");

	printk("- Device name: %s\n", video->name);

	// /* Get capabilities */
	// if (video_get_caps(video, VIDEO_EP_OUT, &caps)) {
	// 	LOG_ERR("Unable to retrieve video capabilities");
	// 	return 0;
	// }

	// printk("- Capabilities:\n");
	// while (caps.format_caps[i].pixelformat) {
	// 	const struct video_format_cap *fcap = &caps.format_caps[i];
	// 	/* fourcc to string */
	// 	printk("  %c%c%c%c width [%u; %u; %u] height [%u; %u; %u]\n",
	// 	       (char)fcap->pixelformat,
	// 	       (char)(fcap->pixelformat >> 8),
	// 	       (char)(fcap->pixelformat >> 16),
	// 	       (char)(fcap->pixelformat >> 24),
	// 	       fcap->width_min, fcap->width_max, fcap->width_step,
	// 	       fcap->height_min, fcap->height_max, fcap->height_step);
	// 	k_msleep(5);
	// 	i++;
	// }

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

	fmt.width = 320;
	fmt.height = 240;

	video_set_format(video, VIDEO_EP_OUT, &fmt);

	/* Get format */
	if (video_get_format(video, VIDEO_EP_OUT, &fmt)) {
		LOG_ERR("Unable to retrieve video format");
		return 0;
	}

	printk("- Set format: %c%c%c%c %ux%u\n", (char)fmt.pixelformat,
	       (char)(fmt.pixelformat >> 8),
	       (char)(fmt.pixelformat >> 16),
	       (char)(fmt.pixelformat >> 24),
	       fmt.width, fmt.height);

	/* Size to allocate for each buffer */
	bsize = 1024;

	/* Alloc video buffers and enqueue for capture */
	vbuf = video_buffer_alloc(bsize);
	if (vbuf == NULL) {
		LOG_ERR("Unable to alloc video buffer");
		return 0;
	}

	while (1) 
	{
		int err;
		uint8_t c=0;
		uint32_t frame_len;
		uart_poll_in(console, &c);
		if (c != 0x0a){
			k_msleep(1);
			continue;
		}

		err = video_set_ctrl(video, VIDEO_CID_ARDUCAM_CAPTURE, &frame_len);
		// if (err == (-EAGAIN)){
		//  	continue;
		// }
		if (err!=0) {
			LOG_ERR("Unable to take picture, %d", err);
			return 0;
		}
		// LOG_INF("take picture length:%u.", frame_len);
		while (frame_len > 0)
		{
			err = video_get_ctrl(video, VIDEO_CID_ARDUCAM_CAPTURE, vbuf);
			if ( err == 0)
			{
				uart_buffer_send(console, vbuf->buffer, vbuf->bytesused);
				// LOG_INF("read fifo :%u.", vbuf->bytesused);
				frame_len -= vbuf->bytesused;
			} else {
				// LOG_INF("read fifo :%u,err %d.", vbuf->bytesused, err);
			}
		}
	}
	k_msleep(100);
	return 0;
}
