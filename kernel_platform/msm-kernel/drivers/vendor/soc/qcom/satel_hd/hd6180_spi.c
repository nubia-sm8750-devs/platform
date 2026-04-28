// SPDX-License-Identifier: GPL-2.0-only
/*
 * Simple synchronous userspace interface to SPI devices
 * Copyright (C) 2006 SWAPP Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 * Copyright (C) 2020 ST Microelectronics S.A.
 * Copyright (C) 2024 ZTE boot team
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/acpi.h>
#include <linux/pinctrl/consumer.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <linux/uaccess.h>

#include <linux/ktime.h>
#include <linux/timekeeping.h>


#include <linux/version.h>


/*
 * This supports access to SPI devices using normal userspace I/O calls.
 * Note that while traditional UNIX/POSIX I/O semantics are half duplex,
 * and often mask message boundaries, full SPI support requires full duplex
 * transfers.  There are several kinds of internal message boundaries to
 * handle chipselect management and other protocol options.
 *
 * SPI has a character major number assigned.  We allocate minor numbers
 * dynamically using a bitmask.  You must use hotplug tools, such as udev
 * (or mdev with busybox) to create and destroy the /dev/hdbdspi device
 * nodes, since there is no fixed association of minor numbers with any
 * particular SPI bus or device.
 */
//#define SPIDEV_MAJOR			0	/* dynamic */
static int spidev_major;
#define N_SPI_MINORS 1 /* ... up to 256 */

static DECLARE_BITMAP(minors, N_SPI_MINORS);

// newer kernels since 5.4
#define ACCESS_OK(x, y, z) access_ok(y, z)




/* Bit masks for spi_device.mode management.  Note that incorrect
 * settings for some settings can cause *lots* of trouble for other
 * devices on a shared bus:
 *
 *  - CS_HIGH ... this device will be active when it shouldn't be
 *  - 3WIRE ... when active, it won't behave as it should
 *  - NO_CS ... there will be no explicit message boundaries; this
 *	is completely incompatible with the shared bus model
 *  - READY ... transfers may proceed when they shouldn't.
 *
 * REVISIT should changing those flags be privileged?
 */
#define SPI_MODE_MASK                                                          \
	(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH | SPI_LSB_FIRST | SPI_3WIRE |       \
	 SPI_LOOP | SPI_NO_CS | SPI_READY | SPI_TX_DUAL | SPI_TX_QUAD |        \
	 SPI_RX_DUAL | SPI_RX_QUAD)

struct hdbdspi_data {
	dev_t devt;
	spinlock_t spi_lock;
	struct spi_device *spi;
	struct spi_device *spi_reset;
	struct list_head device_entry;

	struct kobject *kobj;
	/* TX/RX buffers are NULL unless this device is open (users > 0) */
	struct mutex buf_lock;
	unsigned int users;
	u8 *tx_buffer;
	u8 *rx_buffer;
	u8 *null_buffer;
	u32 speed_hz;

	/* GPIOs for hd6180 chip control */
	int bd_msg_1p8_en;
	int bd_msg_reset_n;
	int bd_msg_download;
	int bd_msg_int;
};


static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static unsigned int bufsiz = 4096;
module_param(bufsiz, uint, 0444);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");

static bool debug_enabled = true;
#define VERBOSE 1

#define DEV (hdbdspi->spi ? &hdbdspi->spi->dev : &hdbdspi->spi_reset->dev)


/*-------------------------------------------------------------------------*/


