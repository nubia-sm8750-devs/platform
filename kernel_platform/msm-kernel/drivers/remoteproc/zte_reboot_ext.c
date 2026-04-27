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
#include <linux/ctype.h>

/* Started by AICoder, pid:kf1c3sd0528e0e0141ff0a3890de450942885551 */
struct zte_reboot_ext {
	struct device *dev;
	struct kobject kobj;
	struct notifier_block panic_nb;
	struct nvmem_cell *vendor_zlog_ss_nvmem_cell;
	struct nvmem_cell *vendor_zlog_w_nvmem_cell;
	struct nvmem_cell *vendor_zlog_p_nvmem_cell;
	struct nvmem_cell *vendor_zlog_panic_ext_nvmem_cell;
};
/* Ended by AICoder, pid:kf1c3sd0528e0e0141ff0a3890de450942885551 */

/* Started by AICoder, pid:236b9db0e7l75b514db508ef90c97f12a501bb52 */
#define SDAM_SPACE_LEN_DEFAULT 1
#define SDAM_SPACE_LEN_PANIC_EXT 8

// read for user space read
#define READ_PANIC_EXT_OFFSET 4
#define READ_PANIC_OFFSET 3  // offset for panic
#define READ_SUBSYS_OFFSET 2  // offset for subsystem
#define READ_WDT_OFFSET 1  // offset for watchdog

// read and write in pmic offset, and will change if qcom dtsi changed
#define SAVED_PANIC_EXT_OFFSET 4
#define SAVED_PANIC_OFFSET 3  // offset for panic
#define SAVED_WDT_OFFSET 1  // offset for ap watchdog
#define SAVED_SUBSYS_OFFSET 0  // offset for subsystem
/* Ended by AICoder, pid:236b9db0e7l75b514db508ef90c97f12a501bb52 */

/* Started by AICoder, pid:v93bc4b8c76e5261420c08e0c05b8e19bfb4e0c4 */
#define SDAM_SPACE_LEN 12  // from 4 extend to 12 for panic ext

#define STUB_SAVED_STR "1234"  // fixed init val, not possible used
#define STUB_READ_STR "5678"  // fixed init val, not possible used

#define CONST_TAG_M 'M'  // single char for modem
#define CONST_TAG_P 'P'  // single char for ap panic

#define CONST_TAG_A 'A'	   // single char for adsp
#define CONST_TAG_C 'C'	   // single char for cdsp
#define CONST_TAG_S 'S'	   // single char for slpi
/* Ended by AICoder, pid:v93bc4b8c76e5261420c08e0c05b8e19bfb4e0c4 */


/* Started by AICoder, pid:s1c3700153eebd9145de095400ee0b0c98a8b78c */
u8 saved_nvmem_buf[SDAM_SPACE_LEN] = {0x31, 0x32, 0x33, 0x34, 0, 0, 0, 0, 0, 0, 0, 0};  // char 1234 and default pmic reg val
u8 read_nvmem_buf[SDAM_SPACE_LEN] = {0x35, 0x36, 0x37, 0x38, 0x35, 0x36, 0x37, 0x38, 0x35, 0x36, 0x37, 0x38};  // char 5678
/* Ended by AICoder, pid:s1c3700153eebd9145de095400ee0b0c98a8b78c */

#define INVALID_READ_STR "567856785678\n"

