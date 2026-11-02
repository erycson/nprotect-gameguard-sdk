#ifndef GG_METADATA_H
#define GG_METADATA_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

typedef struct {
    BYTE *old_data;
    DWORD old_size, old_type, written_value;
    int existed, changed;
} gg_metadata_value;

typedef struct {
    HKEY key;
    DWORD disposition;
    gg_metadata_value values[4];
} gg_metadata;

/* Restores values on failure; the caller must close a successful transaction. */
int gg_metadata_open(gg_metadata *metadata, const uint32_t values[4]);
void gg_metadata_close(gg_metadata *metadata);

#endif
