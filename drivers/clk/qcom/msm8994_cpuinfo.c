/*
 * Copyright (c) 2016, Tom G., <roboter972@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/fs.h>

#include "clock-cpu-8994.h"

struct dentry *cpuinfo_rootdir;

static int a53sbin_get(void *data, u64 *val)
{
	*val = a53speedbin;

	return 0;
}

static int a57sbin_get(void *data, u64 *val)
{
	*val = a57speedbin;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(a53sbin_ops, a53sbin_get, NULL, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(a57sbin_ops, a57sbin_get, NULL, "%llu\n");

static int cpuinfo_debugfs_init(void)
{
	cpuinfo_rootdir = debugfs_create_dir("msm8994_cpuinfo", NULL);
	if (IS_ERR_OR_NULL(cpuinfo_rootdir)) {
		pr_err("Failed to create cpuinfo root dir. Error: %ld\n",
						PTR_ERR(cpuinfo_rootdir));
		return -ENODEV;
	}

	if (debugfs_create_file("A53-Speedbin", S_IRUGO,
			cpuinfo_rootdir, NULL, &a53sbin_ops) == NULL)
		goto init_failed;
	if (debugfs_create_file("A57-Speedbin", S_IRUGO,
			cpuinfo_rootdir, NULL, &a57sbin_ops) == NULL)
		goto init_failed;

	return 0;

init_failed:
	debugfs_remove_recursive(cpuinfo_rootdir);
	return -ENOMEM;
}

static int __init cpuinfo_init(void)
{
	int rc;

	rc = cpuinfo_debugfs_init();
	if (rc) {
		pr_err("Failed to initialize msm8994 cpuinfo debugfs\n");
		return rc;
	}

	return 0;
}

static void __exit cpuinfo_exit(void)
{
	debugfs_remove_recursive(cpuinfo_rootdir);
}

late_initcall(cpuinfo_init);
module_exit(cpuinfo_exit);
