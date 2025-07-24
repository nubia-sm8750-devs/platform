load("//msm-kernel:sun.bzl", "get_zte_board_name")
load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")

board_id = ["lotus","aston"]

zte_disp_feature = ["CONFIG_DRM_ZTE_DISP_FOD"]
zte_disp_feature_aston = ["CONFIG_DRM_ZTE_DISP_FOD", "CONFIG_DRM_ZTE_DISP_LTM"]

def get_zte_disp_feature():
    if get_zte_board_name() == board_id[0]:
        return zte_disp_feature
    elif get_zte_board_name() == board_id[1]:
        return zte_disp_feature_aston
    else:
        return []

