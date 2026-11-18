#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include "cmd.h"
#include "config.h"
#include "text.h"

#define CMD_MAX_STMT 32
#define CMD_MAX_ARGS 2

#define ACT_POS    0
#define ACT_CLICK  1
#define ACT_DOWN   2
#define ACT_UP     3
#define ACT_PING   4
#define ACT_SCROLL 5
#define ACT_TYPE   6
#define ACT_RUN    7
#define ACT_VOLUME 8
#define ACT_MEDIA  9
#define ACT_KEYS   10
#define ACT_SYS    11

#define SCROLL_MAX 120

#define MOUSE_LEFT  0
#define MOUSE_RIGHT 1

struct CmdSpec {
    const char *name;
    int argcMask;
    int act;
    int button;
    const char *value;
    BOOL alone;
};

struct Stmt {
    const CmdSpec *spec;
    int argc;
    int args[CMD_MAX_ARGS];
    BOOL explicitSign;
    const char *text;
};

static const CmdSpec kCmds[] = {
    { "ping", 1 << 0, ACT_PING, 0, 0, FALSE },
    { "setMousePos", 1 << 2, ACT_POS, 0, 0, FALSE },
    { "leftClick", (1 << 0) | (1 << 2), ACT_CLICK, MOUSE_LEFT, 0, FALSE },
    { "leftDown", (1 << 0) | (1 << 2), ACT_DOWN, MOUSE_LEFT, 0, FALSE },
    { "leftUp", (1 << 0) | (1 << 2), ACT_UP, MOUSE_LEFT, 0, FALSE },
    { "rightClick", (1 << 0) | (1 << 2), ACT_CLICK, MOUSE_RIGHT, 0, FALSE },
    { "rightDown", (1 << 0) | (1 << 2), ACT_DOWN, MOUSE_RIGHT, 0, FALSE },
    { "rightUp", (1 << 0) | (1 << 2), ACT_UP, MOUSE_RIGHT, 0, FALSE },
    { "scroll", 1 << 1, ACT_SCROLL, 0, 0, FALSE },
    { "type", 0, ACT_TYPE, 0, 0, FALSE },
    { "run", 0, ACT_RUN, 0, 0, FALSE },
    { "prev", 1 << 0, ACT_MEDIA, 0, "prev", FALSE },
    { "next", 1 << 0, ACT_MEDIA, 0, "next", FALSE },
    { "pause", 1 << 0, ACT_MEDIA, 0, "pause", FALSE },
    { "stop", 1 << 0, ACT_MEDIA, 0, "stop", FALSE },
    { "mute", 1 << 0, ACT_MEDIA, 0, "mute", FALSE },
    { "volumeUp", 1 << 0, ACT_MEDIA, 0, "vol_up", FALSE },
    { "volumeDown", 1 << 0, ACT_MEDIA, 0, "vol_down", FALSE },
    { "volume", (1 << 0) | (1 << 1), ACT_VOLUME, 0, 0, FALSE },
    { "keys", 0, ACT_KEYS, 0, 0, FALSE },
    { "lock", 1 << 0, ACT_SYS, 0, "lock", FALSE },
    { "sleep", 1 << 0, ACT_SYS, 0, "sleep", FALSE },
    { "displayOff", 1 << 0, ACT_SYS, 0, "display", FALSE },
    { "signout", 1 << 0, ACT_SYS, 0, "signout", FALSE },
    { "micMute", 1 << 0, ACT_SYS, 0, "mic", FALSE },
    { "beep", 1 << 0, ACT_SYS, 0, "beep", FALSE },
    { "shot", 1 << 0, ACT_SYS, 0, "shot", TRUE },
    { "shutdown", 1 << 0, ACT_SYS, 0, "shutdown", TRUE },
    { "restart", 1 << 0, ACT_SYS, 0, "restart", TRUE },
};

static void Fail(CmdOut *out, const char *msg)
{
    out->status = CMD_FAIL;
    CopyA(out->error, msg, sizeof(out->error));
}

static const char *SkipGap(const char *p)
{
    for (;;) {
        while (*p == ' ' || *p == '\t' || *p == '\r')
            ++p;
        if (*p != ';' && *p != '\n')
            return p;
        ++p;
    }
}