static ssize_t hdbdspi_sync(struct hdbdspi_data *hdbdspi,
			    struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;
	struct spi_device *spi;

	spin_lock_irq(&hdbdspi->spi_lock);
	spi = hdbdspi->spi;
	spin_unlock_irq(&hdbdspi->spi_lock);

	if (spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_sync(spi, message);

	if (status == 0)
		status = message->actual_length;

	return status;
}

static inline ssize_t hdbdspi_sync_write(struct hdbdspi_data *hdbdspi,
					 size_t len)
{
	struct spi_transfer t = {
		.tx_buf = hdbdspi->tx_buffer,
		.len = len,
		.speed_hz = hdbdspi->speed_hz,
	};
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return hdbdspi_sync(hdbdspi, &m);
}

static inline ssize_t hdbdspi_sync_read(struct hdbdspi_data *hdbdspi,
					size_t len)
{
	struct spi_transfer t = {
		.rx_buf = hdbdspi->rx_buffer,
		.tx_buf = hdbdspi->null_buffer,
		.len = len,
		.speed_hz = hdbdspi->speed_hz,
	};
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return hdbdspi_sync(hdbdspi, &m);

}

/*-------------------------------------------------------------------------*/

/* Read-only message with current device setup */
static ssize_t hdbdspi_read(struct file *filp, char __user *buf, size_t count,
			    loff_t *f_pos)
{
	struct hdbdspi_data *hdbdspi;
	ssize_t status = 0;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	hdbdspi = filp->private_data;

	if (debug_enabled)
		dev_info(DEV, "hdbdspi Read: %zu bytes\n", count);

	mutex_lock(&hdbdspi->buf_lock);
	status = hdbdspi_sync_read(hdbdspi, count);
	if (status > 0) {
		unsigned long missing;

		missing = copy_to_user(buf, hdbdspi->rx_buffer, status);
		if (missing == status)
			status = -EFAULT;
		else
			status = status - missing;
	}
	mutex_unlock(&hdbdspi->buf_lock);

	if (debug_enabled)
		dev_info(DEV, "hdbdspi Read: status: %zd\n", status);

	return status;
}

/* Write-only message with current device setup */
static ssize_t hdbdspi_write(struct file *filp, const char __user *buf,
			     size_t count, loff_t *f_pos)
{
	struct hdbdspi_data *hdbdspi;
	ssize_t status = 0;
	unsigned long missing;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	hdbdspi = filp->private_data;

	if (debug_enabled)
		dev_info(DEV, "hdbdspi Write: %zu bytes\n", count);

	mutex_lock(&hdbdspi->buf_lock);
	missing = copy_from_user(hdbdspi->tx_buffer, buf, count);
	if (missing == 0)
		status = hdbdspi_sync_write(hdbdspi, count);
	else
		status = -EFAULT;
	mutex_unlock(&hdbdspi->buf_lock);

	if (debug_enabled)
		dev_info(DEV, "hdbdspi Write: status: %zd\n", status);

	return status;
}

static int hdbdspi_message(struct hdbdspi_data *hdbdspi,
			   struct spi_ioc_transfer *u_xfers,
			   unsigned int n_xfers)
{
	struct spi_message msg;
	struct spi_transfer *k_xfers;
	struct spi_transfer *k_tmp;
	struct spi_ioc_transfer *u_tmp;
	unsigned int n, total, tx_total, rx_total;
	u8 *tx_buf, *rx_buf;
	int status = -EFAULT;

	spi_message_init(&msg);
	k_xfers = kcalloc(n_xfers, sizeof(*k_tmp), GFP_KERNEL);
	if (k_xfers == NULL)
		return -ENOMEM;

	/* Construct spi_message, copying any tx data to bounce buffer.
	 * We walk the array of user-provided transfers, using each one
	 * to initialize a kernel version of the same transfer.
	 */
	tx_buf = hdbdspi->tx_buffer;
	rx_buf = hdbdspi->rx_buffer;
	total = 0;
	tx_total = 0;
	rx_total = 0;
	for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers; n;
	     n--, k_tmp++, u_tmp++) {
		k_tmp->len = u_tmp->len;

		total += k_tmp->len;
		/* Since the function returns the total length of transfers
		 * on success, restrict the total to positive int values to
		 * avoid the return value looking like an error.  Also check
		 * each transfer length to avoid arithmetic overflow.
		 */
		if (total > INT_MAX || k_tmp->len > INT_MAX) {
			status = -EMSGSIZE;
			goto done;
		}

		if (u_tmp->rx_buf) {
			/* this transfer needs space in RX bounce buffer */
			rx_total += k_tmp->len;
			if (rx_total > bufsiz) {
				status = -EMSGSIZE;
				goto done;
			}
			k_tmp->rx_buf = rx_buf;
			if (!ACCESS_OK(VERIFY_WRITE,
				       (u8 __user *)(uintptr_t)u_tmp->rx_buf,
				       u_tmp->len))
				goto done;
			rx_buf += k_tmp->len;
		}
		if (u_tmp->tx_buf) {
			/* this transfer needs space in TX bounce buffer */
			tx_total += k_tmp->len;
			if (tx_total > bufsiz) {
				status = -EMSGSIZE;
				goto done;
			}
			k_tmp->tx_buf = tx_buf;
			if (copy_from_user(
				    tx_buf,
				    (const u8 __user *)(uintptr_t)u_tmp->tx_buf,
				    u_tmp->len))
				goto done;
			tx_buf += k_tmp->len;
		}

		k_tmp->cs_change = !!u_tmp->cs_change;
		k_tmp->tx_nbits = u_tmp->tx_nbits;
		k_tmp->rx_nbits = u_tmp->rx_nbits;
		k_tmp->bits_per_word = u_tmp->bits_per_word;
		k_tmp->delay.value = u_tmp->delay_usecs;
		k_tmp->delay.unit = SPI_DELAY_UNIT_USECS;
		k_tmp->speed_hz = u_tmp->speed_hz;
		if (!k_tmp->speed_hz)
			k_tmp->speed_hz = hdbdspi->speed_hz;
#ifdef VERBOSE
		dev_dbg(DEV, "  xfer len %u %s%s%s%dbits %u usec %uHz\n",
			u_tmp->len, u_tmp->rx_buf ? "rx " : "",
			u_tmp->tx_buf ? "tx " : "",
			u_tmp->cs_change ? "cs " : "",
			u_tmp->bits_per_word ?: hdbdspi->spi->bits_per_word,
			u_tmp->delay_usecs,
			u_tmp->speed_hz ?: hdbdspi->spi->max_speed_hz);
#endif
		spi_message_add_tail(k_tmp, &msg);
	}

	status = hdbdspi_sync(hdbdspi, &msg);
	if (status < 0)
		goto done;

	/* copy any rx data out of bounce buffer */
	rx_buf = hdbdspi->rx_buffer;
	for (n = n_xfers, u_tmp = u_xfers; n; n--, u_tmp++) {
		if (u_tmp->rx_buf) {
			if (__copy_to_user((u8 __user *)(uintptr_t)u_tmp->rx_buf,
					   rx_buf, u_tmp->len)) {
				status = -EFAULT;
				goto done;
			}
			rx_buf += u_tmp->len;
		}
	}
	status = total;

done:
	kfree(k_xfers);
	return status;
}