/* Started by AICoder, pid:h51b35ec31d3a6214bdf0b23704ed656ad73249e */
void fill_nvmem_buf(char *str, const char *fmtstr, unsigned char *saved_nvmem_buf, size_t offset, size_t length) {
	size_t str_len = str ? strlen(str) : 0;
	size_t fmtstr_len = fmtstr ? strlen(fmtstr) : 0;
	size_t filled = 0;
	size_t to_copy;

	// 优先从 str 填充
	if (str && str_len > 0) {
		to_copy = min(str_len, length);
		memcpy(&saved_nvmem_buf[offset], str, to_copy);
		filled += to_copy;
		if (filled >= length) {
			return;
		}
	}

	// 如果 str 不够长，从 fmtstr 填充
	if (fmtstr && fmtstr_len > 0) {
		to_copy = min(fmtstr_len, length - filled);
		memcpy(&saved_nvmem_buf[offset + filled], fmtstr, to_copy);
		filled += to_copy;
		if (filled >= length) {
			return;
		}
	}

	// 如果仍有剩余空间，用 0 填充
	if (filled < length) {
		memset(&saved_nvmem_buf[offset + filled], 0, length - filled);
	}
}
/* Ended by AICoder, pid:h51b35ec31d3a6214bdf0b23704ed656ad73249e */

// zte print more format
/* Started by AICoder, pid:40525h09eama9fc149910b97a0985966d469d1fc */
/**
 * @brief 计算格式化字符串中格式说明符的数量，并返回第一个 %s 的计数值。
 *
 * @param fmt 格式化字符串。
 * @param first_s_count 用于存储第一个 %s 计数值的指针。
 * @return int 返回格式说明符的数量。
 */
int count_format_args(const char *fmt, int *first_s_count) {
	int count = 0;
	bool in_format_specifier = false;
	const char *p;

	if (first_s_count == NULL) {  // not support
	return -1;
	}

	*first_s_count = -1;  // 初始化为-1，表示未找到

	if (fmt != NULL) {
		for (p = fmt; *p; p++) {
			if (*p == '%') {
				if (!in_format_specifier) {
					in_format_specifier = true;
					continue;
				} else {
					// 双百分号，跳过
					in_format_specifier = false;
				}
			}

			if (in_format_specifier) {
				// 检查是否为格式说明符的结束字符
				if (strchr("diouxXfFeEgGaAcsSpn", *p)) {
					count++;
					if (*p == 's' && *first_s_count == -1) {
						*first_s_count = count;  // 记录第一个 %s 的计数值
					}
					in_format_specifier = false;
				} else if (*p == '%') {
					// 双百分号，跳过
					in_format_specifier = false;
				} else if (*p == '*') {
					// 不支持与星号相关的复杂情况
					return -1;
				}
			}
		}
	}

	return count;
}
/* Ended by AICoder, pid:40525h09eama9fc149910b97a0985966d469d1fc */