static char LowerA(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (char)(c - 'A' + 'a');
    return c;
}

static const CmdSpec *FindCmd(const char *name, int nlen)
{
    for (int i = 0; i < (int)(sizeof(kCmds) / sizeof(kCmds[0])); ++i) {
        const char *s = kCmds[i].name;
        int j = 0;
        while (j < nlen && s[j] && LowerA(name[j]) == LowerA(s[j]))
            ++j;
        if (j == nlen && !s[j])
            return &kCmds[i];
    }
    return 0;
}

static BOOL ParseInt(const char **pp, int *out, BOOL *explicitSign)
{
    const char *p = *pp;
    int sign = 1;
    BOOL marked = FALSE;
    if (*p == '+') {
        marked = TRUE;
        ++p;
    } else if (*p == '-') {
        marked = TRUE;
        sign = -1;
        ++p;
    }
    if (explicitSign)
        *explicitSign = marked;
    if (*p < '0' || *p > '9')
        return FALSE;
    long long v = 0;
    while (*p >= '0' && *p <= '9') {
        v = v * 10 + (*p - '0');
        if (v > 2147483647LL)
            return FALSE;
        ++p;
    }
    if (sign < 0)
        v = -v;
    *out = (int)v;
    *pp = p;
    return TRUE;
}

static BOOL ParseString(const char **pp, char *scratch, int cap, int *used, const char **out, BOOL path)
{
    const char *p = *pp;
    if (*p != '"')
        return FALSE;
    ++p;
    int start = *used;
    while (*p && *p != '"') {
        char c = *p++;
        if (c == '\\') {
            if (*p == '\\' || *p == '"')
                c = *p++;
            else if (!path && *p == 'n') {
                c = '\n';
                ++p;
            } else if (!path && *p == 't') {
                c = '\t';
                ++p;
            } else if (path && *p)
                c = '\\';
            else
                return FALSE;
        }
        if (*used >= cap - 1)
            return FALSE;
        scratch[(*used)++] = c;
    }
    if (*p != '"')
        return FALSE;
    ++p;
    scratch[(*used)++] = 0;
    *out = scratch + start;
    *pp = p;
    return TRUE;
}

static BOOL Parse(const char *text, Stmt *stmts, int *count, CmdOut *out, char *scratch, int scratchCap)
{
    const char *p = text ? text : "";
    int n = 0;
    int used = 0;
    out->status = CMD_OK;
    out->error[0] = 0;
    for (;;) {
        p = SkipGap(p);
        if (!*p)
            break;
        if (n >= CMD_MAX_STMT) {
            Fail(out, "bad command");
            return FALSE;
        }
        const char *name = p;
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || *p == '_')) {
            if (n == 0) {
                out->status = CMD_NONE;
                return FALSE;
            }
            Fail(out, "bad command");
            return FALSE;
        }
        ++p;
        while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
               (*p >= '0' && *p <= '9') || *p == '_')
            ++p;
        int nlen = (int)(p - name);
        while (*p == ' ' || *p == '\t')
            ++p;
        if (*p != '(') {
            if (n == 0) {
                out->status = CMD_NONE;
                return FALSE;
            }
            Fail(out, "bad command");
            return FALSE;
        }
        const CmdSpec *spec = FindCmd(name, nlen);
        if (!spec) {
            Fail(out, "unknown command");
            return FALSE;
        }
        ++p;
        Stmt *st = &stmts[n];
        st->spec = spec;
        st->argc = 0;
        st->explicitSign = FALSE;
        st->text = "";
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
            ++p;
        if (spec->act == ACT_TYPE || spec->act == ACT_RUN || spec->act == ACT_KEYS) {
            if (!ParseString(&p, scratch, scratchCap, &used, &st->text, spec->act == ACT_RUN)) {
                Fail(out, "bad args");
                return FALSE;
            }
            while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
                ++p;
            if (*p != ')') {
                Fail(out, "bad args");
                return FALSE;
            }
            if ((spec->act == ACT_RUN || spec->act == ACT_KEYS) && !st->text[0]) {
                Fail(out, "bad args");
                return FALSE;
            }
        } else if (*p != ')') {
            for (;;) {
                if (st->argc >= CMD_MAX_ARGS) {
                    Fail(out, "bad args");
                    return FALSE;
                }
                BOOL marked = FALSE;
                if (!ParseInt(&p, &st->args[st->argc], &marked)) {
                    Fail(out, "bad args");
                    return FALSE;
                }
                st->explicitSign = marked;
                ++st->argc;
                while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
                    ++p;
                if (*p == ')')
                    break;
                if (*p != ',') {
                    Fail(out, "bad args");
                    return FALSE;
                }
                ++p;
                while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
                    ++p;
            }
        }
        if (spec->act != ACT_TYPE && spec->act != ACT_RUN && spec->act != ACT_KEYS &&
            ((1 << st->argc) & spec->argcMask) == 0) {
            Fail(out, "bad args");
            return FALSE;
        }
        if (spec->act == ACT_SCROLL && (st->args[0] < -SCROLL_MAX || st->args[0] > SCROLL_MAX)) {
            Fail(out, "bad args");
            return FALSE;
        }
        if (spec->act == ACT_VOLUME && st->argc == 1) {
            int v = st->args[0];
            BOOL bad = st->explicitSign ? (v < -100 || v > 100) : (v < 0 || v > 100);
            if (bad) {
                Fail(out, "bad args");
                return FALSE;
            }
        }
        ++p;
        ++n;
        while (*p == ' ' || *p == '\t' || *p == '\r')
            ++p;
        if (*p && *p != ';' && *p != '\n') {
            Fail(out, "bad command");
            return FALSE;
        }
    }
    *count = n;
    if (n == 0) {
        out->status = CMD_NONE;
        return FALSE;
    }
    return TRUE;
}

