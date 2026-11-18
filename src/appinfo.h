#pragma once

#define APP_VER_MAJOR 0
#define APP_VER_MINOR 1
#define APP_VER_PATCH 9

#define APP_VERSION_A   "0.1.9"
#define IDI_APP         1
#define IDI_APP_OFF     2
#define IDI_APP_MSG     3
#define IDR_CAMERA_WAV  101
#define APP_NAME_A      "iliz pc remote"
#define APP_COMPANY_A   "iliz"
#define APP_DESC_A      "Remote control for this PC"
#define APP_INTERNAL_A  "iliz_pc_remote"
#define APP_EXE_A       "iliz_pc_remote.exe"
#define APP_COPYRIGHT_A "Copyright (C) 2026 iliz"

#define APP_WIDEN2(s) L##s
#define APP_WIDEN(s)  APP_WIDEN2(s)

#define APP_VERSION APP_WIDEN(APP_VERSION_A)
#define APP_NAME    APP_WIDEN(APP_NAME_A)
#define APP_COMPANY APP_WIDEN(APP_COMPANY_A)
#define APP_DESC    APP_WIDEN(APP_DESC_A)
