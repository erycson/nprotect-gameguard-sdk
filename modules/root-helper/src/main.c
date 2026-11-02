#include "gg_launch.h"
#include "gg_metadata.h"
#include "gg_files.h"
#include "public_key.h"
#include "gg_startup.h"
#include "gg_sync.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#define GG_MAX_STARTUP_BYTES 2048u

/* A launch frame is never written to the console or persisted to disk. */
static int read_startup(int argc, char **argv, gg_startup_parameters *parameters) {
    uint8_t payload[GG_MAX_STARTUP_BYTES];
    size_t length;
    int i, found = 0;

    /* The SDK supplies the encoded frame as the entire command line, including argv[0]. */
    for (i = 0; i < argc; ++i) {
        if (strncmp(argv[i], "\\x01", 4) != 0)
            continue;
        if (found || !gg_decode_launch_frame(argv[i], payload, sizeof payload, &length) ||
            !gg_parse_startup_parameters(payload, length, parameters)) {
            SecureZeroMemory(payload, sizeof payload);
            return 0;
        }
        found = 1;
    }
    SecureZeroMemory(payload, sizeof payload);
    return found;
}

int main(int argc, char **argv) {
    gg_startup_parameters parameters = {0};
    gg_metadata metadata;
    uint32_t inverted_crc = 0, crc_values[4] = {0};
    const uint8_t *image;
    int valid = read_startup(argc, argv, &parameters);

    if (!valid) {
        SecureZeroMemory(&parameters, sizeof parameters);
        fputs("GameGuard: argumento de inicializacao invalido ou nao suportado.\n", stderr);
        return 1;
    }
#if GG_PUBLIC_KEY_AVAILABLE
    image = (const uint8_t *)GetModuleHandleA(NULL);
    if (!image) {
        SecureZeroMemory(&parameters, sizeof parameters);
        return 1;
    }
    memcpy(&inverted_crc, image + 0x24, sizeof inverted_crc);
    valid = gg_validate_local_files(&parameters, gg_public_key_blob, sizeof gg_public_key_blob,
                                    ~inverted_crc, crc_values);
#else
    (void)image;
    (void)inverted_crc;
    (void)gg_public_key_blob;
    valid = 0;
#endif
    if (!valid) {
        SecureZeroMemory(&parameters, sizeof parameters);
        fputs("GameGuard: configuracao ou integridade do monitor invalida.\n", stderr);
        return 1;
    }
    if (!gg_metadata_open(&metadata, crc_values)) {
        SecureZeroMemory(&parameters, sizeof parameters);
        fputs("GameGuard: falha ao preparar metadados locais.\n", stderr);
        return 1;
    }
    valid = gg_signal_ready(&parameters, 10000);
    gg_metadata_close(&metadata);
    SecureZeroMemory(&parameters, sizeof parameters);
    if (!valid) {
        fputs("GameGuard: falha ao sinalizar o SDK.\n", stderr);
        return 1;
    }
    return 0x755;
}
