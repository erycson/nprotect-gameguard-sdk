#include "gg_metadata.h"
#include <stdlib.h>
#include <string.h>

#ifndef GG_METADATA_PATH
#define GG_METADATA_PATH L"SOFTWARE\\INCAInternet\\nProtectGameGuard\\Update"
#endif
static const wchar_t key_path[] = GG_METADATA_PATH;
static const wchar_t *const names[] = {L"INF HashValue", L"DLL HashValue", L"DLL HashValue 2",
                                       L"DLL HashValue 3"};

void gg_metadata_close(gg_metadata *metadata) {
    size_t i;
    if (!metadata)
        return;
    if (metadata->key) {
        for (i = 0; i < 4; ++i) {
            gg_metadata_value *v = &metadata->values[i];
            DWORD current = 0, size = sizeof current, type = 0;
            LONG rc =
                RegQueryValueExW(metadata->key, names[i], NULL, &type, (BYTE *)&current, &size);
            /* Do not overwrite a value changed by another process. */
            if (v->changed && (rc == ERROR_FILE_NOT_FOUND ||
                               (rc == ERROR_SUCCESS && type == REG_DWORD &&
                                size == sizeof current && current == v->written_value))) {
                if (v->existed)
                    RegSetValueExW(metadata->key, names[i], 0, v->old_type, v->old_data,
                                   v->old_size);
                else
                    RegDeleteValueW(metadata->key, names[i]);
            }
            free(v->old_data);
        }
        RegCloseKey(metadata->key);
        /* The key is retained: another process may have created values under it. */
    }
    memset(metadata, 0, sizeof *metadata);
}

int gg_metadata_open(gg_metadata *metadata, const uint32_t values[4]) {
    size_t i;
    if (!metadata || !values)
        return 0;
    memset(metadata, 0, sizeof *metadata);
    if (RegCreateKeyExW(HKEY_CURRENT_USER, key_path, 0, NULL, 0, KEY_QUERY_VALUE | KEY_SET_VALUE,
                        NULL, &metadata->key, &metadata->disposition) != ERROR_SUCCESS)
        return 0;
    for (i = 0; i < 4; ++i) {
        gg_metadata_value *v = &metadata->values[i];
        LONG rc = RegQueryValueExW(metadata->key, names[i], NULL, &v->old_type, NULL, &v->old_size);
        if (rc == ERROR_SUCCESS) {
            if (v->old_size > 1048576u)
                goto failure;
            v->old_data = (BYTE *)malloc(v->old_size ? v->old_size : 1u);
            if (!v->old_data || RegQueryValueExW(metadata->key, names[i], NULL, &v->old_type,
                                                 v->old_data, &v->old_size) != ERROR_SUCCESS)
                goto failure;
            v->existed = 1;
        } else if (rc != ERROR_FILE_NOT_FOUND) {
            goto failure;
        }
        v->written_value = values[i];
        if (RegSetValueExW(metadata->key, names[i], 0, REG_DWORD, (const BYTE *)&values[i],
                           sizeof values[i]) != ERROR_SUCCESS)
            goto failure;
        v->changed = 1;
    }
    return 1;
failure:
    gg_metadata_close(metadata);
    return 0;
}
