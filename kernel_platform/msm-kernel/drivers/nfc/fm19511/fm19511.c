/* Started by AICoder, pid:u8f33ma3f8obfee14a2d0962a67e141b9923483c */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 * NFC Controller Driver
 * Copyright (C) 2025 ZTE FM Tag.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/of_gpio.h>
#ifndef LEGACY
#include <linux/workqueue.h>
#include <linux/acpi.h>
#include <linux/gpio/consumer.h>
#include <net/nfc/nci.h>
#include <linux/clk.h>
#else
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_address.h>
#endif
#include <linux/of_irq.h>
#include <linux/proc_fs.h>

#define DEVICE_NAME          "fmtag"
#define DRIVER_VERSION       "1.0.0"
#define MAX_BUFFER_SIZE      101  // 2+3+31+64+1
#define FMTAG_RESET          _IOW('g', 0x31, __u32)

#define ZTE_FMTAG_PROC_DIR   "fmtag"
#define ZTE_FMTAG_RESET      "fmtag_reset"
#define ZTE_FMTAG_IRQ        "fmtag_irq"

static bool enable_debug_log = true;
struct proc_dir_entry *fmtag_proc_dir = NULL;
struct mutex fmtag_reset_lock;

struct fmtag_device {
    struct i2c_client *client;
    struct miscdevice fmtag_device;
    bool device_open;
    bool irq_is_attached;
    spinlock_t irq_enabled_lock;
    unsigned int polarity_mode;
    bool irq_enabled;
    wait_queue_head_t read_wq;
    struct gpio_desc *gpiod_irq;
    struct gpio_desc *gpiod_reset;
    struct regulator *vdd;
};
struct fmtag_device *fmtag_dev = NULL;

static void fmtag_disable_irq(struct fmtag_device *fmtag_dev)
{
    unsigned long flags;

    pr_info("%s : enter\n", __func__);
    spin_lock_irqsave(&fmtag_dev->irq_enabled_lock, flags);
    if (fmtag_dev->irq_enabled) {
        disable_irq_nosync(fmtag_dev->client->irq);
        fmtag_dev->irq_enabled = false;
    }
    spin_unlock_irqrestore(&fmtag_dev->irq_enabled_lock, flags);
}

static void fmtag_enable_irq(struct fmtag_device *fmtag_dev)
{
    unsigned long flags;

    pr_info("%s : enter\n", __func__);
    spin_lock_irqsave(&fmtag_dev->irq_enabled_lock, flags);
    if (!fmtag_dev->irq_enabled) {
        pr_info("%s : enable_irq enter\n", __func__);
        fmtag_dev->irq_enabled = true;
        enable_irq(fmtag_dev->client->irq);
    }
    spin_unlock_irqrestore(&fmtag_dev->irq_enabled_lock, flags);
}

static irqreturn_t fmtag_dev_irq_handler(int irq, void *dev_id)
{
    struct fmtag_device *fmtag_dev = dev_id;

    pr_info("%s : enter\n", __func__);
    fmtag_disable_irq(fmtag_dev);
    /* Wake up waiting readers */
    wake_up(&fmtag_dev->read_wq);
    return IRQ_HANDLED;
}

static int fmtag_loc_set_polaritymode(
    struct fmtag_device *fmtag_dev, int mode)
{
    struct i2c_client *client = fmtag_dev->client;
    struct device *dev = &client->dev;
    unsigned int irq_type;
    int ret;

    pr_info("%s: mode %d", __func__, mode);
    fmtag_dev->polarity_mode = mode;
    /* setup irq_flags */
    switch (mode) {
    case IRQF_TRIGGER_RISING:
        irq_type = IRQ_TYPE_EDGE_RISING;
        break;
    case IRQF_TRIGGER_LOW:
        irq_type = IRQ_TYPE_LEVEL_LOW;
        break;
    default:
        irq_type = IRQ_TYPE_LEVEL_LOW;
        break;
    }
    if (fmtag_dev->irq_is_attached) {
        devm_free_irq(dev, client->irq, fmtag_dev);
        fmtag_dev->irq_is_attached = false;
    }
    ret = irq_set_irq_type(client->irq, irq_type);
    if (ret) {
        pr_err("%s : set_irq_type failed\n", __func__);
        return -ENODEV;
    }
    /* request irq.  the irq is set whenever the chip has data available
     * for reading.  it is cleared when all data has been read.
     */
    pr_info("%s : requesting IRQ %d\n", __func__, client->irq);
    fmtag_dev->irq_enabled = true;
    ret = devm_request_irq(dev, client->irq, fmtag_dev_irq_handler,
                           fmtag_dev->polarity_mode,
                           client->name, fmtag_dev);
    if (ret) {
        pr_err("%s : devm_request_irq failed\n", __func__);
        return -ENODEV;
    }
    fmtag_dev->irq_is_attached = true;
    fmtag_disable_irq(fmtag_dev);
    pr_info("%s: ret %d", __func__, ret);
    return ret;
}

