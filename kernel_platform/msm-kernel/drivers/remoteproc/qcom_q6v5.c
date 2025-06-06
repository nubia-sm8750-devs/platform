// SPDX-License-Identifier: GPL-2.0
/*
 * Qualcomm Peripheral Image Loader for Q6V5
 *
 * Copyright (C) 2016-2018 Linaro Ltd.
 * Copyright (C) 2014 Sony Mobile Communications AB
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/interconnect.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/soc/qcom/qcom_aoss.h>
#include <linux/soc/qcom/smem.h>
#include <linux/soc/qcom/smem_state.h>
#include <linux/remoteproc.h>
#include <linux/delay.h>
#include <asm/timex.h>

#include "qcom_common.h"
#include "qcom_q6v5.h"
#include <trace/events/rproc_qcom.h>

#if IS_MODULE(CONFIG_VENDOR_ZLOG_RESET_REASON)
#include <linux/string.h>
#endif

#ifdef CONFIG_VENDOR_ZTE_DEV_MONITOR_SYSTEM
#include <linux/string.h>
#include "zlog_common.h"
#endif

/* Started by AICoder, pid:z0c4df4357e1d5f143e70b5760f5292d04d25f0b */
#if IS_MODULE(CONFIG_VENDOR_ZLOG_RESET_REASON)
#define CONST_TAG_M 'M'  // single char for modem
#define CONST_TAG_A 'A'   // single char for adsp
#define CONST_TAG_C 'C'   // single char for cdsp
#define CONST_TAG_S 'S'   // single char for slpi
#define SUBSYS_PANIC_STUB_U8 0x39  // char '9'
#define SUBSYS_PANIC_CLEAR_U8 0x32  // char '2' equal to its init val
static atomic_t ss_atomic_panic_intval = ATOMIC_INIT(SUBSYS_PANIC_STUB_U8);
u8 get_ss_panic_buf_byte(void) {
	return (u8)atomic_read(&ss_atomic_panic_intval);
}
EXPORT_SYMBOL(get_ss_panic_buf_byte);
void set_ss_panic_buf_byte(u8 val) {
	atomic_set(&ss_atomic_panic_intval, (int)val);
}
EXPORT_SYMBOL(set_ss_panic_buf_byte);
#endif
/* Ended by AICoder, pid:z0c4df4357e1d5f143e70b5760f5292d04d25f0b */
/* Started by AICoder, pid:ee74df7b45ab2cf14c22084fd0fcb21cb278509e */
#if IS_MODULE(CONFIG_VENDOR_ZLOG_RESET_REASON)
u8 get_ss_symbol_from_rproc_name(struct qcom_q6v5 *q6v5) {
	u8 ret = SUBSYS_PANIC_STUB_U8;
	if (q6v5 && q6v5->rproc && q6v5->rproc->name) {
		if (strstr(q6v5->rproc->name, "remoteproc-mss")) {
			ret = CONST_TAG_M;
		} else if (strstr(q6v5->rproc->name, "remoteproc-adsp")) {
			ret = CONST_TAG_A;
		} else if (strstr(q6v5->rproc->name, "remoteproc-slpi")) {
			ret = CONST_TAG_S;
		} else if (strstr(q6v5->rproc->name, "remoteproc-cdsp")) {
			ret = CONST_TAG_C;
		}
	}
	return ret;
}
#endif
/* Ended by AICoder, pid:ee74df7b45ab2cf14c22084fd0fcb21cb278509e */

#define Q6V5_LOAD_STATE_MSG_LEN	64
#define Q6V5_PANIC_DELAY_MS	200

#ifdef CONFIG_VENDOR_ZTE_DEV_MONITOR_SYSTEM
static struct zlog_client *zlog_q6v5_client = NULL;
static struct zlog_mod_info zlog_q6v5_dev = {
	.module_no = ZLOG_MODULE_SUBSYS,
	.name = "Q6V5",
	.device_name = "Q6V5",
	.ic_name = "Q6V5",
	.module_name = "subsys",
	.fops = NULL,
};
#endif

static int q6v5_load_state_toggle(struct qcom_q6v5 *q6v5, bool enable)
{
	int ret;

	if (!q6v5->qmp)
		return 0;

	ret = qmp_send(q6v5->qmp, "{class: image, res: load_state, name: %s, val: %s}",
		       q6v5->load_state, enable ? "on" : "off");
	if (ret)
		dev_err(q6v5->dev, "failed to toggle load state\n");

	return ret;
}

