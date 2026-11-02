#include "gg_launch.h"

#define GG_LAUNCH_HEADER_BYTES 4u
#define GG_LAUNCH_MAX_PAYLOAD 2048u

static int hex_digit(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int hex_byte(const char *input, uint8_t *value) {
    int high = hex_digit(input[0]);
    int low = hex_digit(input[1]);
    if (high < 0 || low < 0)
        return 0;
    *value = (uint8_t)((high << 4) | low);
    return 1;
}

int gg_decode_launch_frame(const char *hex, uint8_t *output, size_t output_capacity,
                           size_t *output_length) {
    uint8_t header[GG_LAUNCH_HEADER_BYTES];
    uint8_t value, state;
    size_t chars = 0, length, i;

    if (!hex || !output || !output_length)
        return 0;
    *output_length = 0;
    while (chars <= 2u * (GG_LAUNCH_MAX_PAYLOAD + GG_LAUNCH_HEADER_BYTES) && hex[chars] != '\0') {
        if (hex_digit(hex[chars]) < 0)
            return 0;
        ++chars;
    }
    if (chars < 2u * GG_LAUNCH_HEADER_BYTES || (chars & 1u) ||
        chars > 2u * (GG_LAUNCH_MAX_PAYLOAD + GG_LAUNCH_HEADER_BYTES))
        return 0;

    for (i = 0; i < GG_LAUNCH_HEADER_BYTES; ++i)
        if (!hex_byte(hex + 2u * i, &header[i]))
            return 0;
    if (header[0] != 1u)
        return 0;

    state = (uint8_t)(((unsigned)header[1] * 3u + 1u) * 3u);
    length = ((size_t)(header[2] ^ (uint8_t)(header[1] * 3u + 0x65u)) << 8) |
             (size_t)(header[3] ^ (uint8_t)(state + 0x65u));
    if (length > GG_LAUNCH_MAX_PAYLOAD || output_capacity < length ||
        chars != 2u * (length + GG_LAUNCH_HEADER_BYTES))
        return 0;

    for (i = 0; i < length; ++i) {
        if (!hex_byte(hex + 2u * (i + GG_LAUNCH_HEADER_BYTES), &value))
            return 0;
        state = (uint8_t)(((unsigned)state + 1u) * 3u);
        output[i] = (uint8_t)(value ^ (uint8_t)(state + 0x65u));
    }
    *output_length = length;
    return 1;
}
