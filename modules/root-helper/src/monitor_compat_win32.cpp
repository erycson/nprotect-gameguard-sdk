#include "gg_compat.h"
#include "gg_contract.h"
#include "gg_raii.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <tlhelp32.h>
#include <vector>
#include <wincrypt.h>

#include "monitor_forwarding.inc"

namespace {

constexpr DWORD kMonitorFileLimit = 32u * 1024u * 1024u;

// Restrict the compatibility handler to the precise signed monitor tested here.
bool known_monitor(const wchar_t *path) {
    static const std::array<BYTE, 32> expected{0x4b, 0x86, 0x10, 0xa8, 0x43, 0x27, 0x4c, 0x94,
                                               0x27, 0x17, 0x03, 0x67, 0x17, 0xb2, 0x59, 0xfc,
                                               0xe9, 0x99, 0x30, 0x7a, 0x0c, 0x65, 0xe3, 0x89,
                                               0x05, 0x00, 0x37, 0x40, 0x3d, 0x15, 0x4a, 0xf9};
    gg::FileHandle file{
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr)};
    if (!file)
        return false;
    const DWORD size = GetFileSize(file.get(), nullptr);
    if (!size || size > kMonitorFileLimit)
        return false;
    std::vector<BYTE> data(size);
    DWORD read = 0;
    gg_footer footer;
    if (!ReadFile(file.get(), data.data(), size, &read, nullptr) || read != size ||
        !gg_parse_footer(data.data(), size, &footer))
        return false;
    gg::CryptProvider provider;
    gg::CryptHash hash;
    if (!CryptAcquireContextW(provider.receive(), nullptr, nullptr, PROV_RSA_AES,
                              CRYPT_VERIFYCONTEXT) ||
        !CryptCreateHash(provider.get(), CALG_SHA_256, 0, 0, hash.receive()) ||
        !CryptHashData(hash.get(), data.data(), static_cast<DWORD>(footer.payload_length), 0))
        return false;
    std::array<BYTE, 32> digest;
    DWORD digest_size = static_cast<DWORD>(digest.size());
    return CryptGetHashParam(hash.get(), HP_HASHVAL, digest.data(), &digest_size, 0) &&
           digest_size == digest.size() &&
           std::memcmp(digest.data(), expected.data(), digest.size()) == 0;
}

DWORD find_monitor(DWORD game_pid) {
    gg::FileHandle snapshot{CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)};
    if (!snapshot)
        return 0;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof entry;
    DWORD pid = 0;
    if (Process32FirstW(snapshot.get(), &entry)) {
        do {
            if (entry.th32ParentProcessID == game_pid &&
                !_wcsicmp(entry.szExeFile, L"GameMon.des")) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot.get(), &entry));
    }
    return pid;
}

DWORD WINAPI monitor_worker(void *argument) {
    gg_monitor_compat *compat = static_cast<gg_monitor_compat *>(argument);
    while (WaitForSingleObject(compat->stop_event, 10) == WAIT_TIMEOUT) {
        const DWORD pid = find_monitor(compat->game_pid);
        if (pid && install_monitor_handler(pid, compat->monitor_path)) {
            InterlockedExchange(&compat->installed, 1);
            break;
        }
    }
    return 0;
}

} // namespace

int gg_monitor_compat_start(gg_monitor_compat *compat, uint32_t game_pid) {
    if (!compat || !game_pid)
        return 0;
    std::memset(compat, 0, sizeof *compat);
    compat->game_pid = game_pid;
    const DWORD length = GetModuleFileNameW(nullptr, compat->monitor_path, MAX_PATH);
    wchar_t *slash;
    if (!length || length >= MAX_PATH || !(slash = wcsrchr(compat->monitor_path, L'\\')))
        return 0;
    *slash = L'\0';
    if (wcslen(compat->monitor_path) + wcslen(L"\\GameGuard\\GameMon.des") >= MAX_PATH)
        return 0;
    wcscat(compat->monitor_path, L"\\GameGuard\\GameMon.des");
    if (!known_monitor(compat->monitor_path))
        return 0;
    compat->stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!compat->stop_event)
        return 0;
    compat->thread = CreateThread(nullptr, 0, monitor_worker, compat, 0, nullptr);
    if (!compat->thread) {
        CloseHandle(compat->stop_event);
        compat->stop_event = nullptr;
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
    std::memset(compat, 0, sizeof *compat);
}