/**
 * qcom_q6v5_prepare() - reinitialize the qcom_q6v5 context before start
 * @q6v5:	reference to qcom_q6v5 context to be reinitialized
 *
 * Return: 0 on success, negative errno on failure
 */
int qcom_q6v5_prepare(struct qcom_q6v5 *q6v5)
{
	int ret;

	ret = icc_set_bw(q6v5->path, UINT_MAX, UINT_MAX);
	if (ret < 0) {
		dev_err(q6v5->dev, "failed to set bandwidth request\n");
		return ret;
	}

	ret = q6v5_load_state_toggle(q6v5, true);
	if (ret) {
		icc_set_bw(q6v5->path, 0, 0);
		return ret;
	}

	reinit_completion(&q6v5->start_done);
	reinit_completion(&q6v5->stop_done);

	q6v5->running = true;
	q6v5->handover_issued = false;

	enable_irq(q6v5->handover_irq);

	return 0;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_prepare);

/**
 * qcom_q6v5_unprepare() - unprepare the qcom_q6v5 context after stop
 * @q6v5:	reference to qcom_q6v5 context to be unprepared
 *
 * Return: 0 on success, 1 if handover hasn't yet been called
 */
int qcom_q6v5_unprepare(struct qcom_q6v5 *q6v5)
{
	disable_irq(q6v5->handover_irq);
	q6v5_load_state_toggle(q6v5, false);

	/* Disable interconnect vote, in case handover never happened */
	icc_set_bw(q6v5->path, 0, 0);

	return !q6v5->handover_issued;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_unprepare);

void qcom_q6v5_register_ssr_subdev(struct qcom_q6v5 *q6v5, struct rproc_subdev *ssr_subdev)
{
	q6v5->ssr_subdev = ssr_subdev;
}
EXPORT_SYMBOL(qcom_q6v5_register_ssr_subdev);

static void qcom_q6v5_crash_handler_work(struct work_struct *work)
{
	struct qcom_q6v5 *q6v5 = container_of(work, struct qcom_q6v5, crash_handler);
	struct rproc *rproc = q6v5->rproc;
	struct rproc_subdev *subdev;
	int votes;

	mutex_lock(&rproc->lock);
	votes = atomic_read(&rproc->power);
	if (votes == 0 || q6v5->crash_seq != q6v5->seq) {
		mutex_unlock(&rproc->lock);
		return;
	}

	rproc->state = RPROC_CRASHED;
	list_for_each_entry_reverse(subdev, &rproc->subdevs, node) {
		if (subdev->stop)
			subdev->stop(subdev, true);
	}

	mutex_unlock(&rproc->lock);

	/*
	 * Temporary workaround until ramdump userspace application calls
	 * sync() and fclose() on attempting the dump.
	 */
	msleep(100);
	panic("Panicking, remoteproc %s crashed\n", q6v5->rproc->name);
}

static irqreturn_t q6v5_wdog_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;
	size_t len;
	char *msg;

	/* Sometimes the stop triggers a watchdog rather than a stop-ack */
	if (!q6v5->running) {
		complete(&q6v5->stop_done);
		return IRQ_HANDLED;
	}

	dev_err(q6v5->dev, "rproc crash at cycle:%llu, recovery state: %s\n",
		get_cycles(),
		q6v5->rproc->recovery_disabled ? "disabled and lead to device crash" :
		"enabled and kick recovery process");

	q6v5->crash_seq = q6v5->seq;
	msg = qcom_smem_get(QCOM_SMEM_HOST_ANY, q6v5->crash_reason, &len);
	if (!IS_ERR(msg) && len > 0 && msg[0]) {
		dev_err(q6v5->dev, "watchdog received: %s\n", msg);
#ifdef CONFIG_VENDOR_ZTE_DEV_MONITOR_SYSTEM
		if (zlog_q6v5_client) {
			if (q6v5->rproc && q6v5->rproc->name &&
					strstr(q6v5->rproc->name, "remoteproc-mss")) {
				zlog_client_record(zlog_q6v5_client, "watchdog received: %s\n", msg);
				zlog_client_notify(zlog_q6v5_client, ZLOG_SUBSYS_MODEM_CRASH_ERROR_NO);
			}
		} else {
			dev_err(q6v5->dev, "can't report subsys watchdog info to zlog\n");
		}
#endif
        } else {
		dev_err(q6v5->dev, "watchdog without message\n");
        }

	/* Started by AICoder, pid:3917237cf3r32661460408a470d97a2644e5b662 */