static unsigned int fmtag_poll(struct file *file, poll_table *wait)
{
    struct fmtag_device *fmtag_dev =
        container_of(file->private_data,
            struct fmtag_device, fmtag_device);
    unsigned int mask = 0;
    int pinlev = 0;

    pr_info("%s: enter", __func__);
    /* wait for Wake_up_pin == high  */
    poll_wait(file, &fmtag_dev->read_wq, wait);
    pinlev = gpiod_get_value(fmtag_dev->gpiod_irq);
    if (pinlev != 1) {
        pr_info("%s RF field in\n", __func__);
        mask = POLLIN | POLLRDNORM; /* signal data avail */
        fmtag_disable_irq(fmtag_dev);
    } else {
        /* Wake_up_pin is low. Activate ISR  */
        if (!fmtag_dev->irq_enabled) {
            pr_info("%s RF field off\n", __func__);
            fmtag_enable_irq(fmtag_dev);
        } else {
            pr_info("%s irq already enabled\n", __func__);
        }
    }
    return mask;
}

static ssize_t fmtag_dev_read(
    struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
    struct fmtag_device *fmtag_dev =
        container_of(filp->private_data,
            struct fmtag_device, fmtag_device);
    int ret;
    uint8_t buffer[MAX_BUFFER_SIZE];
    if (count == 0)
        return 0;
    if (count > MAX_BUFFER_SIZE)
        count = MAX_BUFFER_SIZE;
    /* Read data */
    ret = i2c_master_recv(fmtag_dev->client, buffer, count);
    if (enable_debug_log){
        char tmp_Str[521] = { 0x00 };
        int i = 0;
        for(i = 0; i < ret; i++) {
            snprintf(tmp_Str + 2 * i, 3, "%02hhx", buffer[i]);
        }
        pr_info("%s : reading %zu bytes.ret = %d.DATA:%s\n", __func__, count, ret, tmp_Str);
    }
    if (ret < 0) {
        pr_err("%s: i2c_master_recv returned %d\n", __func__, ret);
        return ret;
    }
    if (ret > count) {
        pr_err("%s: received too many bytes from i2c (%d)\n",
            __func__, ret);
        return -EIO;
    }
    if (copy_to_user(buf, buffer, ret)) {
        pr_info("%s : failed to copy to user space\n", __func__);
        return -EFAULT;
    }
    return ret;
}

static ssize_t fmtag_dev_write(struct file *filp, const char __user *buf,
    size_t count, loff_t *offset)
{
    struct fmtag_device *fmtag_dev =
        container_of(filp->private_data,
            struct fmtag_device, fmtag_device);
    char *tmp = NULL;
    int ret = count;

    tmp = memdup_user(buf, count);
    if (IS_ERR_OR_NULL(tmp)) {
        pr_err("%s : memdup_user failed\n", __func__);
        return -EFAULT;
    }
    if (enable_debug_log) {
        char tmp_Str[521] = { 0x00 };
        int i = 0;
        for(i = 0; i < ret; i++) {
            snprintf(tmp_Str + 2 * i, 3, "%02hhx", tmp[i]);
        }
        pr_info("%s : writing %zu bytes.DATA:%s\n", __func__, count, tmp_Str);
    }
    /* Write data */
    ret = i2c_master_send(fmtag_dev->client, tmp, count);
    if (ret != count) {
        pr_err("%s : i2c_master_send returned %d\n", __func__, ret);
        ret = -EIO;
    }
    kfree(tmp);
    return ret;
}

static int fmtag_dev_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
    struct fmtag_device *fmtag_dev =
        container_of(filp->private_data,
            struct fmtag_device, fmtag_device);

    pr_info("%s:%d dev_open", __FILE__, __LINE__);
    if (fmtag_dev->device_open) {
        ret = -EBUSY;
        pr_err("%s : device already opened ret= %d\n", __func__, ret);
    } else {
        fmtag_dev->device_open = true;
    }
    return ret;
}

static int fmtag_release(struct inode *inode, struct file *file)
{
    struct fmtag_device *fmtag_dev =
        container_of(file->private_data,
            struct fmtag_device, fmtag_device);

    pr_info("%s : device_open  = false\n", __func__);
    fmtag_dev->device_open = false;
    return 0;
}

