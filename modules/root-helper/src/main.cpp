#include "gg_compat.h"
#include "gg_files.h"
#include "gg_launch.h"
#include "gg_metadata.h"
#include "gg_parent.h"
#include "gg_profile_process.h"
#include "gg_startup.h"
#include "gg_sync.h"
#include "public_key.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <windows.h>

namespace {

constexpr std::size_t kMaxStartupBytes = 2048u;

// A launch frame is never written to the console or persisted to disk.
bool read_startup(int argc, char **argv, gg_startup_parameters *parameters) {
    std::array<uint8_t, kMaxStartupBytes> payload;
    bool found = false;

    // The SDK supplies the encoded frame as the entire command line, including argv[0].
    for (int i = 0; i < argc; ++i) {
        if (std::strncmp(argv[i], "\\x01", 4) != 0)
            continue;
        std::size_t length;
        if (found || !gg_decode_launch_frame(argv[i], payload.data(), payload.size(), &length) ||
            !gg_parse_startup_parameters(payload.data(), length, parameters)) {
            SecureZeroMemory(payload.data(), payload.size());
            return false;
        }
        found = true;
    }
    SecureZeroMemory(payload.data(), payload.size());
    return found;
}

} // namespace

int main(int argc, char **argv) {
    gg_startup_parameters parameters = {};
    uint32_t crc_values[4] = {};

    if (!read_startup(argc, argv, &parameters)) {
        SecureZeroMemory(&parameters, sizeof parameters);
        std::fputs("GameGuard: argumento de inicializacao invalido ou nao suportado.\n", stderr);
        return 1;
    }
#if GG_PUBLIC_KEY_AVAILABLE
    const uint8_t *image = reinterpret_cast<const uint8_t *>(GetModuleHandleA(nullptr));
    if (!image) {
        SecureZeroMemory(&parameters, sizeof parameters);
        return 1;
    }
    uint32_t inverted_crc = 0;
    std::memcpy(&inverted_crc, image + 0x24, sizeof inverted_crc);
    bool valid = gg_validate_local_files(&parameters, gg_public_key_blob, sizeof gg_public_key_blob,
                                         ~inverted_crc, crc_values) != 0;
#else
    (void)gg_public_key_blob;
    bool valid = false;
#endif
    if (!valid) {
        SecureZeroMemory(&parameters, sizeof parameters);
        std::fputs("GameGuard: configuracao ou integridade do monitor invalida.\n", stderr);
        return 1;
    }
    if (gg_detect_process_sdk(parameters.game_pid).profile == GG_PROFILE_UNKNOWN) {
        SecureZeroMemory(&parameters, sizeof parameters);
        std::fputs("GameGuard: SDK desconhecido ou nao suportado nesta build.\n", stderr);
        return 1;
    }
    if (!gg_prepare_parent(parameters.game_pid)) {
        SecureZeroMemory(&parameters, sizeof parameters);
        std::fputs("GameGuard: falha ao preparar o processo do jogo.\n", stderr);
        return 1;
    }
    gg_metadata metadata;
    if (!gg_metadata_open(&metadata, crc_values)) {
        SecureZeroMemory(&parameters, sizeof parameters);
        std::fputs("GameGuard: falha ao preparar metadados locais.\n", stderr);
        return 1;
    }
    gg_monitor_compat compat;
    if (!gg_monitor_compat_start(&compat, parameters.game_pid)) {
        gg_metadata_close(&metadata);
        SecureZeroMemory(&parameters, sizeof parameters);
        std::fputs("GameGuard: monitor nao suportado.\n", stderr);
        return 1;
    }
    const bool signaled = gg_signal_ready(&parameters, 30000);
    gg_monitor_compat_stop(&compat);
    gg_metadata_close(&metadata);
    SecureZeroMemory(&parameters, sizeof parameters);
    if (!signaled) {
        std::fputs("GameGuard: falha ao sinalizar o SDK.\n", stderr);
        return 1;
    }
    // ExitProcess avoids C runtime (and C++ destructor) shutdown hooks that the active legacy
    // monitor may have modified; all cleanup above is therefore performed explicitly.
    ExitProcess(0x755);
}