#if IS_MODULE(CONFIG_VENDOR_ZLOG_RESET_REASON)
	set_ss_panic_buf_byte(get_ss_symbol_from_rproc_name(q6v5));
	dev_info(q6v5->dev, "ztedbg w byte %x\n", get_ss_panic_buf_byte());
#endif
	/* Ended by AICoder, pid:3917237cf3r32661460408a470d97a2644e5b662 */

	if (q6v5->crash_stack) {
		msg = qcom_smem_get(q6v5->smem_host_id, q6v5->crash_stack, &len);
		if (!IS_ERR(msg) && len > 0 && msg[0])
			dev_err(q6v5->dev, "%s\n", msg);
	}

	q6v5->running = false;

	trace_rproc_qcom_event(dev_name(q6v5->dev), "q6v5_wdog", msg);
	if (q6v5->ssr_subdev)
		qcom_notify_early_ssr_clients(q6v5->ssr_subdev);

	if (q6v5->rproc->recovery_disabled)
		schedule_work(&q6v5->crash_handler);
	else
		rproc_report_crash(q6v5->rproc, RPROC_WATCHDOG);

	return IRQ_HANDLED;
}

static irqreturn_t q6v5_fatal_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;
	size_t len;
	char *msg;

	if (!q6v5->running)
		return IRQ_HANDLED;

	dev_err(q6v5->dev, "rproc crash at cycle:%llu, recovery state: %s\n",
		get_cycles(),
		q6v5->rproc->recovery_disabled ? "disabled and lead to device crash" :
		"enabled and kick recovery process");

	q6v5->crash_seq = q6v5->seq;
	msg = qcom_smem_get(QCOM_SMEM_HOST_ANY, q6v5->crash_reason, &len);
	if (!IS_ERR(msg) && len > 0 && msg[0]) {
		dev_err(q6v5->dev, "fatal error received: %s\n", msg);
#ifdef CONFIG_VENDOR_ZTE_DEV_MONITOR_SYSTEM
		if (zlog_q6v5_client) {
			if (q6v5->rproc && q6v5->rproc->name &&
					strstr(q6v5->rproc->name, "remoteproc-mss")) {
				zlog_client_record(zlog_q6v5_client, "fatal error received: %s\n", msg);
				zlog_client_notify(zlog_q6v5_client, ZLOG_SUBSYS_MODEM_CRASH_ERROR_NO);
			}
		} else {
			dev_err(q6v5->dev, "can't report subsys fatal error info to zlog\n");
		}
#endif
        } else {
		dev_err(q6v5->dev, "fatal error without message\n");
        }

	/* Started by AICoder, pid:3917237cf3r32661460408a470d97a2644e5b662 */
#if IS_MODULE(CONFIG_VENDOR_ZLOG_RESET_REASON)
	set_ss_panic_buf_byte(get_ss_symbol_from_rproc_name(q6v5));
	dev_info(q6v5->dev, "ztedbg f byte %x\n", get_ss_panic_buf_byte());
#endif
	/* Ended by AICoder, pid:3917237cf3r32661460408a470d97a2644e5b662 */

	if (q6v5->crash_stack) {
		msg = qcom_smem_get(q6v5->smem_host_id, q6v5->crash_stack, &len);
		if (!IS_ERR(msg) && len > 0 && msg[0])
			dev_err(q6v5->dev, "%s\n", msg);
	}

	q6v5->running = false;

	trace_rproc_qcom_event(dev_name(q6v5->dev), "q6v5_fatal", msg);

	if (q6v5->ssr_subdev)
		qcom_notify_early_ssr_clients(q6v5->ssr_subdev);

	if (q6v5->rproc->recovery_disabled)
		schedule_work(&q6v5->crash_handler);
	else
		rproc_report_crash(q6v5->rproc, RPROC_FATAL_ERROR);

	return IRQ_HANDLED;
}

static irqreturn_t q6v5_ready_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;

	complete(&q6v5->start_done);

	return IRQ_HANDLED;
}

/**
 * qcom_q6v5_wait_for_start() - wait for remote processor start signal
 * @q6v5:	reference to qcom_q6v5 context
 * @timeout:	timeout to wait for the event, in jiffies
 *
 * qcom_q6v5_unprepare() should not be called when this function fails.
 *
 * Return: 0 on success, -ETIMEDOUT on timeout
 */
