/*
 * Copyright (c) 2024 ZTE Inc.
 */
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/printk.h>

#include "ifas_inner.h"

/* Started by AICoder, pid:g3048x4b14l124a143160b76b0ee76345b51e004 */
void (*kgsl_pwrctrl_set_max_level_fp)(u32 level);
EXPORT_SYMBOL_GPL(kgsl_pwrctrl_set_max_level_fp);

u32 (*kgsl_pwrctrl_get_max_level_fp)(void);
EXPORT_SYMBOL_GPL(kgsl_pwrctrl_get_max_level_fp);

void (*kgsl_pwrctrl_set_min_level_fp)(u32 level);
EXPORT_SYMBOL_GPL(kgsl_pwrctrl_set_min_level_fp);

u32 (*kgsl_pwrctrl_get_min_level_fp)(void);
EXPORT_SYMBOL_GPL(kgsl_pwrctrl_get_min_level_fp);

int (*kgsl_pwrctrl_get_loading_fp)(void);
EXPORT_SYMBOL_GPL(kgsl_pwrctrl_get_loading_fp);

int (*kgsl_gpu_num_freqs_fp)(void);
EXPORT_SYMBOL_GPL(kgsl_gpu_num_freqs_fp);


static int __init ifas_inner_init(void)
{
	int ret_val = 0;
	return ret_val;
}

static void __exit ifas_inner_exit(void)
{

}

late_initcall(ifas_inner_init);
module_exit(ifas_inner_exit);
/* Ended by AICoder, pid:g3048x4b14l124a143160b76b0ee76345b51e004 */

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("zte ifas_inner");
MODULE_AUTHOR("ZTE Inc.");
