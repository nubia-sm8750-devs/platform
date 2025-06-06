// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2019, 2021 The Linux Foundation. All rights reserved.
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/pm.h>
#include <linux/of_address.h>
#include <linux/nvmem-consumer.h>
#include <linux/kprobes.h>
#include <linux/panic_notifier.h>

/* Started by AICoder, pid:kf1c3sd0528e0e0141ff0a3890de450942885551 */
struct zte_reboot_ext {
	struct device *dev;
	struct kobject kobj;
	struct notifier_block panic_nb;
	struct nvmem_cell *vendor_zlog_ss_nvmem_cell;
	struct nvmem_cell *vendor_zlog_w_nvmem_cell;
	struct nvmem_cell *vendor_zlog_p_nvmem_cell;
};
/* Ended by AICoder, pid:kf1c3sd0528e0e0141ff0a3890de450942885551 */

/* Started by AICoder, pid:236b9db0e7l75b514db508ef90c97f12a501bb52 */
#define SDAM_SPACE_LEN_DEFAULT 1

// read for user space read
#define READ_PANIC_OFFSET 3  // offset for panic
#define READ_SUBSYS_OFFSET 2  // offset for subsystem
#define READ_WDT_OFFSET 1     // offset for watchdog

// read and write in pmic offset, and will change if qcom dtsi changed
#define SAVED_PANIC_OFFSET 3  // offset for panic
#define SAVED_WDT_OFFSET 1    // offset for ap watchdog
#define SAVED_SUBSYS_OFFSET 0  // offset for subsystem
/* Ended by AICoder, pid:236b9db0e7l75b514db508ef90c97f12a501bb52 */

/* Started by AICoder, pid:v93bc4b8c76e5261420c08e0c05b8e19bfb4e0c4 */
#define SDAM_SPACE_LEN 4  // fixed line

#define STUB_SAVED_STR "1234"  // fixed init val, not possible used
#define STUB_READ_STR "5678"  // fixed init val, not possible used

#define CONST_TAG_M 'M'  // single char for modem
#define CONST_TAG_P 'P'  // single char for ap panic

#define CONST_TAG_A 'A'	   // single char for adsp
#define CONST_TAG_C 'C'	   // single char for cdsp
#define CONST_TAG_S 'S'	   // single char for slpi
/* Ended by AICoder, pid:v93bc4b8c76e5261420c08e0c05b8e19bfb4e0c4 */


/* Started by AICoder, pid:s1c3700153eebd9145de095400ee0b0c98a8b78c */
u8 saved_nvmem_buf[SDAM_SPACE_LEN] = {0x31, 0x32, 0x33, 0x34};  // char 1234
u8 read_nvmem_buf[SDAM_SPACE_LEN] = {0x35, 0x36, 0x37, 0x38};  // char 5678
/* Ended by AICoder, pid:s1c3700153eebd9145de095400ee0b0c98a8b78c */

/* Started by AICoder, pid:6493475094p7f9d14f010973f0fdbc13b836cf63 */
static int entry_panic(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	const char *fmtstr = (const char *)regs->regs[0];

	if (fmtstr != NULL) {
		pr_info("ztedbg panic_hook: %s\n", fmtstr);
	}

	saved_nvmem_buf[SAVED_PANIC_OFFSET] = CONST_TAG_P;

	pr_info("ztedbg panic_hook: %x %x %x %x\n", saved_nvmem_buf[0], saved_nvmem_buf[1], saved_nvmem_buf[2], saved_nvmem_buf[3]);

	return 0;
}
/* Ended by AICoder, pid:6493475094p7f9d14f010973f0fdbc13b836cf63 */


/* Started by AICoder, pid:bfdc3m9014we9c11483708bf1083e517b0160e76 */
struct kretprobe panic_probe = {
	.entry_handler = entry_panic,
	.maxactive = 1,
	.kp.symbol_name = "panic",
};
/* Ended by AICoder, pid:bfdc3m9014we9c11483708bf1083e517b0160e76 */


/* Started by AICoder, pid:xced3q5079n670814ba70b17405b9e197d205172 */
static void register_panic_hook(struct platform_device *pdev)
{
	int ret;

	ret = register_kretprobe(&panic_probe);  // only one panic hook is allowed
	if (ret)
		dev_err(&pdev->dev, "ztedbg failed to register p_hook: %d\n", ret);
	else
		dev_info(&pdev->dev, "ztedbg register p_hook\n");
}
/* Ended by AICoder, pid:xced3q5079n670814ba70b17405b9e197d205172 */