int qcom_q6v5_wait_for_start(struct qcom_q6v5 *q6v5, int timeout)
{
	int ret;
#if IS_MODULE(CONFIG_VENDOR_ZLOG_RESET_REASON)
	/* Started by AICoder, pid:7f631zec9feff0814c780bb2f0abcb2af848c855 */
	u8 check_val;
	u8 read_val;
	/* Ended by AICoder, pid:7f631zec9feff0814c780bb2f0abcb2af848c855 */
#endif

	ret = wait_for_completion_timeout(&q6v5->start_done, timeout);
	if (!ret)
		disable_irq(q6v5->handover_irq);

#if IS_MODULE(CONFIG_VENDOR_ZLOG_RESET_REASON)
	/* Started by AICoder, pid:7f631zec9feff0814c780bb2f0abcb2af848c855 */
	if (ret) {  // not timeout, start ready
		// check if clear ss tag in pmic reg
		check_val = get_ss_symbol_from_rproc_name(q6v5);
		read_val = get_ss_panic_buf_byte();
		if (read_val == check_val) {
			dev_info(q6v5->dev, "ztedbg clear ss panic tag %x\n", read_val);
			set_ss_panic_buf_byte(SUBSYS_PANIC_CLEAR_U8);
		} else {
			dev_info(q6v5->dev, "ztedbg skip clear ss panic tag %x %x\n", read_val, check_val);
		}
	} else {
		dev_info(q6v5->dev, "ztedbg ss recover timeout\n");
	}
	/* Ended by AICoder, pid:7f631zec9feff0814c780bb2f0abcb2af848c855 */
#endif

	return !ret ? -ETIMEDOUT : 0;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_wait_for_start);

static irqreturn_t q6v5_handover_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;

	if (q6v5->handover)
		q6v5->handover(q6v5);

	icc_set_bw(q6v5->path, 0, 0);

	q6v5->handover_issued = true;

	return IRQ_HANDLED;
}

static irqreturn_t q6v5_stop_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;

	complete(&q6v5->stop_done);

	return IRQ_HANDLED;
}

/**
 * qcom_q6v5_request_stop() - request the remote processor to stop
 * @q6v5:	reference to qcom_q6v5 context
 * @sysmon:	reference to the remote's sysmon instance, or NULL
 *
 * Return: 0 on success, negative errno on failure
 */
int qcom_q6v5_request_stop(struct qcom_q6v5 *q6v5, struct qcom_sysmon *sysmon)
{
	int ret;

	q6v5->running = false;

	/* Don't perform SMP2P dance if remote isn't running */
	if (qcom_sysmon_shutdown_acked(sysmon) || (q6v5->rproc->state != RPROC_RUNNING))
		return 0;

	qcom_smem_state_update_bits(q6v5->state,
				    BIT(q6v5->stop_bit), BIT(q6v5->stop_bit));

	ret = wait_for_completion_timeout(&q6v5->stop_done, 5 * HZ);

	qcom_smem_state_update_bits(q6v5->state, BIT(q6v5->stop_bit), 0);

	return ret == 0 ? -ETIMEDOUT : 0;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_request_stop);

/**
 * qcom_q6v5_panic() - panic handler to invoke a stop on the remote
 * @q6v5:	reference to qcom_q6v5 context
 *
 * Set the stop bit and sleep in order to allow the remote processor to flush
 * its caches etc for post mortem debugging.
 *
 * Return: 200ms
 */
unsigned long qcom_q6v5_panic(struct qcom_q6v5 *q6v5)
{
	qcom_smem_state_update_bits(q6v5->state,
				    BIT(q6v5->stop_bit), BIT(q6v5->stop_bit));

	return Q6V5_PANIC_DELAY_MS;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_panic);

/**
 * qcom_q6v5_init() - initializer of the q6v5 common struct
 * @q6v5:	handle to be initialized
 * @pdev:	platform_device reference for acquiring resources
 * @rproc:	associated remoteproc instance
 * @crash_reason: SMEM id for crash reason string, or 0 if none
 * @load_state: load state resource string
 * @handover:	function to be called when proxy resources should be released
 *
 * Return: 0 on success, negative errno on failure
 */
