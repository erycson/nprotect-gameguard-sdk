#include "gg_files.h"
#include "gg_config.h"
#include "gg_contract.h"
#include "gg_ini.h"
#include "gg_monitor.h"
#include "gg_raii.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <windows.h>

namespace {

constexpr unsigned long kFileLimit = 32u * 1024u * 1024u;

// Reads a whole file; an empty result means failure (valid files are non-empty).
std::vector<uint8_t> read_file(const char *path) {
    gg::CFile file{std::fopen(path, "rb")};
    if (!file)
        return {};
    long size;
    if (std::fseek(file.get(), 0, SEEK_END) || (size = std::ftell(file.get())) <= 0 ||
        static_cast<unsigned long>(size) > kFileLimit || std::fseek(file.get(), 0, SEEK_SET))
        return {};
    std::vector<uint8_t> data(static_cast<std::size_t>(size));
    if (std::fread(data.data(), 1, data.size(), file.get()) != data.size())
        return {};
    return data;
}

} // namespace

int gg_validate_local_files(const gg_startup_parameters *startup, const uint8_t *public_key,
                            size_t public_key_length, uint32_t expected_monitor_crc,
                            uint32_t crc_values[4]) {
    if (!startup || !public_key || !public_key_length || !crc_values)
        return 0;

    char executable[MAX_PATH];
    DWORD chars = GetModuleFileNameA(nullptr, executable, sizeof executable);
    if (!chars || chars >= sizeof executable)
        return 0;
    char *slash = std::strrchr(executable, '\\');
    if (!slash)
        return 0;
    *slash = '\0';

    char directory[MAX_PATH], filename[GG_MAX_GAME_NAME + 5u], path[MAX_PATH];
    if (static_cast<size_t>(std::snprintf(directory, sizeof directory, "%s\\GameGuard",
                                          executable)) >= sizeof directory ||
        static_cast<size_t>(std::snprintf(filename, sizeof filename, "%s.ini",
                                          startup->game_name)) >= sizeof filename ||
        static_cast<size_t>(std::snprintf(path, sizeof path, "%s\\%s", directory, filename)) >=
            sizeof path)
        return 0;

    uint32_t verified_crc[4];

    // Signed, encrypted INI for the configured identity.
    {
        const std::vector<uint8_t> file = read_file(path);
        if (file.empty() || file.size() > 65536u)
            return 0;
        std::vector<uint8_t> plain(file.size());
        size_t plain_length = 0;
        gg_config config;
        const bool ok =
            gg_read_signed_ini(file.data(), file.size(), filename, public_key, public_key_length,
                               plain.data(), plain.size(), &plain_length) &&
            gg_parse_config(plain.data(), plain_length, &config) &&
            !_stricmp(config.game_name, startup->game_name) && config.no_update;
        SecureZeroMemory(plain.data(), plain.size());
        if (!ok)
            return 0;
        verified_crc[0] = gg_crc32(file.data(), file.size());
    }

    // Signed monitor measured against the CRC embedded in this helper's image.
    {
        if (static_cast<size_t>(std::snprintf(path, sizeof path, "%s\\GameMon.des", directory)) >=
            sizeof path)
            return 0;
        const std::vector<uint8_t> file = read_file(path);
        if (file.empty() || !gg_validate_monitor(file.data(), file.size(), expected_monitor_crc,
                                                 public_key, public_key_length))
            return 0;
        verified_crc[1] = gg_crc32(file.data(), file.size());
    }

    // Remaining signed modules.
    static const char *const modules[] = {"ggscan.des", "npggNT.des"};
    for (size_t i = 0; i < sizeof modules / sizeof modules[0]; ++i) {
        if (static_cast<size_t>(
                std::snprintf(path, sizeof path, "%s\\%s", directory, modules[i])) >= sizeof path)
            return 0;
        const std::vector<uint8_t> file = read_file(path);
        if (file.empty() || !gg_verify_signature(file.data(), file.size(), modules[i], public_key,
                                                 public_key_length))
            return 0;
        verified_crc[i + 2] = gg_crc32(file.data(), file.size());
    }

    std::memcpy(crc_values, verified_crc, sizeof verified_crc);
    return 1;
}
