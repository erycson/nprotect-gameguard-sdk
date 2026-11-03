#include "gg_parent.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <string.h>

#define GG_STATUS_PORT_NOT_SET ((LONG)0xc0000353u)
#define GG_PROCESS_DEBUG_OBJECT 30u

typedef LONG(NTAPI *query_process_fn)(HANDLE, ULONG, PVOID, ULONG, PULONG);
typedef LONG(NTAPI *remove_debug_fn)(HANDLE, HANDLE);

static DWORD parent_pid(void) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W entry;
    DWORD parent = 0;
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;
    memset(&entry, 0, sizeof entry);
    entry.dwSize = sizeof entry;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (entry.th32ProcessID == GetCurrentProcessId()) {
                parent = entry.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return parent;
}

int gg_prepare_parent(uint32_t expected_pid) {
    HMODULE module;
    query_process_fn query;
    remove_debug_fn remove;
    HANDLE game, debug_object = NULL;
    LONG status;
    int success = 0;
    if (!expected_pid || parent_pid() != expected_pid)
        return 0;
    module = GetModuleHandleW(L"ntdll.dll");
    if (!module)
        return 0;
    query = (query_process_fn)(uintptr_t)GetProcAddress(module, "NtQueryInformationProcess");
    remove = (remove_debug_fn)(uintptr_t)GetProcAddress(module, "NtRemoveProcessDebug");
    if (!query || !remove)
        return 0;
    game = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME, FALSE, expected_pid);
    if (!game)
        return 0;
    status = query(game, GG_PROCESS_DEBUG_OBJECT, &debug_object, sizeof debug_object, NULL);
    if (status == GG_STATUS_PORT_NOT_SET) {
        success = 1;
    } else if (status >= 0 && debug_object) {
        success = remove(game, debug_object) >= 0;
    }
    if (debug_object)
        CloseHandle(debug_object);
    CloseHandle(game);
    return success;
}
