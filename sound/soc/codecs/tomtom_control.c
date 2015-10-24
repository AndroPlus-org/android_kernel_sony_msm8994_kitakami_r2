/*
 * Author: Tom G., <roboter972@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) "tomtom-control: " fmt

#include <linux/mfd/wcd9xxx/wcd9330_registers.h>

#include "wcd9330.h"
#include "tomtom_control.h"

enum dglims {
	DGAIN_MAX = 0x14,
	DGAIN_MIN = 0xEC
};

#define bound_check(val)			\
do {						\
	if (val > DGAIN_MAX && val < DGAIN_MIN)	\
		val = 0x00;			\
} while (0)

static unsigned int dreg[] = {
	TOMTOM_A_CDC_RX1_VOL_CTL_B2_CTL,
	TOMTOM_A_CDC_RX2_VOL_CTL_B2_CTL,
	TOMTOM_A_CDC_TX7_VOL_CTL_GAIN,
	TOMTOM_A_CDC_TX5_VOL_CTL_GAIN
};

static unsigned int dreg_reset[] = {
	TOMTOM_A_CDC_RX1_VOL_CTL_B2_CTL__POR,
	TOMTOM_A_CDC_RX2_VOL_CTL_B2_CTL__POR,
	TOMTOM_A_CDC_TX7_VOL_CTL_GAIN__POR,
	TOMTOM_A_CDC_TX5_VOL_CTL_GAIN__POR
};

static unsigned int areg[] = {
	TOMTOM_A_RX_HPH_L_GAIN,
	TOMTOM_A_RX_HPH_R_GAIN
};

#define DTCR	(ARRAY_SIZE(dreg))

enum dgdefs {
	HPGL_DEF = 0xFE,
	HPGR_DEF = 0xFE,
	MICG_DEF = 0x00,
	CMICG_DEF = 0x00,
	GAIN_DEF = 0x00
};

static struct snd_soc_codec *codec = NULL;

static struct tomtom_control {
	unsigned int enabled;
	int dgain[DTCR];
	int sgain[DTCR];
} *ctl;

static inline void ctl_write(unsigned int reg, int val)
{
	if (tomtom_write(codec, reg, val) < 0)
		pr_err("ctl_write failed!\n");
}

static inline int ctl_read(unsigned int reg)
{
	int ret;

	if ((ret = tomtom_read(codec, reg)) < 0)
		pr_err("ctl_read failed!\n");

	return ret;
}

static void ctl_init(void)
{
	ctl->enabled = 0;

	ctl->dgain[0] = ctl->sgain[0] = HPGL_DEF;
	ctl->dgain[1] = ctl->sgain[1] = HPGR_DEF;
	ctl->dgain[2] = ctl->sgain[2] = MICG_DEF;
	ctl->dgain[3] = ctl->sgain[3] = CMICG_DEF;

	pr_info("Gains initialised!\n");
}

static void store_gains(void)
{
	int i;

	for (i = 0; i < DTCR; i++)
		ctl->sgain[i] = ctl->dgain[i];

	pr_info("Gains stored!\n");
}

static void restore_gains(void)
{
	int i;

	for (i = 0; i < DTCR; i++)
		ctl->dgain[i] = ctl->sgain[i];

	pr_info("Gains restored!\n");
}

static void reset_gains(void)
{
	int i;

	for (i = 0; i < DTCR; i++)
		ctl->dgain[i] = GAIN_DEF;

	pr_info("Gains resetted!\n");
}

static void reset_regvals(void)
{
	int i;

	for (i = 0; i < DTCR; i++)
		ctl_write(dreg[i], dreg_reset[i]);

	pr_info("Regvals resetted!\n");
}

static void restore_regvals(void)
{
	int i;

	for (i = 0; i < DTCR; i++)
		ctl_write(dreg[i], ctl->dgain[i]);

	pr_info("Regvals restored!\n");
}

/********************************* Sysfs Start *******************************/
static ssize_t enabled_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", ctl->enabled);
}