#define ZTEDBG_HOOK_PREFIX "ztedbg panic_hook:"
#define ZTEDBG_HOOK_BUFLEN_MAX 256
int once_entry_panic_count = 0;
static int entry_panic(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	const char *fmtstr = (const char *)regs->regs[0];
	int arg_count = 0;
	int first_s_count = -1;  // default value when not set
	char buf[ZTEDBG_HOOK_BUFLEN_MAX];
	size_t prefix_len = strlen(ZTEDBG_HOOK_PREFIX);
	size_t fmt_len;
	char *str = NULL;

	once_entry_panic_count++;

	if (once_entry_panic_count == 1) {  // only deal for first time
		/* Started by AICoder, pid:39361na4bc6f4ee141c90a5950b5c092cfa16591 */
		if (fmtstr != NULL) {
			fmt_len = strlen(fmtstr);

			// 留出更多空间用于填充0和换行符
			if ((prefix_len + fmt_len + 3) <= ZTEDBG_HOOK_BUFLEN_MAX) {
				// 使用 snprintf 拼接字符串
				snprintf(buf, sizeof(buf), "%s%s", ZTEDBG_HOOK_PREFIX, fmtstr);

				arg_count = count_format_args(buf, &first_s_count);  // keep default count if not set
				if (first_s_count > 0 && first_s_count <= 7) {
					str = (char *)regs->regs[first_s_count];
					if (str) {
						pr_info("ztedbg panic_hook firstS :%s:\n", str);
						if (strcmp(str, "panicinpanic") == 0) {  // special for test abnormal case only
							panic("panicinpanic %d", once_entry_panic_count);
						}
					} else {
						pr_info("ztedbg panic_hook unexpected null firstS %d\n", first_s_count);
					}
				}

				switch (arg_count) {  // 最多支持3个参数时打印
					case 0:
						printk(buf);
						break;
					case 1:
						printk(buf, regs->regs[1]);
						break;
					case 2:
						printk(buf, regs->regs[1], regs->regs[2]);
						break;
					case 3:
						printk(buf, regs->regs[1], regs->regs[2], regs->regs[3]);
						break;
					default:
						pr_info("ztedbg panic_hook %d parameters: %s\n", arg_count, fmtstr);
						break;
				}
			} else {
				pr_info("ztedbg panic_hook %zu fmt: %s\n", prefix_len + fmt_len, fmtstr);
			}
		}
		/* Ended by AICoder, pid:39361na4bc6f4ee141c90a5950b5c092cfa16591 */

		saved_nvmem_buf[SAVED_PANIC_OFFSET] = CONST_TAG_P;
		// further fill panic ext
		fill_nvmem_buf(str, fmtstr, saved_nvmem_buf, READ_PANIC_EXT_OFFSET, SDAM_SPACE_LEN_PANIC_EXT);
		pr_info("ztedbg panic_hook entry s: %x %x %x %x %x %x %x %x %x %x %x %x\n",
			saved_nvmem_buf[0], saved_nvmem_buf[1], saved_nvmem_buf[2], saved_nvmem_buf[3],
			saved_nvmem_buf[4], saved_nvmem_buf[5], saved_nvmem_buf[6], saved_nvmem_buf[7],
			saved_nvmem_buf[8], saved_nvmem_buf[9], saved_nvmem_buf[10], saved_nvmem_buf[11]);
	} else {
		pr_info("ztedbg panic_hook skip for entry %d: %x %x %x %x %x %x %x %x %x %x %x %x\n", once_entry_panic_count,
				saved_nvmem_buf[0], saved_nvmem_buf[1], saved_nvmem_buf[2], saved_nvmem_buf[3],
				saved_nvmem_buf[4], saved_nvmem_buf[5], saved_nvmem_buf[6], saved_nvmem_buf[7],
				saved_nvmem_buf[8], saved_nvmem_buf[9], saved_nvmem_buf[10], saved_nvmem_buf[11]);
	}

	return 0;
}

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

/* Started by AICoder, pid:cc418279a0j36b214ad109f79097b92ecb91266a */
		// 检查 vendor_zlog_panic_ext_nvmem_cell 是否有效
		if (IS_ERR(reboot->vendor_zlog_panic_ext_nvmem_cell)) {
			ret = PTR_ERR(reboot->vendor_zlog_panic_ext_nvmem_cell);
			pr_err("ztedbg invalid w vendor_zlog_panic_ext %d\n", ret);
		} else {
			// 打印保存的 NVMEM 缓冲区内容
			pr_info("ztedbg write vendor_zlog_panic_ext: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
					saved_nvmem_buf[SAVED_PANIC_EXT_OFFSET],
					saved_nvmem_buf[READ_PANIC_EXT_OFFSET + 1],
					saved_nvmem_buf[READ_PANIC_EXT_OFFSET + 2],
					saved_nvmem_buf[READ_PANIC_EXT_OFFSET + 3],
					saved_nvmem_buf[READ_PANIC_EXT_OFFSET + 4],
					saved_nvmem_buf[READ_PANIC_EXT_OFFSET + 5],
					saved_nvmem_buf[READ_PANIC_EXT_OFFSET + 6],
					saved_nvmem_buf[READ_PANIC_EXT_OFFSET + 7]);

			// 将数据写入 NVMEM 单元
			nvmem_cell_write(reboot->vendor_zlog_panic_ext_nvmem_cell,
					 &saved_nvmem_buf[SAVED_PANIC_EXT_OFFSET],
					 SDAM_SPACE_LEN_PANIC_EXT);
		}