static DWORD ButtonFlag(int button, BOOL up)
{
    if (button == MOUSE_RIGHT)
        return up ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_RIGHTDOWN;
    return up ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN;
}

static BOOL MouseInput(int button, int act, CmdOut *out)
{
    INPUT in[2];
    memset(in, 0, sizeof(in));
    int n = 0;
    if (act == ACT_CLICK || act == ACT_DOWN) {
        in[n].type = INPUT_MOUSE;
        in[n].mi.dwFlags = ButtonFlag(button, FALSE);
        ++n;
    }
    if (act == ACT_CLICK || act == ACT_UP) {
        in[n].type = INPUT_MOUSE;
        in[n].mi.dwFlags = ButtonFlag(button, TRUE);
        ++n;
    }
    if (SendInput((UINT)n, in, sizeof(INPUT)) != (UINT)n) {
        Fail(out, "mouse failed");
        return FALSE;
    }
    return TRUE;
}

static BOOL ScrollWheel(int notches, CmdOut *out)
{
    int step = notches > 0 ? 1 : -1;
    int left = notches > 0 ? notches : -notches;
    while (left > 0) {
        INPUT in;
        memset(&in, 0, sizeof(in));
        in.type = INPUT_MOUSE;
        in.mi.dwFlags = MOUSEEVENTF_WHEEL;
        in.mi.mouseData = (DWORD)(step * WHEEL_DELTA);
        if (SendInput(1, &in, sizeof(INPUT)) != 1) {
            Fail(out, "mouse failed");
            return FALSE;
        }
        --left;
    }
    return TRUE;
}

static BOOL KeyVk(WORD vk, CmdOut *out)
{
    INPUT in[2];
    memset(in, 0, sizeof(in));
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = vk;
    in[1].type = INPUT_KEYBOARD;
    in[1].ki.wVk = vk;
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;
    if (SendInput(2, in, sizeof(INPUT)) != 2) {
        Fail(out, "type failed");
        return FALSE;
    }
    return TRUE;
}