int qcom_q6v5_init(struct qcom_q6v5 *q6v5, struct platform_device *pdev,
		   struct rproc *rproc,  int crash_reason, int crash_stack,
		   unsigned int smem_host_id, const char *load_state,
		   void (*handover)(struct qcom_q6v5 *q6v5))
{
	int ret;

	q6v5->rproc = rproc;
	q6v5->dev = &pdev->dev;
	q6v5->crash_reason = crash_reason;
	q6v5->crash_stack = crash_stack;
	q6v5->smem_host_id = smem_host_id;
	q6v5->handover = handover;
	q6v5->ssr_subdev = NULL;

	init_completion(&q6v5->start_done);
	init_completion(&q6v5->stop_done);

	q6v5->wdog_irq = platform_get_irq_byname(pdev, "wdog");
	if (q6v5->wdog_irq < 0)
		return q6v5->wdog_irq;

	ret = devm_request_threaded_irq(&pdev->dev, q6v5->wdog_irq,
					NULL, q6v5_wdog_interrupt,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					"q6v5 wdog", q6v5);
	if (ret) {
		dev_err(&pdev->dev, "failed to acquire wdog IRQ\n");
		return ret;
	}

	q6v5->fatal_irq = platform_get_irq_byname(pdev, "fatal");
	if (q6v5->fatal_irq < 0)
		return q6v5->fatal_irq;

	ret = devm_request_threaded_irq(&pdev->dev, q6v5->fatal_irq,
					NULL, q6v5_fatal_interrupt,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					"q6v5 fatal", q6v5);
	if (ret) {
		dev_err(&pdev->dev, "failed to acquire fatal IRQ\n");
		return ret;
	}

#ifdef CONFIG_VENDOR_ZTE_DEV_MONITOR_SYSTEM
	zlog_q6v5_client = zlog_register_client(&zlog_q6v5_dev);
	if (!zlog_q6v5_client) {
		dev_err(q6v5->dev, "zlog failed to register client for q6v5\n");
	}
#endif

	q6v5->ready_irq = platform_get_irq_byname(pdev, "ready");
	if (q6v5->ready_irq < 0)
		return q6v5->ready_irq;

	ret = devm_request_threaded_irq(&pdev->dev, q6v5->ready_irq,
					NULL, q6v5_ready_interrupt,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					"q6v5 ready", q6v5);
	if (ret) {
		dev_err(&pdev->dev, "failed to acquire ready IRQ\n");
		return ret;
	}

	q6v5->handover_irq = platform_get_irq_byname(pdev, "handover");
	if (q6v5->handover_irq < 0)
		return q6v5->handover_irq;

	ret = devm_request_threaded_irq(&pdev->dev, q6v5->handover_irq,
					NULL, q6v5_handover_interrupt,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					"q6v5 handover", q6v5);
	if (ret) {
		dev_err(&pdev->dev, "failed to acquire handover IRQ\n");
		return ret;
	}
	disable_irq(q6v5->handover_irq);

	q6v5->stop_irq = platform_get_irq_byname(pdev, "stop-ack");
	if (q6v5->stop_irq < 0)
		return q6v5->stop_irq;

	ret = devm_request_threaded_irq(&pdev->dev, q6v5->stop_irq,
					NULL, q6v5_stop_interrupt,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					"q6v5 stop", q6v5);
	if (ret) {
		dev_err(&pdev->dev, "failed to acquire stop-ack IRQ\n");
		return ret;
	}

	q6v5->state = devm_qcom_smem_state_get(&pdev->dev, "stop", &q6v5->stop_bit);
	if (IS_ERR(q6v5->state)) {
		dev_err(&pdev->dev, "failed to acquire stop state\n");
		return PTR_ERR(q6v5->state);
	}

	q6v5->load_state = devm_kstrdup_const(&pdev->dev, load_state, GFP_KERNEL);
	q6v5->qmp = qmp_get(&pdev->dev);
	if (IS_ERR(q6v5->qmp)) {
		if (PTR_ERR(q6v5->qmp) != -ENODEV)
			return dev_err_probe(&pdev->dev, PTR_ERR(q6v5->qmp),
					     "failed to acquire load state\n");
		q6v5->qmp = NULL;
	} else if (!q6v5->load_state) {
		if (!load_state)
			dev_err(&pdev->dev, "load state resource string empty\n");

		qmp_put(q6v5->qmp);
		return load_state ? -ENOMEM : -EINVAL;
	}

	q6v5->path = devm_of_icc_get(&pdev->dev, NULL);
	if (IS_ERR(q6v5->path))
		return dev_err_probe(&pdev->dev, PTR_ERR(q6v5->path),
				     "failed to acquire interconnect path\n");

	INIT_WORK(&q6v5->crash_handler, qcom_q6v5_crash_handler_work);

	return 0;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_init);

/**
 * qcom_q6v5_deinit() - deinitialize the q6v5 common struct
 * @q6v5:	reference to qcom_q6v5 context to be deinitialized
 */
void qcom_q6v5_deinit(struct qcom_q6v5 *q6v5)
{
	qmp_put(q6v5->qmp);
}
EXPORT_SYMBOL_GPL(qcom_q6v5_deinit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Qualcomm Peripheral Image Loader for Q6V5");
