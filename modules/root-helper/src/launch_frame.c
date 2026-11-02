#include "gg_launch.h"

#define GG_LAUNCH_HEADER_BYTES 4u
#define GG_LAUNCH_MAX_PAYLOAD 2048u
#define GG_ENCODED_BYTE_CHARS 4u

static int hex_digit(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/* The v584 SDK writes each byte as the four characters backslash, x, and two hex digits. */
static int encoded_byte(const char *input, uint8_t *value) {
    int high, low;
    if (input[0] != '\\' || input[1] != 'x')
        return 0;
    high = hex_digit(input[2]);
    low = hex_digit(input[3]);
    if (high < 0 || low < 0)
        return 0;
    *value = (uint8_t)((high << 4) | low);
    return 1;
}

int gg_decode_launch_frame(const char *encoded, uint8_t *output, size_t output_capacity,
                           size_t *output_length) {
    uint8_t header[GG_LAUNCH_HEADER_BYTES];
    uint8_t value, state;
    size_t chars = 0, length, i;

    if (!encoded || !output || !output_length)
        return 0;
    *output_length = 0;
    while (chars <= GG_ENCODED_BYTE_CHARS * (GG_LAUNCH_MAX_PAYLOAD + GG_LAUNCH_HEADER_BYTES) &&
           encoded[chars] != '\0')
        ++chars;
    if (chars < GG_ENCODED_BYTE_CHARS * GG_LAUNCH_HEADER_BYTES || chars % GG_ENCODED_BYTE_CHARS ||
        chars > GG_ENCODED_BYTE_CHARS * (GG_LAUNCH_MAX_PAYLOAD + GG_LAUNCH_HEADER_BYTES))
        return 0;

    /* Validate the whole input before writing any decoded bytes. */
    for (i = 0; i < chars; i += GG_ENCODED_BYTE_CHARS)
        if (!encoded_byte(encoded + i, &value))
            return 0;
    for (i = 0; i < GG_LAUNCH_HEADER_BYTES; ++i)
        if (!encoded_byte(encoded + GG_ENCODED_BYTE_CHARS * i, &header[i]))
            return 0;
    if (header[0] != 1u)
        return 0;

    state = (uint8_t)(((unsigned)header[1] * 3u + 1u) * 3u);
    length = ((size_t)(header[2] ^ (uint8_t)(header[1] * 3u + 0x65u)) << 8) |
             (size_t)(header[3] ^ (uint8_t)(state + 0x65u));
    if (length > GG_LAUNCH_MAX_PAYLOAD || output_capacity < length ||
        chars != GG_ENCODED_BYTE_CHARS * (length + GG_LAUNCH_HEADER_BYTES))
        return 0;

    for (i = 0; i < length; ++i) {
        if (!encoded_byte(encoded + GG_ENCODED_BYTE_CHARS * (i + GG_LAUNCH_HEADER_BYTES), &value))
            return 0;
        state = (uint8_t)(((unsigned)state + 1u) * 3u);
        output[i] = (uint8_t)(value ^ (uint8_t)(state + 0x65u));
    }
    *output_length = length;
    return 1;
}