static struct spi_ioc_transfer *
hdbdspi_get_ioc_message(unsigned int cmd, struct spi_ioc_transfer __user *u_ioc,
			unsigned int *n_ioc)
{
	struct spi_ioc_transfer *ioc;
	u32 tmp;

	/* Check type, command number and direction */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC ||
	    _IOC_NR(cmd) != _IOC_NR(SPI_IOC_MESSAGE(0)) ||
	    _IOC_DIR(cmd) != _IOC_WRITE)
		return ERR_PTR(-ENOTTY);

	tmp = _IOC_SIZE(cmd);
	if ((tmp % sizeof(struct spi_ioc_transfer)) != 0)
		return ERR_PTR(-EINVAL);

	*n_ioc = tmp / sizeof(struct spi_ioc_transfer);
	if (*n_ioc == 0)
		return NULL;

	/* copy into scratch area */
	ioc = kmalloc(tmp, GFP_KERNEL);
	if (!ioc)
		return ERR_PTR(-ENOMEM);

	if (__copy_from_user(ioc, u_ioc, tmp)) {
		kfree(ioc);
		return ERR_PTR(-EFAULT);
	}
	return ioc;
}


static long hdbdspi_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	int err = 0;
	int retval = 0;
	struct hdbdspi_data *hdbdspi;
	struct spi_device *spi;
	u32 tmp;
	unsigned int n_ioc;
	struct spi_ioc_transfer *ioc;

	/* Check type and command number */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC)
		return -ENOTTY;

	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !ACCESS_OK(VERIFY_WRITE, (void __user *)arg,
				 _IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !ACCESS_OK(VERIFY_READ, (void __user *)arg,
				 _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

    /* guard against device removal before, or while,
     * we issue this ioctl.
     */
	hdbdspi = filp->private_data;
	spin_lock_irq(&hdbdspi->spi_lock);
	spi = spi_dev_get(hdbdspi->spi);
	spin_unlock_irq(&hdbdspi->spi_lock);

	if (debug_enabled)
		dev_info(DEV, "hdbdspi ioctl cmd %d\n", cmd);

	if (spi == NULL)
		return -ESHUTDOWN;

    /*  use the buffer lock here for triple duty:
     *  - prevent I/O (from us) so calling spi_setup() is safe;
     *  - prevent concurrent SPI_IOC_WR_* from morphing
     *    data fields while SPI_IOC_RD_* reads them;
     *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
     */
	mutex_lock(&hdbdspi->buf_lock);

	switch (cmd) {
		/* read requests */
	case SPI_IOC_RD_MODE:
		retval = __put_user(spi->mode & SPI_MODE_MASK,
				    (__u8 __user *)arg);
		break;
	case SPI_IOC_RD_MODE32:
		retval = __put_user(spi->mode & SPI_MODE_MASK,
				    (__u32 __user *)arg);
		break;
	case SPI_IOC_RD_LSB_FIRST:
		retval = __put_user((spi->mode & SPI_LSB_FIRST) ? 1 : 0,
				    (__u8 __user *)arg);
		break;
	case SPI_IOC_RD_BITS_PER_WORD:
		retval = __put_user(spi->bits_per_word, (__u8 __user *)arg);
		break;
	case SPI_IOC_RD_MAX_SPEED_HZ:
		retval = __put_user(hdbdspi->speed_hz, (__u32 __user *)arg);
		break;

		/* write requests */
	case SPI_IOC_WR_MODE:
	case SPI_IOC_WR_MODE32:
		if (cmd == SPI_IOC_WR_MODE)
			retval = __get_user(tmp, (u8 __user *)arg);
		else
			retval = __get_user(tmp, (u32 __user *)arg);
		if (retval == 0) {
			u32 save = spi->mode;

			if (tmp & ~SPI_MODE_MASK) {
				retval = -EINVAL;
				break;
			}

			tmp |= spi->mode & ~SPI_MODE_MASK;
			spi->mode = (u16)tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "spi mode %x\n", tmp);
		}
		break;
	case SPI_IOC_WR_LSB_FIRST:
		retval = __get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u32 save = spi->mode;

			if (tmp)
				spi->mode |= SPI_LSB_FIRST;
			else
				spi->mode &= ~SPI_LSB_FIRST;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "%csb first\n",
					tmp ? 'l' : 'm');
		}
		break;
	case SPI_IOC_WR_BITS_PER_WORD:
		retval = __get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u8 save = spi->bits_per_word;

			spi->bits_per_word = tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->bits_per_word = save;
			else
				dev_dbg(&spi->dev, "%d bits per word\n", tmp);
		}
		break;
	case SPI_IOC_WR_MAX_SPEED_HZ:
		retval = __get_user(tmp, (__u32 __user *)arg);
		if (retval == 0) {
			u32 save = spi->max_speed_hz;

			spi->max_speed_hz = tmp;
			retval = spi_setup(spi);
			if (retval >= 0)
				hdbdspi->speed_hz = tmp;
			else
				dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);
			spi->max_speed_hz = save;
		}
		break;

	default:
		/* segmented and/or full-duplex I/O request */
		/* Check message and copy into scratch area */
		ioc = hdbdspi_get_ioc_message(
			cmd, (struct spi_ioc_transfer __user *)arg, &n_ioc);
		if (IS_ERR(ioc)) {
			retval = PTR_ERR(ioc);
			break;
		}
		if (!ioc)
			break; /* n_ioc is also 0 */

		/* translate to spi_message, execute */
		retval = hdbdspi_message(hdbdspi, ioc, n_ioc);
		kfree(ioc);
		break;
	}

	mutex_unlock(&hdbdspi->buf_lock);
	spi_dev_put(spi);

	if (debug_enabled)
		dev_info(&spi->dev, "hdbdspi ioctl retval %d\n", retval);

	return retval;
}

