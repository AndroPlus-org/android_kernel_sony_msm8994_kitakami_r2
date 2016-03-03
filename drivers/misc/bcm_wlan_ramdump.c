/* bcm_wlan_ramdump.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/*
 * Copyright (C) 2014 Sony Mobile Communications Inc.
 * Copyright (C) 2016 Tom G., <roboter972@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/bcm_wlan_ramdump.h>

void bcm_wlan_ramdump(void *addr, int size)
{
}
EXPORT_SYMBOL(bcm_wlan_ramdump);

void bcm_wlan_crash_reason(char *msg)
{
}
EXPORT_SYMBOL(bcm_wlan_crash_reason);

MODULE_DESCRIPTION("bcm wlan ramdump");
MODULE_LICENSE("GPL v2");
