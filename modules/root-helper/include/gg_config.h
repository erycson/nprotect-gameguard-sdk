#ifndef GG_CONFIG_H
#define GG_CONFIG_H

#include <stddef.h>
#include <stdint.h>

#define GG_CONFIG_MAX_IDENTITY 31u

typedef struct {
    char game_name[GG_CONFIG_MAX_IDENTITY + 1u];
    int no_update;
} gg_config;

/* Parse a verified, decrypted INI. Reject duplicate or malformed required keys. */
int gg_parse_config(const uint8_t *ini, size_t length, gg_config *config);

#endif
