#ifndef GG_COMPAT_H
#define GG_COMPAT_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

typedef struct {
    HANDLE stop_event;
    HANDLE thread;
    DWORD game_pid;
    wchar_t monitor_path[MAX_PATH];
    volatile LONG installed;
} gg_monitor_compat;

/* Compatibility is limited to the verified v584 GameMon image. */
int gg_monitor_compat_start(gg_monitor_compat *compat, uint32_t game_pid);
void gg_monitor_compat_stop(gg_monitor_compat *compat);

#endif
