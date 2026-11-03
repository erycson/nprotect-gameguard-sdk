#include "gg_compat.h"
#include "gg_contract.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>
#include <wchar.h>
#include <wincrypt.h>

#include "monitor_forwarding.inc"

/* Restrict the compatibility handler to the precise signed monitor tested here. */
static int known_monitor(const wchar_t *path) {
    static const BYTE expected[32] = {0x4b, 0x86, 0x10, 0xa8, 0x43, 0x27, 0x4c, 0x94,
                                      0x27, 0x17, 0x03, 0x67, 0x17, 0xb2, 0x59, 0xfc,
                                      0xe9, 0x99, 0x30, 0x7a, 0x0c, 0x65, 0xe3, 0x89,
                                      0x05, 0x00, 0x37, 0x40, 0x3d, 0x15, 0x4a, 0xf9};
    BYTE digest[32], *data = NULL;
    DWORD read, size, digest_size = sizeof digest;
    HANDLE file = INVALID_HANDLE_VALUE;
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    gg_footer footer;
    int ok = 0;
    file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
        goto done;
    size = GetFileSize(file, NULL);
    if (!size || size > 32u * 1024u * 1024u || !(data = (BYTE *)malloc(size)) ||
        !ReadFile(file, data, size, &read, NULL) || read != size ||
        !gg_parse_footer(data, size, &footer) ||
        !CryptAcquireContextW(&provider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) ||
        !CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash) ||
        !CryptHashData(hash, data, (DWORD)footer.payload_length, 0))
        goto done;
    if (CryptGetHashParam(hash, HP_HASHVAL, digest, &digest_size, 0) &&
        digest_size == sizeof digest && !memcmp(digest, expected, sizeof digest))
        ok = 1;
done:
    if (hash)
        CryptDestroyHash(hash);
    if (provider)
        CryptReleaseContext(provider, 0);
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);
    free(data);
    return ok;
}

static DWORD find_monitor(DWORD game_pid) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W entry;
    DWORD pid = 0;
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;
    memset(&entry, 0, sizeof entry);
    entry.dwSize = sizeof entry;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (entry.th32ParentProcessID == game_pid &&
                !_wcsicmp(entry.szExeFile, L"GameMon.des")) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return pid;
}

static DWORD WINAPI monitor_worker(void *argument) {
    gg_monitor_compat *compat = (gg_monitor_compat *)argument;
    while (WaitForSingleObject(compat->stop_event, 10) == WAIT_TIMEOUT) {
        DWORD pid = find_monitor(compat->game_pid);
        if (pid && install_monitor_handler(pid, compat->monitor_path)) {
            InterlockedExchange(&compat->installed, 1);
            break;
        }
    }
    return 0;
}

int gg_monitor_compat_start(gg_monitor_compat *compat, uint32_t game_pid) {
    wchar_t *slash;
    DWORD length;
    if (!compat || !game_pid)
        return 0;
    memset(compat, 0, sizeof *compat);
    compat->game_pid = game_pid;
    length = GetModuleFileNameW(NULL, compat->monitor_path, MAX_PATH);
    if (!length || length >= MAX_PATH || !(slash = wcsrchr(compat->monitor_path, L'\\')))
        return 0;
    *slash = L'\0';
    if (wcslen(compat->monitor_path) + wcslen(L"\\GameGuard\\GameMon.des") >= MAX_PATH)
        return 0;
    wcscat(compat->monitor_path, L"\\GameGuard\\GameMon.des");
    if (!known_monitor(compat->monitor_path))
        return 0;
    compat->stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!compat->stop_event)
        return 0;
    compat->thread = CreateThread(NULL, 0, monitor_worker, compat, 0, NULL);
    if (!compat->thread) {
        CloseHandle(compat->stop_event);
        compat->stop_event = NULL;
        return 0;
    }
    return 1;
}

void gg_monitor_compat_stop(gg_monitor_compat *compat) {
    if (!compat)
        return;
    if (compat->stop_event)
        SetEvent(compat->stop_event);
    if (compat->thread) {
        WaitForSingleObject(compat->thread, INFINITE);
        CloseHandle(compat->thread);
    }
    if (compat->stop_event)
        CloseHandle(compat->stop_event);
    memset(compat, 0, sizeof *compat);
}
