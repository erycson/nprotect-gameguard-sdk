#include "gg_contract.h"

static uint32_t read32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int gg_parse_footer(const uint8_t *b, size_t n, gg_footer *out) {
    uint32_t name, sig;
    size_t end, start, i;
    if (!b || !out || n < 16)
        return 0;
    end = n - 16;
    if (read32(b + end) != UINT32_C(0x32812622) || read32(b + end + 12) != UINT32_C(0x32812621))
        return 0;
    name = read32(b + end + 4);
    sig = read32(b + end + 8);
    if (sig != 64 || name < 2 || name > 256 || (size_t)name + sig > end)
        return 0;
    start = end - sig - name;
    if (b[start + name - 1] != 0)
        return 0;
    for (i = 0; i + 1 < name; ++i) {
        uint8_t c = b[start + i];
        if (!c || c > 127 || c == '/' || c == '\\' || c == ':')
            return 0;
    }
    if ((name == 2 && b[start] == '.') || (name == 3 && b[start] == '.' && b[start + 1] == '.'))
        return 0;
    out->payload_length = start;
    out->signed_length = start + name;
    out->name_offset = start;
    out->name_length = name;
    out->signature_offset = start + name;
    out->signature_length = sig;
    return 1;
}

uint32_t gg_crc32(const uint8_t *p, size_t n) {
    uint32_t c = UINT32_C(0xffffffff);
    size_t i;
    unsigned bit;
    if (!p && n)
        return 0;
    for (i = 0; i < n; ++i) {
        c ^= p[i];
        for (bit = 0; bit < 8; ++bit)
            c = (c >> 1) ^ ((0u - (c & 1u)) & UINT32_C(0xedb88320));
    }
    return ~c;
}
