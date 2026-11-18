#pragma once

#define CMD_OK      0
#define CMD_NONE    1
#define CMD_FAIL    2
#define CMD_CONFIRM 3
#define CMD_SHOT    4

struct CmdOut {
    int status;
    char error[64];
    char reply[64];
};

void CmdRun(const char *text, CmdOut *out);
BOOL CmdCheck(const char *text, char *err, int cap);