/* Ended by AICoder, pid:cc418279a0j36b214ad109f79097b92ecb91266a */
	} else {
		pr_err("ztedbg NULL reboot struct in panic save");
	}
}

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

#define NVMEM_BUF_MAXLEN 64
static ssize_t boot_nvmem_show(struct kobject *kobj, struct attribute *this, char *buf)
{
/* Started by AICoder, pid:901e7acca0j14301410c0b2500309623ef76ac65 */
	char output[NVMEM_BUF_MAXLEN];  // max enough
	char *output_ptr = output;
	int i;

	if (SDAM_SPACE_LEN + 2 > NVMEM_BUF_MAXLEN) {
		return scnprintf(buf, PAGE_SIZE, "%s", INVALID_READ_STR);
	}

	for (i = 0; i < SDAM_SPACE_LEN; i++) {
		if (isprint(read_nvmem_buf[i])) {
			*output_ptr++ = read_nvmem_buf[i];
		} else {
			*output_ptr++ = '*';
		}
	}
	*output_ptr++ = '\n';  // 添加换行符
	*output_ptr = 0;  // zero end

	return scnprintf(buf, PAGE_SIZE, "%s", output);
/* Ended by AICoder, pid:901e7acca0j14301410c0b2500309623ef76ac65 */
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

static int zte_reboot_ext_probe(struct platform_device *pdev)
{
	struct zte_reboot_ext *reboot;
	u8 *buf;
	int ret;
	size_t len;
	int i;

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

/* Started by AICoder, pid:k737euc05dg363f14e310b40f099e03c2056b3aa */
	// 获取 NVMEM 单元
	reboot->vendor_zlog_panic_ext_nvmem_cell = nvmem_cell_get(reboot->dev, "vendor_zlog_panic_ext");
	if (IS_ERR(reboot->vendor_zlog_panic_ext_nvmem_cell)) {
		ret = PTR_ERR(reboot->vendor_zlog_panic_ext_nvmem_cell);
		pr_err("ztedbg failed to init vendor_zlog_panic_ext %d\n", ret);
	} else {
		// 读取 NVMEM 单元内容
		buf = nvmem_cell_read(reboot->vendor_zlog_panic_ext_nvmem_cell, &len);

		if (IS_ERR(buf)) {
			ret = PTR_ERR(buf);
			pr_err("ztedbg failed to read vendor_zlog_panic_ext %d\n", ret);
		} else {
			if (len >= SDAM_SPACE_LEN_PANIC_EXT) {
			// 将读取的数据存储到 read_nvmem_buf 中
				for (i = 0; i < SDAM_SPACE_LEN_PANIC_EXT; i++) {
					read_nvmem_buf[READ_PANIC_EXT_OFFSET + i] = buf[i];
				}

				// 打印读取的数据
				pr_info("ztedbg read %zu vendor_zlog_panic_ext: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", len,
						read_nvmem_buf[READ_PANIC_EXT_OFFSET],
						read_nvmem_buf[READ_PANIC_EXT_OFFSET + 1],
						read_nvmem_buf[READ_PANIC_EXT_OFFSET + 2],
						read_nvmem_buf[READ_PANIC_EXT_OFFSET + 3],
						read_nvmem_buf[READ_PANIC_EXT_OFFSET + 4],
						read_nvmem_buf[READ_PANIC_EXT_OFFSET + 5],
						read_nvmem_buf[READ_PANIC_EXT_OFFSET + 6],
						read_nvmem_buf[READ_PANIC_EXT_OFFSET + 7]);
			} else {
				pr_err("ztedbg unexpected vendor_zlog_panic_ext len %zu\n", len);
			}
			kfree(buf);	 // 释放缓冲区
		}
	}
/* Ended by AICoder, pid:k737euc05dg363f14e310b40f099e03c2056b3aa */

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
