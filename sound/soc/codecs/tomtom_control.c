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

enum gains {
	HPGL_DEF = -2,
	HPGR_DEF = -2,
	MICG_DEF = 0,
	CMICG_DEF = 0,
	REGVAL_DEF = 0,
	GAIN_MIN = -20,
	GAIN_MAX = 20
};

#define bound_check(val) 		\
	if (val > GAIN_MAX)		\
		val = GAIN_MAX;		\
	else if (val < GAIN_MIN)	\
		val = GAIN_MIN;

enum regs {
	HPGL_REG = TOMTOM_A_CDC_RX1_VOL_CTL_B2_CTL,
	HPGR_REG = TOMTOM_A_CDC_RX2_VOL_CTL_B2_CTL,
	MICG_REG = TOMTOM_A_CDC_TX7_VOL_CTL_GAIN,
	CMICG_REG = TOMTOM_A_CDC_TX5_VOL_CTL_GAIN
};

static struct snd_soc_codec *codec = NULL;

static struct tomtom_control {
	unsigned int enabled;
	int hpgain_l;
	int hpgain_r;
	int micgain;
	int cmicgain;
	int hpgain_l_s;
	int hpgain_r_s;
	int micgain_s;
	int cmicgain_s;
} *ctl;

static inline int ctl_write(unsigned int reg, int val)
{
	if (tomtom_write(codec, reg, val) == -ENODEV)
		return -EINVAL;

	return 0;
}

static void ctl_init(void)
{
	ctl->enabled = 0;
	ctl->hpgain_l_s = ctl->hpgain_l = HPGL_DEF;
	ctl->hpgain_r_s = ctl->hpgain_r = HPGR_DEF;
	ctl->micgain_s = ctl->micgain = MICG_DEF;
	ctl->cmicgain_s = ctl->cmicgain = CMICG_DEF;

	pr_info("Gains initialised!\n");
}

static void store_gains(void)
{
	ctl->hpgain_l_s = ctl->hpgain_l;
	ctl->hpgain_r_s = ctl->hpgain_r;
	ctl->micgain_s = ctl->micgain;
	ctl->cmicgain_s = ctl->cmicgain;

	pr_info("Gains stored!\n");
}

static void restore_gains(void)
{
	ctl->hpgain_l = ctl->hpgain_l_s;
	ctl->hpgain_r = ctl->hpgain_r_s;
	ctl->micgain = ctl->micgain_s;
	ctl->cmicgain = ctl->cmicgain_s;

	pr_info("Gains restored!\n");
}

static void reset_gains(void)
{
	ctl->hpgain_l = ctl->hpgain_r = ctl->micgain = ctl->cmicgain = REGVAL_DEF;

	pr_info("Gains resetted!\n");
}

static void reset_regvals(void)
{
	if (ctl_write(HPGL_REG, REGVAL_DEF) == -EINVAL)
		pr_err("Failed to write %d to HPGL_REG\n", REGVAL_DEF);
	if (ctl_write(HPGR_REG, REGVAL_DEF) == -EINVAL)
		pr_err("Failed to write %d to HPGR_REG\n", REGVAL_DEF);
	if (ctl_write(MICG_REG, REGVAL_DEF) == -EINVAL)
		pr_err("Failed to write %d to MICG_REG\n", REGVAL_DEF);
	if (ctl_write(CMICG_REG, REGVAL_DEF) == -EINVAL)
		pr_err("Failed to write %d to CMICG_REG\n", REGVAL_DEF);

	pr_info("Regvals resetted!\n");
}

static void restore_regvals(void)
{
	if (ctl_write(HPGL_REG, ctl->hpgain_l) == -EINVAL)
		pr_err("Failed to write %d to HPGL_REG\n", ctl->hpgain_l);
	if (ctl_write(HPGR_REG, ctl->hpgain_r) == -EINVAL)
		pr_err("Failed to write %d to HPGR_REG\n", ctl->hpgain_r);
	if (ctl_write(MICG_REG, ctl->micgain) == -EINVAL)
		pr_err("Failed to write %d to MICG_REG\n", ctl->micgain);
	if (ctl_write(CMICG_REG, ctl->cmicgain) == -EINVAL)
		pr_err("Failed to write %d to CMICG_REG\n", ctl->cmicgain);

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

	ctl->enabled = val;

	switch (ctl->enabled) {
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
	return snprintf(buf, PAGE_SIZE, "L:\t%d R:\t%d\n",
						ctl->hpgain_l,
						ctl->hpgain_r);
}

static ssize_t hpgain_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int ret, val_l, val_r;

	if (!ctl->enabled)
		return count;

	ret = sscanf(buf, "%d %d", &val_l, &val_r);
	if (ret < 1 || ret > 2)
		return -EINVAL;

	bound_check(val_l);
	bound_check(val_r);

	ctl->hpgain_l = val_l;
	ctl->hpgain_r = val_r;

	if (ctl_write(HPGL_REG, ctl->hpgain_l) == -EINVAL)
		pr_err("Failed to write %d to HPGL_REG\n", ctl->hpgain_l);
	if (ctl_write(HPGR_REG, ctl->hpgain_r) == -EINVAL)
		pr_err("Failed to write %d to HPGR_REG\n", ctl->hpgain_r);

	return count;
}

static ssize_t micgain_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", ctl->micgain);
}

static ssize_t micgain_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int ret, val;

	if (!ctl->enabled)
		return count;

	ret = sscanf(buf, "%d", &val);
	if (ret != 1)
		return -EINVAL;

	bound_check(val);

	ctl->micgain = val;
	
	if (ctl_write(MICG_REG, ctl->micgain) == -EINVAL)
		pr_err("Failed to write %d to MICG_REG\n", ctl->micgain);

	return count;
}

static ssize_t cmicgain_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", ctl->cmicgain);
}

static ssize_t cmicgain_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int ret, val;

	if (!ctl->enabled)
		return count;

	ret = sscanf(buf, "%d", &val);
	if (ret != 1)
		return -EINVAL;

	bound_check(val);

	ctl->cmicgain = val;
	
	if (ctl_write(CMICG_REG, ctl->cmicgain) == -EINVAL)
		pr_err("Failed to write %d to CMICG_REG\n", ctl->cmicgain);

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

void tomtom_control_probe(struct snd_soc_codec *codec_ptr)
{
	codec = codec_ptr;

	pr_info("Probed!\n");
}

static int tomtom_control_init(void)
{
	ctl = kmalloc(sizeof(ctl), GFP_KERNEL);
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
