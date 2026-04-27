/* Started by AICoder, pid:v3b44u3b29w8ceb1478408248046672ff8218258 */
#include "../dsi/dsi_panel.h"

#ifdef CONFIG_ZTE_LCD_ZLOG
//#include <../../../../drivers/vendor/common/zlog/zlog_common/zlog_common.h>
#include <vendor/comdef/zlog_common_base.h>
static struct zlog_mod_info zlog_lcd_dev = {
	.module_no = ZLOG_MODULE_LCD,
	.name = "LCD",
	.device_name = "qcom_lcd",
	.ic_name = "dummy_lcd",
	.module_name = "QCOM",
	.fops = NULL,
};
void zte_zlog_lcd_client_init(struct dsi_panel *panel);
#endif
/* Ended by AICoder, pid:v3b44u3b29w8ceb1478408248046672ff8218258 */