static long fmtag_dev_ioctl(struct file *filp,
    unsigned int cmd, unsigned long arg)
{
    struct fmtag_device *fmtag_dev =
        container_of(filp->private_data,
            struct fmtag_device, fmtag_device);
    int ret = 0;

    pr_info("%s : enter, cmd=%d\n", __func__, cmd);
    switch (cmd) {
    case FMTAG_RESET:
        pr_info("%s: FMTAG_RESET\n", __func__);
        gpiod_set_value(fmtag_dev->gpiod_reset, 1);
        udelay(200);
        gpiod_set_value(fmtag_dev->gpiod_reset, 0);
        msleep(5);
        gpiod_set_value(fmtag_dev->gpiod_reset, 1);
        msleep(20);
        pr_info("%s: FMTAG_RESET end\n", __func__);
        break;
    default:
        ret = -ENOTTY;
    }
    return ret;
}

static ssize_t fmtag_reset_read(struct file *file,
                                char __user *buffer, size_t count, loff_t * offset)
{
    ssize_t ret = 0;
    unsigned char reset_flag[4];
    int reset_gpio_value = 0;

    pr_info("%s: enter\n", __func__);
    if( *offset > 0)
        return 0;
    mutex_lock(&fmtag_reset_lock);
    reset_gpio_value = gpiod_get_value(fmtag_dev->gpiod_reset);
    if (reset_gpio_value == 0) {
        snprintf(reset_flag, sizeof(reset_flag), "%d", 0);
    } else {
        snprintf(reset_flag, sizeof(reset_flag), "%d", 1);
    }
    ret = copy_to_user(buffer, &reset_flag, sizeof(reset_flag));
    if (ret < 0) {
        pr_err("%s: failed to copy data to user space!\n", __func__);
        return ret;
    }
    ret = sizeof(reset_flag);
    *offset += ret;
    mutex_unlock(&fmtag_reset_lock);

    pr_info("%s: count:%lu ret:%ld, reset_flag:%s, offset:%lld\n", __func__, count, ret, reset_flag, *offset);
    pr_info("%s: finished\n", __func__);
    return ret;
}

static ssize_t fmtag_reset_write(struct file *file,
                                const char __user *buffer, size_t count, loff_t *pos) {
    ssize_t ret = 0;
    unsigned char reset_flag[8];

    pr_info("%s: enter\n", __func__);
    mutex_lock(&fmtag_reset_lock);
    ret = copy_from_user(reset_flag, buffer, sizeof(reset_flag));
    if (ret < 0) {
        pr_err("%s: failed to copy data from user space!\n", __func__);
        return ret;
    }
	pr_info("%s: reset_flag is %s, count is %lu, ret is %ld.\n",
			__func__, reset_flag, count, ret);
    if(reset_flag[0] == '1') {
        pr_err("%s: set reset 1\n", __func__);
        gpiod_set_value(fmtag_dev->gpiod_reset, 1);
    }
    if(reset_flag[0] == '0') {
        pr_err("%s: set reset 0\n", __func__);
        gpiod_set_value(fmtag_dev->gpiod_reset, 0);
    }
    mutex_unlock(&fmtag_reset_lock);
    pr_info("%s: finished!\n", __func__);

    return count;
}

static ssize_t fmtag_irq_read(struct file *file,
                                char __user *buffer, size_t count, loff_t * offset) {
    ssize_t ret = 0;
    unsigned char irq_flag[4];
    int irq_gpio_value = 0;

    pr_info("%s: enter\n", __func__);
    if( *offset > 0)
        return 0;
    mutex_lock(&fmtag_reset_lock);
    irq_gpio_value = gpiod_get_value(fmtag_dev->gpiod_irq);
    if (irq_gpio_value == 0) {
        snprintf(irq_flag, sizeof(irq_flag), "%d", 0);
    } else {
        snprintf(irq_flag, sizeof(irq_flag), "%d", 1);
    }
    ret = copy_to_user(buffer, &irq_flag, sizeof(irq_flag));
    if (ret < 0) {
        pr_err("%s: failed to copy data to user space!\n", __func__);
        return ret;
    }
    ret = sizeof(irq_flag);
    *offset += ret;
    mutex_unlock(&fmtag_reset_lock);

    pr_info("%s: count:%lu ret:%ld, irq_flag:%s, irq_gpio_value:%d, offset:%lld\n",
            __func__, count, ret, irq_flag, irq_gpio_value, *offset);
    pr_info("%s: finished\n", __func__);
    return ret;
}

