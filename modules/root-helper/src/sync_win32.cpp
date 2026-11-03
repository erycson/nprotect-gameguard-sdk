#include "gg_sync.h"
#include "gg_raii.h"

int gg_signal_ready(const gg_startup_parameters *startup, uint32_t wait_ms) {
    if (!startup || !startup->game_pid || !startup->startup_event)
        return 0;
    gg::NullHandle game{OpenProcess(SYNCHRONIZE, FALSE, startup->game_pid)};
    if (!game)
        return 0;
    if (WaitForSingleObject(game.get(), 0) != WAIT_TIMEOUT)
        return 0;
    HANDLE event = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(startup->startup_event));
    if (!SetEvent(event))
        return 0;
    // Keep this process alive while the SDK reads its in-memory CRC field.
    const DWORD state = WaitForSingleObject(game.get(), wait_ms);
    return state == WAIT_TIMEOUT || state == WAIT_OBJECT_0;
}
