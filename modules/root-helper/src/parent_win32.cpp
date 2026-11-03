#include "gg_parent.h"
#include "gg_raii.h"

#include <cstdint>
#include <tlhelp32.h>

namespace {

constexpr LONG kStatusPortNotSet = static_cast<LONG>(0xc0000353u);
constexpr ULONG kProcessDebugObject = 30u;

using QueryProcessFn = LONG(NTAPI *)(HANDLE, ULONG, PVOID, ULONG, PULONG);
using RemoveDebugFn = LONG(NTAPI *)(HANDLE, HANDLE);

DWORD parent_pid() {
    gg::FileHandle snapshot{CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)};
    if (!snapshot)
        return 0;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof entry;
    DWORD parent = 0;
    if (Process32FirstW(snapshot.get(), &entry)) {
        do {
            if (entry.th32ProcessID == GetCurrentProcessId()) {
                parent = entry.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(snapshot.get(), &entry));
    }
    return parent;
}

} // namespace

int gg_prepare_parent(uint32_t expected_pid) {
    if (!expected_pid || parent_pid() != expected_pid)
        return 0;
    HMODULE module = GetModuleHandleW(L"ntdll.dll");
    if (!module)
        return 0;
    auto query = reinterpret_cast<QueryProcessFn>(
        reinterpret_cast<uintptr_t>(GetProcAddress(module, "NtQueryInformationProcess")));
    auto remove = reinterpret_cast<RemoveDebugFn>(
        reinterpret_cast<uintptr_t>(GetProcAddress(module, "NtRemoveProcessDebug")));
    if (!query || !remove)
        return 0;
    gg::NullHandle game{
        OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME, FALSE, expected_pid)};
    if (!game)
        return 0;
    gg::NullHandle debug_object;
    const LONG status =
        query(game.get(), kProcessDebugObject, debug_object.receive(), sizeof(HANDLE), nullptr);
    if (status == kStatusPortNotSet)
        return 1;
    if (status >= 0 && debug_object)
        return remove(game.get(), debug_object.get()) >= 0 ? 1 : 0;
    return 0;
}
