#include "gg_sync.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int gg_signal_ready(const gg_startup_parameters *startup, uint32_t wait_ms) {
    HANDLE game, event;
    DWORD state;
    if (!startup || !startup->game_pid || !startup->startup_event)
        return 0;
    game = OpenProcess(SYNCHRONIZE, FALSE, startup->game_pid);
    if (!game)
        return 0;
    state = WaitForSingleObject(game, 0);
    if (state != WAIT_TIMEOUT) {
        CloseHandle(game);
        return 0;
    }
    event = (HANDLE)(uintptr_t)startup->startup_event;
    if (!SetEvent(event)) {
        CloseHandle(game);
        return 0;
    }
    /* Keep this process alive while the SDK reads its in-memory CRC field. */
    state = WaitForSingleObject(game, wait_ms);
    CloseHandle(game);
    return state == WAIT_TIMEOUT || state == WAIT_OBJECT_0;
}