static void unregister_panic_hook(void)
{
	unregister_kretprobe(&panic_probe);  // no matter if not found
	pr_info("ztedbg unregister p_hook");
}

/* Started by AICoder, pid:t33bb0c8cciacd214a4f0a69b054963969823c3a */
static void save_panic_buf_data_to_nvmem(struct zte_reboot_ext *reboot)
{
	int ret;

	if (reboot != NULL) {
		if (IS_ERR(reboot->vendor_zlog_p_nvmem_cell)) {
			ret = PTR_ERR(reboot->vendor_zlog_p_nvmem_cell);
			pr_err("ztedbg invalid vendor_zlog_p %d\n", ret);
		} else {
			pr_info("ztedbg write vendor_zlog_p: 0x%x\n", saved_nvmem_buf[SAVED_PANIC_OFFSET]);
			nvmem_cell_write(reboot->vendor_zlog_p_nvmem_cell, &saved_nvmem_buf[SAVED_PANIC_OFFSET], SDAM_SPACE_LEN_DEFAULT);
		}

		if (IS_ERR(reboot->vendor_zlog_w_nvmem_cell)) {
			ret = PTR_ERR(reboot->vendor_zlog_w_nvmem_cell);
			pr_err("ztedbg invalid vendor_zlog_w %d\n", ret);
		} else {
			pr_info("ztedbg write vendor_zlog_w: 0x%x\n", saved_nvmem_buf[SAVED_WDT_OFFSET]);
			nvmem_cell_write(reboot->vendor_zlog_w_nvmem_cell, &saved_nvmem_buf[SAVED_WDT_OFFSET], SDAM_SPACE_LEN_DEFAULT);
		}

		if (IS_ERR(reboot->vendor_zlog_ss_nvmem_cell)) {
			ret = PTR_ERR(reboot->vendor_zlog_ss_nvmem_cell);
			pr_err("ztedbg invalid vendor_zlog_ss %d\n", ret);
		} else {
			pr_info("ztedbg write vendor_zlog_ss: 0x%x\n", saved_nvmem_buf[SAVED_SUBSYS_OFFSET]);
			nvmem_cell_write(reboot->vendor_zlog_ss_nvmem_cell, &saved_nvmem_buf[SAVED_SUBSYS_OFFSET], SDAM_SPACE_LEN_DEFAULT);
		}
	} else {
		pr_err("ztedbg NULL reboot struct in panic save");
	}
}
/* Ended by AICoder, pid:t33bb0c8cciacd214a4f0a69b054963969823c3a */

extern u8 get_ss_panic_buf_byte(void);  // defined in qcom_q6v5 ko
static int zte_reboot_ext_panic(struct notifier_block *this, unsigned long event, void *ptr)
{
	struct zte_reboot_ext *reboot = container_of(this, struct zte_reboot_ext, panic_nb);

	saved_nvmem_buf[SAVED_SUBSYS_OFFSET] = get_ss_panic_buf_byte();  // update possible subsystem panic symbol
	save_panic_buf_data_to_nvmem(reboot);

	return NOTIFY_OK;
}

/* Started by AICoder, pid:z3da8rb8e8o09df14bc1091390de4445ff34480c */
/* interface for exporting attributes */
struct bootreason_attribute {
	struct attribute		attr;
	ssize_t (*show)(struct kobject *kobj, struct attribute *attr,
					 char *buf);
	ssize_t (*store)(struct kobject *kobj, struct attribute *attr,
					 const char *buf, size_t count);
};

#define to_bootreason_attr(_attr) \
	container_of(_attr, struct bootreason_attribute, attr)

static ssize_t attr_show(struct kobject *kobj, struct attribute *attr,
						 char *buf)
{
	struct bootreason_attribute *bootreason_attr = to_bootreason_attr(attr);
	ssize_t ret = -EIO;

	if (bootreason_attr->show)
		ret = bootreason_attr->show(kobj, attr, buf);

	return ret;
}

