#ifndef GG_STARTUP_H
#define GG_STARTUP_H

#include <stddef.h>
#include <stdint.h>

#define GG_MAX_GAME_NAME 31u

/* The SDK formats these values before encoding the launch frame. */
typedef struct {
    uint32_t initial_value;
    char game_name[GG_MAX_GAME_NAME + 1u];
    uint32_t startup_event;
    uint32_t inherited_handle;
    uint32_t game_pid;
    uint32_t extra_values[2];
} gg_startup_parameters;

/* Parses the decoded v584 payload without modifying the input or logging it. */
int gg_parse_startup_parameters(const uint8_t *payload, size_t length,
                                gg_startup_parameters *parameters);

#endif