static ssize_t enabled_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int val;
	int ret;

	ret = sscanf(buf, "%u", &val);
	if (ret != 1 || val > 1)
		return -EINVAL;

	switch ((ctl->enabled = val)) {
	case 0:
		store_gains();
		reset_gains();
		reset_regvals();
		pr_info("Disabled!\n");
		break;
	case 1:
		restore_gains();
		restore_regvals();
		pr_info("Enabled!\n");
		break;
	}

	return count;
}

static ssize_t dhpgain_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "L:0x%02X\tR:0x%02X\n", ctl->dgain[0],
								ctl->dgain[1]);
}

static ssize_t dhpgain_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int val[2];
	int i, ret;

	if (!ctl->enabled)
		return count;

	ret = sscanf(buf, "0x%X 0x%X", &val[0], &val[1]);
	if (ret < 1 || ret > 2)
		return -EINVAL;

	for (i = 0; i < (DTCR >> 1); i++) {
		bound_check(val[i]);
		ctl->dgain[i] = val[i];
		ctl_write(dreg[i], ctl->dgain[i]);
	}

	return count;
}

static ssize_t ahphgain_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "L:0x%02X\tR:0x%02X\n", ctl_read(areg[0]),
								ctl_read(areg[1]));
}

static ssize_t dmicgain_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%02X\n", ctl->dgain[2]);
}

static ssize_t dmicgain_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int val;
	int ret;

	if (!ctl->enabled)
		return count;

	ret = sscanf(buf, "0x%X", &val);
	if (ret != 1)
		return -EINVAL;

	bound_check(val);
	ctl->dgain[2] = val;

	ctl_write(dreg[2], ctl->dgain[2]);

	return count;
}

static ssize_t dcmicgain_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%02X\n", ctl->dgain[3]);
}

static ssize_t dcmicgain_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int val;
	int ret;

	if (!ctl->enabled)
		return count;

	ret = sscanf(buf, "0x%X", &val);
	if (ret != 1)
		return -EINVAL;

	bound_check(val);
	ctl->dgain[3] = val;

	ctl_write(dreg[3], ctl->dgain[3]);

	return count;
}

static DEVICE_ATTR(enabled, S_IRUGO | S_IWUSR,
					enabled_show,
					enabled_store);
static DEVICE_ATTR(dhpgain, S_IRUGO | S_IWUSR,
					dhpgain_show,
					dhpgain_store);
static DEVICE_ATTR(ahphgain, S_IRUGO,
					ahphgain_show,
					NULL);
static DEVICE_ATTR(dmicgain, S_IRUGO | S_IWUSR,
					dmicgain_show,
					dmicgain_store);
static DEVICE_ATTR(dcmicgain, S_IRUGO | S_IWUSR,
					dcmicgain_show,
					dcmicgain_store);

static struct attribute *tomtom_control_attributes[] = {
	&dev_attr_enabled.attr,
	&dev_attr_dhpgain.attr,
	&dev_attr_ahphgain.attr,
	&dev_attr_dmicgain.attr,
	&dev_attr_dcmicgain.attr,
	NULL
};

static struct attribute_group tomtom_control_group = {
	.attrs = tomtom_control_attributes,
};

static struct miscdevice tomtom_control_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "tomtom_control",
};
/********************************* Sysfs End *********************************/

void tomtom_control_probe(struct snd_soc_codec *codec_ptr)
{
	codec = codec_ptr;

	pr_info("Probed!\n");
}

static int tomtom_control_init(void)
{
	ctl = kmalloc(sizeof(*ctl), GFP_KERNEL);
	if (ctl == NULL) {
		pr_err("Failed to allocate memory for ctl!\n");
		return -ENOMEM;
	}

	misc_register(&tomtom_control_device);

	if (sysfs_create_group(&tomtom_control_device.this_device->kobj,
				&tomtom_control_group) < 0) {
		pr_err("Failed to create sysfs group!\n");
		return -ENOMEM;
	}

	ctl_init();
	reset_gains();

	pr_info("Initialized!\n");

	return 0;
}


static void tomtom_control_exit(void)
{
	sysfs_remove_group(&tomtom_control_device.this_device->kobj,
						&tomtom_control_group);

	misc_deregister(&tomtom_control_device);

	kfree(ctl);
}

module_init(tomtom_control_init);
module_exit(tomtom_control_exit);
