#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "debug.h"
#include "iliz_updater_client.h"
#include "settings.h"
#include "text.h"
#include "update.h"

#define TIMER_UPDATE   41
#define FIRST_CHECK_MS (60 * 1000)
#define NEXT_CHECK_MS  (60 * 60 * 1000)   // hourly tick; the updater's --if-due enforces 24h

void UiNotify(const wchar_t *title, const wchar_t *text);

static BOOL g_busy;
static wchar_t g_latest[64];

static void ExeDirFile(wchar_t *out, int cap, const wchar_t *name)
{
    out[0] = 0;
    if (!GetModuleFileNameW(0, out, (DWORD)cap))
        return;
    int n = LenW(out);
    while (n > 0 && out[n - 1] != L'\\')
        --n;
    out[n] = 0;
    AppendW(out, cap, name);
}

BOOL UpdateAvailable(void)
{
    wchar_t p[MAX_PATH];
    return IlizUpdaterPath(p, MAX_PATH);
}

const wchar_t *UpdateLatest(void)
{
    return g_latest;
}

void UpdateStart(HWND wnd)
{
    if (!UpdateAvailable()) {
        DebugLog("update", "iliz_updater.exe not found, updates disabled");
        return;
    }
    IlizUpdaterWatchQuit(wnd, WM_CLOSE);
    if (Settings()->auto_update != UPD_MODE_OFF)
        SetTimer(wnd, TIMER_UPDATE, FIRST_CHECK_MS, 0);
}

void UpdateStop(HWND wnd)
{
    KillTimer(wnd, TIMER_UPDATE);
}

BOOL UpdateOnTimer(HWND wnd, WPARAM id)
{
    if (id != TIMER_UPDATE)
        return FALSE;
    SetTimer(wnd, TIMER_UPDATE, NEXT_CHECK_MS, 0);
    if (!g_busy && IlizUpdaterCheckAsync(wnd, WM_APP_UPDATE, L"--if-due"))
        g_busy = TRUE;
    return TRUE;
}

// Remember the last version we showed a balloon for (update\notified, next to the updater's own files).
static BOOL AlreadyNotified(const wchar_t *ver)
{
    wchar_t path[MAX_PATH];
    ExeDirFile(path, MAX_PATH, L"update\\notified");
    char want[64], have[64] = "";
    WideToUtf8(ver, want, sizeof(want));
    HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (f != INVALID_HANDLE_VALUE) {
        DWORD got = 0;
        ReadFile(f, have, sizeof(have) - 1, &got, 0);
        have[got] = 0;
        CloseHandle(f);
    }
    if (EqA(have, want))
        return TRUE;
    wchar_t dir[MAX_PATH];
    ExeDirFile(dir, MAX_PATH, L"update");
    CreateDirectoryW(dir, 0);
    f = CreateFileW(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (f != INVALID_HANDLE_VALUE) {
        DWORD w = 0;
        WriteFile(f, want, (DWORD)LenA(want), &w, 0);
        CloseHandle(f);
    }
    return FALSE;
}

void UpdateOnResult(HWND wnd, WPARAM code, LPARAM info)
{
    (void)wnd;
    g_busy = FALSE;
    IlizUpdateInfo *u = (IlizUpdateInfo *)info;
    if (!u)
        return;
    DebugLog("update", "check exit=%d status=%s latest=%s", (int)code, u->status, u->latest);
    if (code == ILIZ_UPD_UPDATE_AVAILABLE && u->latest[0]) {
        Utf8ToWide(u->latest, g_latest, 64);
        if (Settings()->auto_update == UPD_MODE_AUTO) {
            DebugLog("update", "auto: installing %s", u->latest);
            IlizUpdaterLaunch(TRUE);
        } else if (!AlreadyNotified(g_latest)) {
            wchar_t text[160];
            CopyW(text, L"Version ", 160);
            AppendW(text, 160, g_latest);
            AppendW(text, 160, L" is available. Click to update.");
            UiNotify(L"Update available", text);
        }
    } else if (code == ILIZ_UPD_OK) {
        g_latest[0] = 0;
    }
    IlizUpdaterFree(u);
}

void UpdateCheckNow(void)
{
    // The interactive updater does the check, shows notes, and installs on request.
    IlizUpdaterLaunch(FALSE);
}

void UpdateInstall(void)
{
    IlizUpdaterLaunch(FALSE);
}