#ifdef CONFIG_COMPAT
static long hdbdspi_compat_ioc_message(struct file *filp, unsigned int cmd,
				       unsigned long arg)
{
	struct spi_ioc_transfer __user *u_ioc;
	int retval = 0;
	struct hdbdspi_data *hdbdspi;
	struct spi_device *spi;
	unsigned int n_ioc, n;
	struct spi_ioc_transfer *ioc;

	u_ioc = (struct spi_ioc_transfer __user *)compat_ptr(arg);
	if (!ACCESS_OK(VERIFY_READ, u_ioc, _IOC_SIZE(cmd)))
		return -EFAULT;

    /* guard against device removal before, or while,
     * we issue this ioctl.
     */
	hdbdspi = filp->private_data;
	spin_lock_irq(&hdbdspi->spi_lock);
	spi = spi_dev_get(hdbdspi->spi);
	spin_unlock_irq(&hdbdspi->spi_lock);

	if (debug_enabled)
		dev_info(DEV, "hdbdspi compat_ioctl cmd %d\n", cmd);
	if (spi == NULL)
		return -ESHUTDOWN;

	/* SPI_IOC_MESSAGE needs the buffer locked "normally" */
	mutex_lock(&hdbdspi->buf_lock);

	/* Check message and copy into scratch area */
	ioc = hdbdspi_get_ioc_message(cmd, u_ioc, &n_ioc);
	if (IS_ERR(ioc)) {
		retval = PTR_ERR(ioc);
		goto done;
	}
	if (!ioc)
		goto done; /* n_ioc is also 0 */

	/* Convert buffer pointers */
	for (n = 0; n < n_ioc; n++) {
		ioc[n].rx_buf = (uintptr_t)compat_ptr(ioc[n].rx_buf);
		ioc[n].tx_buf = (uintptr_t)compat_ptr(ioc[n].tx_buf);
	}

	/* translate to spi_message, execute */
	retval = hdbdspi_message(hdbdspi, ioc, n_ioc);
	kfree(ioc);

done:
	mutex_unlock(&hdbdspi->buf_lock);
	spi_dev_put(spi);
	if (debug_enabled)
		dev_info(DEV, "hdbdspi compat_ioctl retval %d\n", retval);
	return retval;
}