static BOOL TypeText(const char *text, CmdOut *out)
{
    if (!text || !text[0])
        return TRUE;
    wchar_t w[1024];
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, w, 1024);
    if (n <= 1) {
        Fail(out, "type failed");
        return FALSE;
    }
    --n;
    for (int i = 0; i < n; ++i) {
        if (w[i] == L'\r')
            continue;
        if (w[i] == L'\n') {
            if (!KeyVk(VK_RETURN, out))
                return FALSE;
            continue;
        }
        if (w[i] == L'\t') {
            if (!KeyVk(VK_TAB, out))
                return FALSE;
            continue;
        }
        INPUT in[2];
        memset(in, 0, sizeof(in));
        in[0].type = INPUT_KEYBOARD;
        in[0].ki.wScan = w[i];
        in[0].ki.dwFlags = KEYEVENTF_UNICODE;
        in[1].type = INPUT_KEYBOARD;
        in[1].ki.wScan = w[i];
        in[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        if (SendInput(2, in, sizeof(INPUT)) != 2) {
            Fail(out, "type failed");
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL StartsIW(const wchar_t *s, const wchar_t *prefix)
{
    while (*prefix) {
        wchar_t a = *s++;
        wchar_t b = *prefix++;
        if (a >= L'A' && a <= L'Z')
            a = (wchar_t)(a - L'A' + L'a');
        if (b >= L'A' && b <= L'Z')
            b = (wchar_t)(b - L'A' + L'a');
        if (a != b)
            return FALSE;
    }
    return TRUE;
}

static void CopyNW(wchar_t *dst, int cap, const wchar_t *src, int n)
{
    int i = 0;
    if (cap < 1)
        return;
    while (i < n && src[i] && i < cap - 1) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = 0;
}

static BOOL IsUrl(const wchar_t *s)
{
    return StartsIW(s, L"http://") || StartsIW(s, L"https://") || StartsIW(s, L"mailto:") || StartsIW(s, L"file:");
}

static BOOL IsFile(const wchar_t *s)
{
    DWORD attr = GetFileAttributesW(s);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static BOOL IsExisting(const wchar_t *s)
{
    return GetFileAttributesW(s) != INVALID_FILE_ATTRIBUTES;
}

static void SplitRun(const wchar_t *cmd, wchar_t *file, int fileCap, wchar_t *params, int paramsCap)
{
    params[0] = 0;
    if (cmd[0] == L'"') {
        const wchar_t *end = cmd + 1;
        while (*end && *end != L'"')
            ++end;
        CopyNW(file, fileCap, cmd + 1, (int)(end - (cmd + 1)));
        if (*end == L'"')
            ++end;
        while (*end == L' ' || *end == L'\t')
            ++end;
        CopyW(params, end, paramsCap);
        return;
    }
    if (IsUrl(cmd) || IsExisting(cmd)) {
        CopyW(file, cmd, fileCap);
        return;
    }
    const wchar_t *cut = 0;
    for (const wchar_t *p = cmd; *p; ++p) {
        if (*p != L' ' && *p != L'\t')
            continue;
        int n = (int)(p - cmd);
        wchar_t prefix[2048];
        if (n <= 0 || n >= 2048)
            continue;
        CopyNW(prefix, 2048, cmd, n);
        if (IsFile(prefix))
            cut = p;
    }
    if (cut) {
        CopyNW(file, fileCap, cmd, (int)(cut - cmd));
        while (*cut == L' ' || *cut == L'\t')
            ++cut;
        CopyW(params, cut, paramsCap);
        return;
    }
    const wchar_t *p = cmd;
    int i = 0;
    while (*p && *p != L' ' && *p != L'\t' && i < fileCap - 1)
        file[i++] = *p++;
    file[i] = 0;
    while (*p == L' ' || *p == L'\t')
        ++p;
    CopyW(params, p, paramsCap);
}

static BOOL RunTarget(const char *text, CmdOut *out)
{
    wchar_t src[2048];
    wchar_t expanded[2048];
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text ? text : "", -1, src, 2048);
    if (n <= 1) {
        Fail(out, "run failed");
        return FALSE;
    }
    wchar_t *cmd = src;
    while (*cmd == L' ' || *cmd == L'\t')
        ++cmd;
    if (!IsUrl(cmd)) {
        DWORD en = ExpandEnvironmentStringsW(cmd, expanded, 2048);
        if (en > 0 && en <= 2048)
            cmd = expanded;
    }
    while (*cmd == L' ' || *cmd == L'\t')
        ++cmd;
    if (!*cmd) {
        Fail(out, "bad args");
        return FALSE;
    }
    wchar_t file[2048];
    wchar_t params[2048];
    SplitRun(cmd, file, 2048, params, 2048);
    if (!file[0]) {
        Fail(out, "bad args");
        return FALSE;
    }
    SHELLEXECUTEINFOW info;
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_FLAG_NO_UI;
    info.lpVerb = L"open";
    info.lpFile = file;
    info.lpParameters = params[0] ? params : 0;
    info.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&info)) {
        Fail(out, "run failed");
        return FALSE;
    }
    return TRUE;
}

static BOOL Exec(const Stmt *st, CmdOut *out)
{
    if (st->spec->act == ACT_MEDIA) {
        ConfigExecMedia(st->spec->value);
        return TRUE;
    }
    if (st->spec->act == ACT_KEYS) {
        if (!ConfigExecKeys(st->text)) {
            Fail(out, "bad args");
            return FALSE;
        }
        return TRUE;
    }
    if (st->spec->act == ACT_SYS) {
        int kind = ConfigExecSystem(st->spec->value);
        if (kind == CFG_EXEC_CONFIRM) {
            out->status = CMD_CONFIRM;
            CopyA(out->reply, st->spec->value, sizeof(out->reply));
            return TRUE;
        }
        if (kind == CFG_EXEC_SHOT) {
            out->status = CMD_SHOT;
            return TRUE;
        }
        return TRUE;
    }
    if (st->spec->act == ACT_VOLUME) {
        char level[16];
        int mode = VOL_READ;
        level[0] = 0;
        if (st->argc == 1)
            mode = st->explicitSign ? VOL_STEP : VOL_SET;
        if (!ConfigVolume(st->argc == 1 ? st->args[0] : 0, mode, level, (int)sizeof(level))) {
            Fail(out, "volume failed");
            return FALSE;
        }
        CopyA(out->reply, level, sizeof(out->reply));
        return TRUE;
    }
    if (st->spec->act == ACT_PING) {
        CopyA(out->reply, "pong", sizeof(out->reply));
        return TRUE;
    }
    if (st->spec->act == ACT_RUN)
        return RunTarget(st->text, out);
    if (st->spec->act == ACT_TYPE)
        return TypeText(st->text, out);
    if (st->spec->act == ACT_SCROLL)
        return ScrollWheel(st->args[0], out);
    if (st->argc == 2 && !SetCursorPos(st->args[0], st->args[1])) {
        Fail(out, "set cursor failed");
        return FALSE;
    }
    if (st->spec->act == ACT_POS)
        return TRUE;
    return MouseInput(st->spec->button, st->spec->act, out);
}

void CmdRun(const char *text, CmdOut *out)
{
    Stmt stmts[CMD_MAX_STMT];
    char scratch[2048];
    int n = 0;
    if (!out)
        return;
    out->status = CMD_OK;
    out->error[0] = 0;
    out->reply[0] = 0;
    if (!Parse(text, stmts, &n, out, scratch, (int)sizeof(scratch)))
        return;
    int specials = 0;
    for (int i = 0; i < n; ++i) {
        if (stmts[i].spec->alone)
            ++specials;
    }
    if (specials && n != 1) {
        Fail(out, "bad command");
        return;
    }
    for (int i = 0; i < n; ++i) {
        if (!Exec(&stmts[i], out))
            return;
    }
    if (out->status == CMD_CONFIRM || out->status == CMD_SHOT)
        return;
    out->status = CMD_OK;
    out->error[0] = 0;
}

BOOL CmdCheck(const char *text, char *err, int cap)
{
    Stmt stmts[CMD_MAX_STMT];
    char scratch[2048];
    int n = 0;
    CmdOut out;
    BOOL ok = Parse(text, stmts, &n, &out, scratch, (int)sizeof(scratch));
    if (err && cap > 0)
        CopyA(err, ok ? "" : (out.error[0] ? out.error : "bad command"), cap);
    return ok;
}