static ssize_t fmtag_irq_write(struct file *file,
                                const char __user *buffer, size_t count, loff_t *pos) {
    ssize_t ret = 0;
    unsigned char irq_flag[8];

    pr_info("%s: enter\n", __func__);
    mutex_lock(&fmtag_reset_lock);
    ret = copy_from_user(irq_flag, buffer, sizeof(irq_flag));
    if (ret < 0) {
        pr_err("%s: failed to copy data from user space!\n", __func__);
        return ret;
    }
	pr_info("%s: irq_flag is %s, count is %lu ret is %ld.\n",
			__func__, irq_flag, count, ret);
    if(irq_flag[0] == '1') {
        pr_err("%s: set irq 1\n", __func__);
        gpiod_set_value(fmtag_dev->gpiod_irq, 1);
    }
    if(irq_flag[0] == '0') {
        pr_err("%s:  set irq 0\n", __func__);
        gpiod_set_value(fmtag_dev->gpiod_irq, 0);
    }
    mutex_unlock(&fmtag_reset_lock);
    pr_info("%s: finished!\n", __func__);

    return count;
}

static const struct proc_ops proc_ops_fmtag_reset = {
    .proc_read = fmtag_reset_read,
    .proc_write = fmtag_reset_write,
};
static const struct proc_ops proc_ops_fmtag_irq = {
    .proc_read = fmtag_irq_read,
    .proc_write = fmtag_irq_write,
};

static void create_fmtag_proc_entry(void) {
    struct proc_dir_entry *fmtag_reset_proc_entry = NULL;
    struct proc_dir_entry *fmtag_irq_proc_entry = NULL;

    pr_info("%s: enter \n", __func__);
    fmtag_proc_dir = proc_mkdir(ZTE_FMTAG_PROC_DIR, NULL);
    if(fmtag_proc_dir == NULL) {
        pr_err("%s: Failed to create fmtag entry\n", __func__);
        return;
    }

    fmtag_reset_proc_entry = proc_create(ZTE_FMTAG_RESET, 0666, fmtag_proc_dir, &proc_ops_fmtag_reset);
    if (fmtag_reset_proc_entry == NULL) {
        pr_err("%s: Failed to create fmtag reset entry\n", __func__);
        remove_proc_entry(ZTE_FMTAG_PROC_DIR, NULL);
        return;
    }
    fmtag_irq_proc_entry = proc_create(ZTE_FMTAG_IRQ, 0666, fmtag_proc_dir, &proc_ops_fmtag_irq);
    if (fmtag_irq_proc_entry == NULL) {
        pr_err("%s: Failed to create fmtag irq entry\n", __func__);
        remove_proc_entry(ZTE_FMTAG_RESET, fmtag_proc_dir);
        remove_proc_entry(ZTE_FMTAG_PROC_DIR, NULL);
        return;
    }
    return;
}

static const struct file_operations fmtag_dev_fops = {
    .owner = THIS_MODULE,
    .llseek = no_llseek,
    .read = fmtag_dev_read,
    .write = fmtag_dev_write,
    .open = fmtag_dev_open,
    .poll = fmtag_poll,
    .release = fmtag_release,
    .unlocked_ioctl = fmtag_dev_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = fmtag_dev_ioctl
#endif
};

