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

enum def_gains {
	HPGL_DEF = -2,
	HPGR_DEF = -2,
	MICG_DEF = 0,
	CMICG_DEF = 0,
	REGVAL_DEF = 0,
	GAIN_MIN = -20,
	GAIN_MAX = 20
};

enum tomtom_regs {
	HPGL_REG = TOMTOM_A_CDC_RX1_VOL_CTL_B2_CTL,	/* HPL */
	HPGR_REG = TOMTOM_A_CDC_RX2_VOL_CTL_B2_CTL,	/* HPR */
	MICG_REG = TOMTOM_A_CDC_TX7_VOL_CTL_GAIN,
	CMICG_REG = TOMTOM_A_CDC_TX5_VOL_CTL_GAIN
};

static struct snd_soc_codec *codec;

static unsigned int enabled;

static int hpgain_l, hpgain_r, micgain, cmicgain;
static int hpgain_l_s, hpgain_r_s, micgain_s, cmicgain_s;

void codec_probe(struct snd_soc_codec *codec_ptr)
{
	codec = codec_ptr;

	pr_info("Probed!\n");
}

static void init_gains(void)
{
	hpgain_l_s = hpgain_l = HPGL_DEF;
	hpgain_r_s = hpgain_r = HPGR_DEF;
	micgain_s = micgain = MICG_DEF;
	cmicgain_s = cmicgain = CMICG_DEF;

	pr_info("Gains initialised!\n");
}

static void store_gains(void)
{
	hpgain_l_s = hpgain_l;
	hpgain_r_s = hpgain_r;
	micgain_s = micgain;
	cmicgain_s = cmicgain;

	pr_info("Gains stored!\n");
}

static void restore_gains(void)
{
	hpgain_l = hpgain_l_s;
	hpgain_r = hpgain_r_s;
	micgain = micgain_s;
	cmicgain = cmicgain_s;

	pr_info("Gains restored!\n");
}

static void reset_gains(void)
{
	hpgain_l = hpgain_r = micgain = cmicgain = REGVAL_DEF;

	pr_info("Gains resetted!\n");
}

static void reset_regvals(void)
{
	tomtom_write(codec, HPGL_REG, REGVAL_DEF);
	tomtom_write(codec, HPGR_REG, REGVAL_DEF);
	tomtom_write(codec, MICG_REG, REGVAL_DEF);
	tomtom_write(codec, CMICG_REG, REGVAL_DEF);

	pr_info("Regvals resetted!\n");
}

static void restore_regvals(void)
{
	tomtom_write(codec, HPGL_REG, hpgain_l);
	tomtom_write(codec, HPGR_REG, hpgain_r);
	tomtom_write(codec, MICG_REG, micgain);
	tomtom_write(codec, CMICG_REG, cmicgain);

	pr_info("Regvals restored!\n");
}

/********************************* Sysfs Start *******************************/
static ssize_t enabled_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", enabled);
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

	enabled = val;

	switch (enabled) {
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

static ssize_t hpgain_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "L:%d R:%d\n",
						hpgain_l,
						hpgain_r);
}

static ssize_t hpgain_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int ret, val_l, val_r;

	if (!enabled)
		return count;

	ret = sscanf(buf, "%d %d", &val_l, &val_r);
	if (ret < 1 || ret > 2)
		return -EINVAL;

	if (val_l > GAIN_MAX)
		val_l = GAIN_MAX;
	else if (val_l < GAIN_MIN)
		val_l = GAIN_MIN;
	else if (val_r > GAIN_MAX)
		val_r = GAIN_MAX;
	else if (val_r < GAIN_MIN)
		val_r = GAIN_MIN;

	hpgain_l = val_l;
	hpgain_r = val_r;

	tomtom_write(codec, HPGL_REG, hpgain_l);
	tomtom_write(codec, HPGR_REG, hpgain_r);

	return count;
}

static ssize_t micgain_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", micgain);
}

static ssize_t micgain_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int ret, val;

	ret = sscanf(buf, "%d", &val);
	if (ret != 1)
		return -EINVAL;

	if (val > GAIN_MAX)
		val = GAIN_MAX;
	else if (val < GAIN_MIN)
		val = GAIN_MIN;

	micgain = val;
	
	tomtom_write(codec, MICG_REG, micgain);

	return count;
}

static ssize_t cmicgain_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", cmicgain);
}

static ssize_t cmicgain_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int ret, val;

	ret = sscanf(buf, "%d", &val);
	if (ret != 1)
		return -EINVAL;

	if (val > GAIN_MAX)
		val = GAIN_MAX;
	else if (val < GAIN_MIN)
		val = GAIN_MIN;

	cmicgain = val;
	
	tomtom_write(codec, CMICG_REG, cmicgain);

	return count;
}

static DEVICE_ATTR(enabled, S_IRUGO | S_IWUSR,
					enabled_show,
					enabled_store);
static DEVICE_ATTR(hpgain, S_IRUGO | S_IWUSR,
					hpgain_show,
					hpgain_store);
static DEVICE_ATTR(micgain, S_IRUGO | S_IWUSR,
					micgain_show,
					micgain_store);
static DEVICE_ATTR(cmicgain, S_IRUGO | S_IWUSR,
					cmicgain_show,
					cmicgain_store);

static struct attribute *tomtom_control_attributes[] = {
	&dev_attr_enabled.attr,
	&dev_attr_hpgain.attr,
	&dev_attr_micgain.attr,
	&dev_attr_cmicgain.attr,
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

static int tomtom_control_init(void)
{
	misc_register(&tomtom_control_device);

	if (sysfs_create_group(&tomtom_control_device.this_device->kobj,
				&tomtom_control_group) < 0) {
		pr_err("Failed to create sysfs group!\n");
		return -ENOMEM;
	}

	init_gains();
	reset_gains();

	pr_info("Initialized!\n");

	return 0;
}


static void tomtom_control_exit(void)
{
	sysfs_remove_group(&tomtom_control_device.this_device->kobj,
						&tomtom_control_group);
}

module_init(tomtom_control_init);
module_exit(tomtom_control_exit);
