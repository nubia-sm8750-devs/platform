/*
 * file: ztp_core_2nd.h
 *
 */
#ifndef _ZTP_CORE_2ND_H_
#define _ZTP_CORE_2ND_H_

#ifdef CONFIG_TOUCHSCREEN_LCD_NOTIFY_2nd
void lcd_notify_register_2nd(void);
void lcd_notify_unregister_2nd(void);
#endif

#ifdef CONFIG_TOUCHSCREEN_FTS_3383
int  fts_ts_init_2nd(void);
void  fts_ts_exit_2nd(void);
#endif

#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
int ufp_mac_init_2nd(void);
void  ufp_mac_exit_2nd(void);
#endif
bool tp_ghost_check_2nd(void);
void ghost_check_reset_2nd(void);
void tpd_clean_all_event_2nd(void);
int tpd_report_work_init_2nd(void);
void tpd_report_work_deinit_2nd(void);
void tpd_resume_work_init_2nd(void);
void tpd_resume_work_deinit_2nd(void);

#endif

