#include "gg_launch.h"
#include "gg_startup.h"

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
    int valid = read_startup(argc, argv, &parameters);

    SecureZeroMemory(&parameters, sizeof parameters);
    if (!valid) {
        fputs("GameGuard: argumento de inicializacao invalido ou nao suportado.\n", stderr);
        return 1;
    }
    /* Do not signal readiness until update, monitor and IPC behavior are implemented. */
    fputs("GameGuard: inicializacao do monitor ainda nao implementada.\n", stderr);
    return 1;
}
