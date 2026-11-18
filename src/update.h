#pragma once
#include <windows.h>

#define WM_APP_UPDATE (WM_APP + 4)

#define UPD_MODE_OFF    0
#define UPD_MODE_NOTIFY 1
#define UPD_MODE_AUTO   2

BOOL UpdateAvailable(void);                  // iliz_updater.exe is next to the exe
void UpdateStart(HWND wnd);                  // after the tray icon is up
void UpdateStop(HWND wnd);
BOOL UpdateOnTimer(HWND wnd, WPARAM id);     // TRUE if handled
void UpdateOnResult(HWND wnd, WPARAM code, LPARAM info);
void UpdateCheckNow(void);                   // tray "Check for updates"
void UpdateInstall(void);                    // tray "Update to X" / balloon click
const wchar_t *UpdateLatest(void);           // "" when no update is known