static ssize_t attr_store(struct kobject *kobj, struct attribute *attr,
						 const char *buf, size_t count)
{
	struct bootreason_attribute *bootreason_attr = to_bootreason_attr(attr);
	ssize_t ret = -EIO;

	if (bootreason_attr->store)
		ret = bootreason_attr->store(kobj, attr, buf, count);

	return ret;
}

static const struct sysfs_ops bootreason_sysfs_ops = {
	.show   = attr_show,
	.store  = attr_store,
};

static struct kobj_type bootreason_nvmem_kobj_type = {
	.sysfs_ops	= &bootreason_sysfs_ops,
};
/* Ended by AICoder, pid:z3da8rb8e8o09df14bc1091390de4445ff34480c */

/* Started by AICoder, pid:p9720tca66v67b4148610adc802f9e275ea4b3c9 */
static ssize_t boot_nvmem_show(struct kobject *kobj, struct attribute *this, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%c%c%c%c\n",
		read_nvmem_buf[0], read_nvmem_buf[1], read_nvmem_buf[2], read_nvmem_buf[3]);
}

static ssize_t boot_nvmem_store(struct kobject *kobj, struct attribute *this, const char *buf, size_t count)
{
	pr_err("ztedeg not support set request\n");
	return -EINVAL;
}

static struct bootreason_attribute attr_boot_nvmem = __ATTR_RW(boot_nvmem);

static struct attribute *qcom_boot_nvmem_attrs[] = {
	&attr_boot_nvmem.attr,
	NULL
};

static struct attribute_group qcom_boot_nvmem_attr_group = {
	.attrs = qcom_boot_nvmem_attrs,
};
/* Ended by AICoder, pid:p9720tca66v67b4148610adc802f9e275ea4b3c9 */


