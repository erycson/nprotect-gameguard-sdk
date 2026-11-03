#include "gg_files.h"
#include "gg_config.h"
#include "gg_contract.h"
#include "gg_ini.h"
#include "gg_monitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define GG_FILE_LIMIT (32u * 1024u * 1024u)

static uint8_t *read_file(const char *path, size_t *length) {
    FILE *file = fopen(path, "rb");
    long size;
    uint8_t *data = NULL;
    if (!file)
        return NULL;
    if (fseek(file, 0, SEEK_END) || (size = ftell(file)) <= 0 ||
        (unsigned long)size > GG_FILE_LIMIT || fseek(file, 0, SEEK_SET))
        goto done;
    data = (uint8_t *)malloc((size_t)size);
    if (!data)
        goto done;
    if (fread(data, 1, (size_t)size, file) != (size_t)size) {
        free(data);
        data = NULL;
        goto done;
    }
    *length = (size_t)size;
done:
    fclose(file);
    return data;
}

int gg_validate_local_files(const gg_startup_parameters *startup, const uint8_t *public_key,
                            size_t public_key_length, uint32_t expected_monitor_crc,
                            uint32_t crc_values[4]) {
    char executable[MAX_PATH], directory[MAX_PATH], filename[GG_MAX_GAME_NAME + 5u];
    char path[MAX_PATH];
    uint8_t *file = NULL, *plain = NULL;
    size_t length = 0, plain_length = 0;
    DWORD chars;
    char *slash;
    gg_config config;
    int result = 0;
    uint32_t verified_crc[4];
    const char *modules[] = {"ggscan.des", "npggNT.des"};
    size_t i;

    if (!startup || !public_key || !public_key_length || !crc_values)
        return 0;
    chars = GetModuleFileNameA(NULL, executable, sizeof executable);
    if (!chars || chars >= sizeof executable)
        return 0;
    slash = strrchr(executable, '\\');
    if (!slash)
        return 0;
    *slash = '\0';
    if ((size_t)snprintf(directory, sizeof directory, "%s\\GameGuard", executable) >=
            sizeof directory ||
        (size_t)snprintf(filename, sizeof filename, "%s.ini", startup->game_name) >=
            sizeof filename ||
        (size_t)snprintf(path, sizeof path, "%s\\%s", directory, filename) >= sizeof path)
        return 0;
    file = read_file(path, &length);
    if (!file || length > 65536u)
        goto done;
    plain = (uint8_t *)malloc(length);
    if (!plain ||
        !gg_read_signed_ini(file, length, filename, public_key, public_key_length, plain, length,
                            &plain_length) ||
        !gg_parse_config(plain, plain_length, &config) ||
        _stricmp(config.game_name, startup->game_name) || !config.no_update)
        goto done;
    verified_crc[0] = gg_crc32(file, length);
    SecureZeroMemory(plain, length);
    free(plain);
    plain = NULL;
    free(file);
    file = NULL;
    if ((size_t)snprintf(path, sizeof path, "%s\\GameMon.des", directory) >= sizeof path)
        goto done;
    file = read_file(path, &length);
    if (!file)
        goto done;
    if (!gg_validate_monitor(file, length, expected_monitor_crc, public_key, public_key_length))
        goto done;
    verified_crc[1] = gg_crc32(file, length);
    free(file);
    file = NULL;
    for (i = 0; i < sizeof modules / sizeof modules[0]; ++i) {
        if ((size_t)snprintf(path, sizeof path, "%s\\%s", directory, modules[i]) >= sizeof path)
            goto done;
        file = read_file(path, &length);
        if (!file || !gg_verify_signature(file, length, modules[i], public_key, public_key_length))
            goto done;
        verified_crc[i + 2] = gg_crc32(file, length);
        free(file);
        file = NULL;
    }
    memcpy(crc_values, verified_crc, sizeof verified_crc);
    result = 1;
done:
    if (plain) {
        SecureZeroMemory(plain, length);
        free(plain);
    }
    free(file);
    return result;
}
