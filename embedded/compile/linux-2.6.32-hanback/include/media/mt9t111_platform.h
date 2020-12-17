/* linux/include/media/mt9t111_platform.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com/
 *
 * Driver for MT9T111 from Samsung Electronics
 * 1/4" 3.0Mp CMOS Image Sensor SoC with an Embedded Image Processor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

struct mt9t111_platform_data {
	unsigned int default_width;
	unsigned int default_height;
	unsigned int pixelformat;
	int freq;	/* MCLK in KHz */

	/* This SoC supports Parallel & CSI-2 */
	int is_mipi;
};

