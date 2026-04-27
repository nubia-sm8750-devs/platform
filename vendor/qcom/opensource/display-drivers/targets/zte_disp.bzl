load("//msm-kernel:sun.bzl", "get_zte_board_name")
load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")

board_id = ["lotus","aston","qvnettles","qvpeony","qvburdock","qvkino","qvhodur"]
zte_disp_config = ["CONFIG_DRM_ZTE_DISP","CONFIG_ZTE_LCD_ZLOG"]
zte_disp_config_qvhodur = ["CONFIG_DRM_ZTE_DISP"]
zte_disp_feature = ["CONFIG_DRM_ZTE_DISP_FOD"]
zte_disp_feature_aston = ["CONFIG_DRM_ZTE_DISP_FOD", "CONFIG_DRM_ZTE_DISP_LTM"]
zte_disp_feature_qvkino = ["CONFIG_DRM_ZTE_DISP_FOD", "CONFIG_DRM_ZTE_DISP_LTM"]
zte_disp_feature_qvhodur = ["CONFIG_DRM_ZTE_DISP_FOD", "CONFIG_DRM_ZTE_DISP_DUAL_PANEL", "CONFIG_DRM_ZTE_DISP_LTPO"]
zte_disp_srcs = {
    "CONFIG_DRM_ZTE_DISP" : [
        "msm/zte_disp/zte_disp_panel.c",
        "msm/zte_disp/zte_disp_panel_info.c",
        "msm/zte_disp/zte_disp_feature.c",
        "msm/zte_disp/zte_disp_backlight.c",
        "msm/zte_disp/zte_disp_work.c",
        "msm/zte_disp/zte_disp_layer.c",
        "msm/zte_disp/zte_disp_sync_frame.c",
        "msm/zte_disp/zte_lcd_reg_debug.c",
        "msm/zte_disp/zte_disp_pm.c",
        "msm/zte_disp/zte_disp_i2c.c"],
    "CONFIG_ZTE_LCD_ZLOG" : [
        "msm/zte_disp/zte_disp_zlog.c"]
}

# qvcork
zte_board_qvcork = "qvcork"
zte_disp_config_qvcork = ["CONFIG_DRM_ZTE_DISP_QVCORK"]
zte_disp_feature_qvcork = ["CONFIG_DRM_ZTE_DISP_DUAL"]
zte_disp_srcs_qvcork = {
    "CONFIG_DRM_ZTE_DISP_QVCORK" : [
        "msm/zte_dual_disp/zte_disp_panel.c",
        "msm/zte_dual_disp/zte_lcd_reg_debug.c",
        "msm/zte_dual_disp/zte_disp_panel_info.c",
        "msm/zte_dual_disp/zte_disp_feature.c",
        "msm/zte_dual_disp/zte_disp_backlight.c",
        "msm/zte_dual_disp/zte_disp_work.c"
       ]
}

def get_zte_disp_bulid_srcs():
    if get_zte_board_name() in board_id:
        return zte_disp_srcs
    else:
        if get_zte_board_name() == zte_board_qvcork:
            return zte_disp_srcs_qvcork
    return {}

def get_zte_disp_common_config():
    if get_zte_board_name() in board_id:
        if get_zte_board_name() == board_id[6]:
            return zte_disp_config_qvhodur
        else:
            return zte_disp_config
    else:
        if get_zte_board_name() == zte_board_qvcork:
            return zte_disp_config_qvcork
    return []

def get_zte_disp_feature():
    if get_zte_board_name() == board_id[0]:
        return zte_disp_feature
    elif get_zte_board_name() == board_id[1]:
        return zte_disp_feature_aston
    elif get_zte_board_name() == board_id[2]:
        return zte_disp_feature
    elif get_zte_board_name() == board_id[3]:
        return zte_disp_feature
    elif get_zte_board_name() == board_id[4]:
        return zte_disp_feature
    elif get_zte_board_name() == board_id[5]:
        return zte_disp_feature_qvkino
    elif get_zte_board_name() == board_id[6]:
        return zte_disp_feature_qvhodur
    elif get_zte_board_name() == zte_board_qvcork:
        return zte_disp_feature_qvcork
    else:
        return []


def get_zte_disp_hdr():
    if get_zte_board_name() in board_id:
        ddk_headers(
            name = "zte_disp",
            hdrs = native.glob([
            "msm/zte_disp/*.h",
            ]),
            includes = ["msm/zte_disp"]
        )
        return [":zte_disp"]
    else:
        if get_zte_board_name() == zte_board_qvcork:
            ddk_headers(
                name = "zte_dual_disp",
                hdrs = native.glob([
                "msm/zte_dual_disp/*.h",
                ]),
                includes = ["msm/zte_dual_disp"]
            )
            return [":zte_dual_disp"]
    return []