static long hdbdspi_compat_ioctl(struct file *filp, unsigned int cmd,
				 unsigned long arg)
{
	if (_IOC_TYPE(cmd) == SPI_IOC_MAGIC &&
	    _IOC_NR(cmd) == _IOC_NR(SPI_IOC_MESSAGE(0)) &&
	    _IOC_DIR(cmd) == _IOC_WRITE)
		return hdbdspi_compat_ioc_message(filp, cmd, arg);

	return hdbdspi_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define hdbdspi_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int hdbdspi_open(struct inode *inode, struct file *filp)
{
	struct hdbdspi_data *hdbdspi;
	int status = -ENXIO;

	mutex_lock(&device_list_lock);

	list_for_each_entry (hdbdspi, &device_list, device_entry) {
		if (hdbdspi->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}

	if (status) {
		dev_dbg(DEV, "hdbdspi: nothing for minor %d\n", iminor(inode));
		goto err_find_dev;
	}

	// Authorize only 1 process to open the device.
	if (hdbdspi->users > 0) {
		dev_err(DEV, "hdbdspi: already open\n");
		mutex_unlock(&device_list_lock);
		return -EBUSY;
	}

	if (debug_enabled)
		dev_info(DEV, "hdbdspi: open\n");

	if (!hdbdspi->tx_buffer) {
		hdbdspi->tx_buffer = kmalloc(bufsiz, GFP_KERNEL);
		if (!hdbdspi->tx_buffer) {
			// dev_dbg(DEV, "open/ENOMEM\n");
			status = -ENOMEM;
			goto err_find_dev;
		}
	}

	if (!hdbdspi->rx_buffer) {
		hdbdspi->rx_buffer = kmalloc(bufsiz, GFP_KERNEL);
		if (!hdbdspi->rx_buffer) {
			// dev_dbg(DEV, "open/ENOMEM\n");
			status = -ENOMEM;
			goto err_alloc_rx_buf;
		}
	}

	if (!hdbdspi->null_buffer) {
		hdbdspi->null_buffer = kzalloc(bufsiz, GFP_KERNEL);
		if (!hdbdspi->null_buffer) {
			status = -ENOMEM;
			goto err_alloc_null_buf;
		}
	}

	hdbdspi->users++;
	filp->private_data = hdbdspi;
	nonseekable_open(inode, filp);

	mutex_unlock(&device_list_lock);

	return 0;

err_alloc_null_buf:
	kfree(hdbdspi->rx_buffer);
	hdbdspi->rx_buffer = NULL;
err_alloc_rx_buf:
	kfree(hdbdspi->tx_buffer);
	hdbdspi->tx_buffer = NULL;
err_find_dev:
	mutex_unlock(&device_list_lock);
	return status;
}

static int hdbdspi_release(struct inode *inode, struct file *filp)
{
	struct hdbdspi_data *hdbdspi;
	int			dofree;

	mutex_lock(&device_list_lock);
	hdbdspi = filp->private_data;
	filp->private_data = NULL;

	/* ... after we unbound from the underlying device? */
	dofree = (hdbdspi->spi == NULL);

	/* last close? */
	hdbdspi->users--;
	if (!hdbdspi->users) {

		kfree(hdbdspi->tx_buffer);
		hdbdspi->tx_buffer = NULL;

		kfree(hdbdspi->rx_buffer);
		hdbdspi->rx_buffer = NULL;

		if (dofree)
			kfree(hdbdspi);
		else
			hdbdspi->speed_hz = hdbdspi->spi->max_speed_hz;
	}

	mutex_unlock(&device_list_lock);

	if (debug_enabled)
		dev_info(DEV, "hdbdspi: release\n");


	return 0;
}

static const struct file_operations hdbdspi_fops = {
	.owner = THIS_MODULE,
    /* REVISIT switch to aio primitives, so that userspace
     * gets more complete API coverage.  It'll simplify things
     * too, except for the locking.
     */
	.write = hdbdspi_write,
	.read = hdbdspi_read,
	.unlocked_ioctl = hdbdspi_ioctl,
	.compat_ioctl = hdbdspi_compat_ioctl,
	.open = hdbdspi_open,
	.release = hdbdspi_release,
	.llseek = no_llseek,
};

/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/hdbdspi character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */

static struct class *hdbdspi_class;

static const struct of_device_id hdbdspi_dt_ids[] = {
	{ .compatible = "hd,hdbdspi" },
	{},
};
MODULE_DEVICE_TABLE(of, hdbdspi_dt_ids);

#ifdef CONFIG_ACPI

/* Dummy SPI devices not to be used in production systems */
#define SPIDEV_ACPI_DUMMY 1

static const struct acpi_device_id hdbdspi_acpi_ids[] = {
    /*
     * The ACPI SPT000* devices are only meant for development and
     * testing. Systems used in production should have a proper ACPI
     * description of the connected peripheral and they should also use
     * a proper driver instead of poking directly to the SPI bus.
     */
	{ "SPT0001", SPIDEV_ACPI_DUMMY },
	{ "SPT0002", SPIDEV_ACPI_DUMMY },
	{ "SPT0003", SPIDEV_ACPI_DUMMY },
	{},
};
MODULE_DEVICE_TABLE(acpi, hdbdspi_acpi_ids);

static void hdbdspi_probe_acpi(struct spi_device *spi)
{
	const struct acpi_device_id *id;

	if (!has_acpi_companion(&spi->dev))
		return;

	id = acpi_match_device(hdbdspi_acpi_ids, &spi->dev);
	if (WARN_ON(!id))
		return;
}
#else
static inline void hdbdspi_probe_acpi(struct spi_device *spi)
{
}
#endif

static ssize_t bd_msg_1p8_en_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int value = 0;
	struct hdbdspi_data* pdata = dev_get_drvdata(dev);

	sscanf(buff, "%u", &value);

	if (value) {
		pr_info("%s: hd6180 chip, set poweron pin high", __func__);
		gpio_direction_output(pdata->bd_msg_1p8_en, 1);
	} else {
		pr_info("%s: hd6180 chip, set poweron pin low", __func__);
		gpio_direction_output(pdata->bd_msg_1p8_en, 0);
	}

	return strnlen(buff, size);
}

static ssize_t bd_msg_1p8_en_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	unsigned int value = 0;
	struct hdbdspi_data* pdata = dev_get_drvdata(dev);

	value = gpio_get_value(pdata->bd_msg_1p8_en);
	return sprintf(buff, "%d\n", value);
}