static int fmtag_probe(struct i2c_client *client) {
    int ret;
    int voltage;
    struct device *dev = &client->dev;

    pr_info("%s : probe enter\n", __func__);
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        pr_err("%s : need I2C_FUNC_I2C\n", __func__);
        return -ENODEV;
    }

    fmtag_dev = devm_kzalloc(dev, sizeof(*fmtag_dev), GFP_KERNEL);
    if (!fmtag_dev)
    {
        pr_err("%s : alloc failed\n", __func__);
        return -ENOMEM;
    }

    fmtag_dev->client = client;

    fmtag_dev->vdd = devm_regulator_get(dev, "vdd");
    if (IS_ERR(fmtag_dev->vdd)) {
        ret = PTR_ERR(fmtag_dev->vdd);
        pr_err("%s: Regulator vdd get failed ret=%d\n", __func__, ret);
        fmtag_dev->vdd = NULL;
        return ret;
    }
    voltage = regulator_get_voltage(fmtag_dev->vdd);
    if (voltage < 0) {
        pr_err("%s: Failed to get voltage: %d\n", __func__, voltage);
        ret = -1;
        goto deinit_vdd;
    } else {
        pr_info("%s: Current voltage: %d uV\n", __func__, voltage);
    }

    /*ret = regulator_set_voltage(fmtag_dev->vdd, 3300000, 3300000);  // 3.3v
    if (ret) {
        pr_err("%s: Set voltage failed ret=%d\n", __func__, ret);
        goto deinit_vdd;
    }*/
    ret = regulator_enable(fmtag_dev->vdd);
    if (ret) {
        pr_err("%s: Enable regulator vdd failed ret=%d\n", __func__, ret);
        goto deinit_vdd;
    }

    fmtag_dev->gpiod_reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
    if (IS_ERR(fmtag_dev->gpiod_reset)) {
        pr_err("%s : Unable to request reset-gpios\n", __func__);
        ret = -ENODEV;
        goto deinit_vdd;
    }
    ret = gpiod_direction_output(fmtag_dev->gpiod_reset, 1);
    if (ret) {
        pr_err("%s : failed to set reset-gpios 1\n", __func__);
        goto deinit_reset;
    }

    fmtag_dev->irq_is_attached = false;
    fmtag_dev->gpiod_irq = devm_gpiod_get(dev, "irq", GPIOD_IN);
    if (IS_ERR(fmtag_dev->gpiod_irq)) {
        pr_err("%s : Unable to request irq-gpios\n", __func__);
        ret = -ENODEV;
        goto deinit_reset;
    }
    client->irq = gpiod_to_irq(fmtag_dev->gpiod_irq);
    init_waitqueue_head(&fmtag_dev->read_wq);
    spin_lock_init(&fmtag_dev->irq_enabled_lock);
    pr_info("%s : client-irq =  %d\n", __func__, client->irq);
    fmtag_loc_set_polaritymode(fmtag_dev, IRQF_TRIGGER_LOW);

    fmtag_dev->fmtag_device.minor = MISC_DYNAMIC_MINOR;
    fmtag_dev->fmtag_device.name = DEVICE_NAME;
    fmtag_dev->fmtag_device.fops = &fmtag_dev_fops;
    fmtag_dev->fmtag_device.parent = dev;
    i2c_set_clientdata(client, fmtag_dev);
    ret = misc_register(&fmtag_dev->fmtag_device);
    if (ret) {
        pr_err("%s : misc_register failed\n", __func__);
        goto deinit_irq;
    }

    create_fmtag_proc_entry();
    mutex_init(&fmtag_reset_lock);

    pr_info("%s : fmtag probe complete\n", __func__);
    return 0;

deinit_irq:
    if(fmtag_dev->gpiod_irq) {
        devm_gpiod_put(dev, fmtag_dev->gpiod_irq);
    }
deinit_reset:
    if(fmtag_dev->gpiod_reset) {
        devm_gpiod_put(dev, fmtag_dev->gpiod_reset);
    }
deinit_vdd:
    if(fmtag_dev->vdd) {
        devm_regulator_put(fmtag_dev->vdd);
    }

    return ret;
}

static void fmtag_remove(struct i2c_client *client)
{
    struct fmtag_device *fmtag_dev = i2c_get_clientdata(client);
    misc_deregister(&fmtag_dev->fmtag_device);
    remove_proc_entry(ZTE_FMTAG_IRQ, fmtag_proc_dir);
    remove_proc_entry(ZTE_FMTAG_RESET, fmtag_proc_dir);
    remove_proc_entry(ZTE_FMTAG_PROC_DIR, NULL);
}

static const struct i2c_device_id fmtag_id[] = {
    {"fmtag", 0},
    {}
};

static const struct of_device_id fmtag_of_match[] = {
    {
        .compatible = "fm,fmtag",
    },
    { }
};
MODULE_DEVICE_TABLE(of, fmtag_of_match);

static struct i2c_driver fmtag_driver = {
    .id_table = fmtag_id,
    .probe = fmtag_probe,
    .remove = fmtag_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "fmtag",
        .of_match_table = fmtag_of_match,
        .probe_type = PROBE_PREFER_ASYNCHRONOUS,
    },
};

/* module load/unload record keeping */
static int __init fmtag_dev_init(void)
{
    pr_info("Loading fmtag driver\n");
    return i2c_add_driver(&fmtag_driver);
}

static void __exit fmtag_dev_exit(void)
{
    pr_info("Unloading fmtag driver\n");
    i2c_del_driver(&fmtag_driver);
}

module_init(fmtag_dev_init);
module_exit(fmtag_dev_exit);
MODULE_AUTHOR("zte fmtag");
MODULE_DESCRIPTION("NFC fm tag driver");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
/* Ended by AICoder, pid:u8f33ma3f8obfee14a2d0962a67e141b9923483c */

