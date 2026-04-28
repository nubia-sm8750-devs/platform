#ifndef __VEB_PLATFORM_H__
#define __VEB_PLATFORM_H__

#include <linux/spi/spi.h>
#include <linux/device.h>

#ifdef CONFIG_VENDOR_ZTE_DEV_MONITOR_SYSTEM
#include "zlog_common.h"
#endif

#if		defined(CONFIG_ARCH_MT6572)
#define VEB_SPI_BUS_NUM					0
#elif	defined(CONFIG_ARCH_MT6755)
#define VEB_SPI_BUS_NUM					1
#elif	defined(CONFIG_ARCH_MT6735)
#define VEB_SPI_BUS_NUM					0
#else
#define VEB_SPI_BUS_NUM					0
#endif

#define VEB_SPI_CHIP_SELECT				0

#define WAKE_TIME						4

extern void veb_spi_wake_hardware(int count);
extern int veb_spi_wake(int count);
extern void veb_spi_reset(void);

extern int veb_done_wait_ready(void);
extern int veb_wait_ready(void);
extern int veb_rsa_wait_ready(int msUnit);
extern int veb_smartcard_wait_ready(int msUnit);

extern int veb_platform_init(struct spi_device *spi);
extern int veb_platform_exit(struct spi_device *spi);

#ifdef CONFIG_VENDOR_ZTE_DEV_MONITOR_SYSTEM
extern void veb_zlog_register(void);
extern void veb_zlog_unregister(void);
extern void veb_zlog_record_notify(int errorno);
#endif
#endif	/*__veb_platform_h__*/