static int zte_reboot_ext_probe(struct platform_device *pdev)
{
	struct zte_reboot_ext *reboot;
	u8 *buf;
	int ret;
	size_t len;

	reboot = devm_kzalloc(&pdev->dev, sizeof(*reboot), GFP_KERNEL);
	if (!reboot)
		return -ENOMEM;

	reboot->dev = &pdev->dev;

/* Started by AICoder, pid:79cc558e2bv96e914b260ad1301b071080a42b9b */
	ret = kobject_init_and_add(&reboot->kobj, &bootreason_nvmem_kobj_type,
			   kernel_kobj, "bootreason");
	if (ret) {
		pr_err("%s: Error in creation kobject_add\n", __func__);
		kobject_put(&reboot->kobj);
		return ret;
	}

	ret = sysfs_create_group(&reboot->kobj, &qcom_boot_nvmem_attr_group);
	if (ret) {
		pr_err("%s: Error in creation sysfs_create_group\n", __func__);
		kobject_del(&reboot->kobj);
		return ret;
	}
/* Ended by AICoder, pid:79cc558e2bv96e914b260ad1301b071080a42b9b */

/* Started by AICoder, pid:5d908b58c5n8b5d142c909b9a0330519f9e958e5 */
	reboot->vendor_zlog_ss_nvmem_cell = nvmem_cell_get(reboot->dev, "vendor_zlog_ss");
	if (IS_ERR(reboot->vendor_zlog_ss_nvmem_cell)) {
		ret = PTR_ERR(reboot->vendor_zlog_ss_nvmem_cell);
		pr_err("ztedbg failed to get vendor_zlog_ss %d\n", ret);
	} else {
		buf = nvmem_cell_read(reboot->vendor_zlog_ss_nvmem_cell, &len);
		if (IS_ERR(buf)) {
			ret = PTR_ERR(buf);
			pr_err("ztedbg failed to read vendor_zlog_ss %d\n", ret);
		} else {
			if (len >= SDAM_SPACE_LEN_DEFAULT) {
				read_nvmem_buf[READ_SUBSYS_OFFSET] = buf[0];
				pr_info("ztedbg read 1 bytes vendor_zlog_ss: 0x%x\n", read_nvmem_buf[READ_SUBSYS_OFFSET]);
			} else {
				pr_err("ztedbg unexpected vendor_zlog_ss len %zu\n", len);
			}
			kfree(buf);
		}
	}
/* Ended by AICoder, pid:5d908b58c5n8b5d142c909b9a0330519f9e958e5 */

/* Started by AICoder, pid:hf3112d5d9u7b7c14aff0a2f1056a914d6f9f484 */
	reboot->vendor_zlog_w_nvmem_cell = nvmem_cell_get(reboot->dev, "vendor_zlog_w");
	if (IS_ERR(reboot->vendor_zlog_w_nvmem_cell)) {
		ret = PTR_ERR(reboot->vendor_zlog_w_nvmem_cell);
		pr_err("ztedbg failed to get vendor_zlog_w %d\n", ret);
	} else {
		buf = nvmem_cell_read(reboot->vendor_zlog_w_nvmem_cell, &len);
		if (IS_ERR(buf)) {
			ret = PTR_ERR(buf);
			pr_err("ztedbg failed to read vendor_zlog_w %d\n", ret);
		} else {
			if (len >= SDAM_SPACE_LEN_DEFAULT) {
				read_nvmem_buf[READ_WDT_OFFSET] = buf[0];
				pr_info("ztedbg read 1 bytes vendor_zlog_w: 0x%x\n", read_nvmem_buf[READ_WDT_OFFSET]);
			} else {
				pr_err("ztedbg unexpected vendor_zlog_w len %zu\n", len);
			}
			kfree(buf);
		}
	}
/* Ended by AICoder, pid:hf3112d5d9u7b7c14aff0a2f1056a914d6f9f484 */

/* Started by AICoder, pid:vecbd28f63o507b14c850a98b02bf21561090a22 */
	reboot->vendor_zlog_p_nvmem_cell = nvmem_cell_get(reboot->dev, "vendor_zlog_p");
	if (IS_ERR(reboot->vendor_zlog_p_nvmem_cell)) {
		ret = PTR_ERR(reboot->vendor_zlog_p_nvmem_cell);
		pr_err("ztedbg failed to get vendor_zlog_p %d\n", ret);
	} else {
		buf = nvmem_cell_read(reboot->vendor_zlog_p_nvmem_cell, &len);
		if (IS_ERR(buf)) {
			ret = PTR_ERR(buf);
			pr_err("ztedbg failed to read vendor_zlog_p %d\n", ret);
		} else {
			if (len >= SDAM_SPACE_LEN_DEFAULT) {
				read_nvmem_buf[READ_PANIC_OFFSET] = buf[0];
				pr_info("ztedbg read 1 bytes vendor_zlog_p: 0x%x\n", read_nvmem_buf[READ_PANIC_OFFSET]);
			} else {
				pr_err("ztedbg unexpected vendor_zlog_p len %zu\n", len);
			}
			kfree(buf);
		}
	}
/* Ended by AICoder, pid:vecbd28f63o507b14c850a98b02bf21561090a22 */


/* Started by AICoder, pid:i148ae33cch9df014de60882706b3a0ba052d6e7 */
	// write pmic reg using default digital numbers in pmic saved buffer
	save_panic_buf_data_to_nvmem(reboot);
/* Ended by AICoder, pid:i148ae33cch9df014de60882706b3a0ba052d6e7 */

	register_panic_hook(pdev);

	reboot->panic_nb.notifier_call = zte_reboot_ext_panic;
	reboot->panic_nb.priority = INT_MAX;
	atomic_notifier_chain_register(&panic_notifier_list, &reboot->panic_nb);

	platform_set_drvdata(pdev, reboot);

	return 0;
}

static int zte_reboot_ext_remove(struct platform_device *pdev)
{
	struct zte_reboot_ext *reboot = platform_get_drvdata(pdev);

	atomic_notifier_chain_unregister(&panic_notifier_list, &reboot->panic_nb);
	unregister_panic_hook();

	return 0;
}

static const struct of_device_id of_zte_reboot_ext_match[] = {
	{ .compatible = "zte,reboot-ext", },
	{},
};
MODULE_DEVICE_TABLE(of, of_zte_reboot_ext_match);

static struct platform_driver zte_reboot_ext_driver = {
	.probe = zte_reboot_ext_probe,
	.remove = zte_reboot_ext_remove,
	.driver = {
		.name = "zte-reboot-ext",
		.of_match_table = of_match_ptr(of_zte_reboot_ext_match),
	},
};

module_platform_driver(zte_reboot_ext_driver);

MODULE_DESCRIPTION("ZTE Reboot Ext Driver");
MODULE_LICENSE("GPL v2");