static DEVICE_ATTR(bd_msg_1p8_en, S_IRUGO | S_IWUSR, bd_msg_1p8_en_show,  bd_msg_1p8_en_store);


static ssize_t bd_msg_reset_n_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int value = 0;
	struct hdbdspi_data* pdata = dev_get_drvdata(dev);

	sscanf(buff, "%u", &value);

	if (value) {
		pr_info("%s: hd6180 chip, set reset pin high", __func__);
		gpio_direction_output(pdata->bd_msg_reset_n, 1);
	} else {
		pr_info("%s: hd6180 chip, set reset pin low", __func__);
		gpio_direction_output(pdata->bd_msg_reset_n, 0);
	}

	return strnlen(buff, size);
}

static ssize_t bd_msg_reset_n_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	unsigned int value = 0;
	struct hdbdspi_data* pdata = dev_get_drvdata(dev);

	value = gpio_get_value(pdata->bd_msg_reset_n);
	return sprintf(buff, "%d\n", value);
}

static DEVICE_ATTR(bd_msg_reset_n, S_IRUGO | S_IWUSR, bd_msg_reset_n_show,  bd_msg_reset_n_store);


static ssize_t bd_msg_download_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int value = 0;
	struct hdbdspi_data* pdata = dev_get_drvdata(dev);

	sscanf(buff, "%u", &value);

	if (value) {
		pr_info("%s: hd6180 chip, set download/prtrg pin high", __func__);
		gpio_direction_output(pdata->bd_msg_download, 1);
	} else {
		pr_info("%s: hd6180 chip, set download/prtrg pin low", __func__);
		gpio_direction_output(pdata->bd_msg_download, 0);
	}

	return strnlen(buff, size);
}

