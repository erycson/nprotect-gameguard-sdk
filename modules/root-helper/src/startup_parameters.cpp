#include "gg_startup.h"

#include <stdint.h>
#include <string.h>

typedef struct {
    const uint8_t *data;
    size_t length;
} field;

static int next_field(const uint8_t *payload, size_t length, size_t *position, field *result) {
    size_t start;

    while (*position < length && payload[*position] == ' ')
        ++*position;
    start = *position;
    while (*position < length && payload[*position] != ' ')
        ++*position;
    if (*position == start)
        return 0;
    result->data = payload + start;
    result->length = *position - start;
    return 1;
}

static int parse_uint32(field value, unsigned base, uint32_t *result) {
    uint32_t number = 0;
    size_t i;

    if (!value.length)
        return 0;
    for (i = 0; i < value.length; ++i) {
        unsigned digit;
        uint8_t c = value.data[i];
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (base == 16u && c >= 'a' && c <= 'f')
            digit = c - 'a' + 10u;
        else if (base == 16u && c >= 'A' && c <= 'F')
            digit = c - 'A' + 10u;
        else
            return 0;
        if (digit >= base || number > (UINT32_MAX - digit) / base)
            return 0;
        number = number * base + digit;
    }
    *result = number;
    return 1;
}

static int parse_game_name(field value, char *output) {
    size_t i;

    if (!value.length || value.length > GG_MAX_GAME_NAME)
        return 0;
    for (i = 0; i < value.length; ++i) {
        uint8_t c = value.data[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-'))
            return 0;
        output[i] = (char)c;
    }
    output[value.length] = '\0';
    return 1;
}

int gg_parse_startup_parameters(const uint8_t *payload, size_t length,
                                gg_startup_parameters *parameters) {
    gg_startup_parameters parsed = {};
    uint32_t *hex_fields[] = {&parsed.startup_event, &parsed.inherited_handle, &parsed.game_pid,
                              &parsed.extra_values[0], &parsed.extra_values[1]};
    size_t position = 0, i;
    field current;

    if (!payload || !parameters || length == 0 || length > 2048u)
        return 0;
    if (!next_field(payload, length, &position, &current) ||
        !parse_uint32(current, 10u, &parsed.initial_value) ||
        !next_field(payload, length, &position, &current) ||
        !parse_game_name(current, parsed.game_name))
        return 0;
    /* Legacy SDKs supply four hexadecimal fields; newer ones supply five. */
    for (i = 0; i < 4u; ++i) {
        if (!next_field(payload, length, &position, &current) ||
            !parse_uint32(current, 16u, hex_fields[i]))
            return 0;
    }
    while (position < length && payload[position] == ' ')
        ++position;
    if (position < length) {
        if (!next_field(payload, length, &position, &current) ||
            !parse_uint32(current, 16u, hex_fields[4]))
            return 0;
    }
    while (position < length && payload[position] == ' ')
        ++position;
    if (position != length)
        return 0;
    *parameters = parsed;
    return 1;
}