static ssize_t bd_msg_download_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	unsigned int value = 0;
	struct hdbdspi_data* pdata = dev_get_drvdata(dev);

	value = gpio_get_value(pdata->bd_msg_download);
	return sprintf(buff, "%d\n", value);
}

static DEVICE_ATTR(bd_msg_download, S_IRUGO | S_IWUSR, bd_msg_download_show,  bd_msg_download_store);


static struct attribute* hdbdspi_attrs[] = {
	&dev_attr_bd_msg_1p8_en.attr,
	&dev_attr_bd_msg_reset_n.attr,
	&dev_attr_bd_msg_download.attr,
	NULL,
};

static struct attribute_group hdbdspi_attr_grp = {
	.name = "hd6180",
	.attrs = hdbdspi_attrs
};

/*-------------------------------------------------------------------------*/

static int hdbdspi_parse_dt(struct device *dev, struct hdbdspi_data *pdata)
{
	struct device_node* np = dev->of_node;

	if (!pdata) {
		dev_err(dev, "%s rpcom_pdata is null \n", __func__);
		return -ENOMEM;
	}

	if (!np) {
		dev_err(dev, "%s : get num err.\n", __func__);
		return -EINVAL;
	}

#define GET_GPIO(name, var) \
		do { \
			pdata->var = of_get_named_gpio(dev->of_node, name, 0); \
			pr_info("%s: %s = %d\n", __func__, name, pdata->var); \
			if (gpio_is_valid(pdata->var)) { \
				if (devm_gpio_request(dev, pdata->var, name)) { \
					dev_err(dev, "failed to request %s gpio!\n", name); \
					return -EINVAL; \
				} \
			} \
		} while (0)

		GET_GPIO("bd_msg_1p8_en", bd_msg_1p8_en);
		GET_GPIO("bd_msg_reset_n", bd_msg_reset_n);
		GET_GPIO("bd_msg_download", bd_msg_download);
		GET_GPIO("bd_msg_int", bd_msg_int);
#undef GET_GPIO

	return 0;
}


/* Change CS_TIME for hd6180 spi */
static int hdbdspi_probe(struct spi_device *spi)
{
	struct hdbdspi_data *hdbdspi;
	int status;
	int ret;
	unsigned long minor;

	/*
	 * hdbdspi should never be referenced in DT without a specific
	 * compatible string, it is a Linux implementation thing
	 * rather than a description of the hardware.
	 */
	pr_info("%s: hdbdspi driver probe enter\n",__func__);

	hdbdspi_probe_acpi(spi);

	/* Allocate driver data */
	hdbdspi = kzalloc(sizeof(*hdbdspi), GFP_KERNEL);
	if (!hdbdspi)
		return -ENOMEM;

	/* Initialize the driver data */
	hdbdspi->spi = spi;
	spin_lock_init(&hdbdspi->spi_lock);
	mutex_init(&hdbdspi->buf_lock);

	INIT_LIST_HEAD(&hdbdspi->device_entry);

    /* If we can allocate a minor number, hook up this device.
     * Reusing minors is fine so long as udev or mdev is working.
     */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		hdbdspi->devt = MKDEV(spidev_major, minor);
		dev = device_create(hdbdspi_class, &spi->dev, hdbdspi->devt,
				    // spidev, "spidev%d.%d",
				    // spi->master->bus_num, spi->chip_select);
				    hdbdspi, "hdbdspi");
		status = PTR_ERR_OR_ZERO(dev);
	} else {
		dev_err(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&hdbdspi->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);

	hdbdspi->speed_hz = spi->max_speed_hz;
	dev_info(&spi->dev, "hdbdspi->speed_hz=%d\n", hdbdspi->speed_hz);
	// {
	// /* fixed SPI clock speed: 109200000 */
	// int period = DIV_ROUND_UP(109200000, hdbdspi->speed_hz);

	// hdbdspi_chip_info.cs_idletime = period;
	// hdbdspi_chip_info.cs_holdtime = period;
	// }

	hdbdspi->spi->cs_setup.unit = SPI_DELAY_UNIT_USECS;
	hdbdspi->spi->cs_setup.value = 20;

	if (status == 0)
		spi_set_drvdata(spi, hdbdspi);
	else
		kfree(hdbdspi);

	(void)hdbdspi_parse_dt(&spi->dev, hdbdspi);

	hdbdspi->kobj = kobject_create_and_add("hdbd_satel", kernel_kobj);
	ret = sysfs_create_group(hdbdspi->kobj, &hdbdspi_attr_grp);
	if (ret) {
		pr_err("%s: failed to create hdbdspi_attr_grp!", __func__);
	}

	pr_info("%s: hdbdspi probe complete\n", __func__);

	return status;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
static void hdbdspi_remove(struct spi_device *spi)
#else
static int hdbdspi_remove(struct spi_device *spi)
#endif
{
	struct hdbdspi_data *hdbdspi = spi_get_drvdata(spi);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&hdbdspi->spi_lock);
	hdbdspi->spi = NULL;
	hdbdspi->spi_reset = NULL;
	spin_unlock_irq(&hdbdspi->spi_lock);
	list_del(&hdbdspi->device_entry);
	device_destroy(hdbdspi_class, hdbdspi->devt);
	clear_bit(MINOR(hdbdspi->devt), minors);
	if (hdbdspi->users == 0) {
		kfree(hdbdspi->tx_buffer);
		kfree(hdbdspi->rx_buffer);
		kfree(hdbdspi->null_buffer);
		kfree(hdbdspi);
	}

	mutex_unlock(&device_list_lock);
	sysfs_remove_group(&spi->dev.kobj, &hdbdspi_attr_grp);

	return;
}

static struct spi_driver hdbdspi_spi_driver = {
	.driver =
		{
			.name = "hdbdspi",
			.of_match_table = of_match_ptr(hdbdspi_dt_ids),
			.acpi_match_table = ACPI_PTR(hdbdspi_acpi_ids),
		},
	.probe = hdbdspi_probe,
	.remove = hdbdspi_remove,

    /* NOTE:  suspend/resume methods are not necessary here.
     * We don't do anything except pass the requests to/from
     * the underlying controller.  The refrigerator handles
     * most issues; the controller driver handles the rest.
     */
};

/*-------------------------------------------------------------------------*/

static int __init hdbdspi_init(void)
{
	int status;

	pr_info("%s: Loading hdbdspi driver...\n", __func__);

	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	spidev_major =
		__register_chrdev(0, 0, N_SPI_MINORS, "spi", &hdbdspi_fops);
	pr_info("%s: Loading hdbdspi driver, major: %d\n", __func__, spidev_major);

#if (KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE)
	hdbdspi_class = class_create("hdbdspi");
#else
	hdbdspi_class = class_create(THIS_MODULE, "hdbdspi");
#endif

	if (IS_ERR(hdbdspi_class)) {
		unregister_chrdev(spidev_major, hdbdspi_spi_driver.driver.name);
		return PTR_ERR(hdbdspi_class);
	}

	status = spi_register_driver(&hdbdspi_spi_driver);
	if (status < 0) {
		class_destroy(hdbdspi_class);
		unregister_chrdev(spidev_major, hdbdspi_spi_driver.driver.name);
	}
	pr_info("%s: Loading hdbdspi driver: %d\n", __func__, status);
	return status;
}
module_init(hdbdspi_init);

static void __exit hdbdspi_exit(void)
{
	spi_unregister_driver(&hdbdspi_spi_driver);
	class_destroy(hdbdspi_class);
	unregister_chrdev(spidev_major, hdbdspi_spi_driver.driver.name);
}
module_exit(hdbdspi_exit);

MODULE_AUTHOR("ZTE boot team");
MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:hdbd_spi");
