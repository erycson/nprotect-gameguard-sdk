#include "gg_contract.h"
#include <string.h>

static uint32_t read32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

gg_detection gg_detect_sdk(const uint8_t *p, size_t n) {
    static const uint8_t a[] = {0x80, 0x39, 0x00, 0x74};
    static const uint8_t b[] = {0x8b, 0x44, 0x24, 0x04, 0x81, 0xc1};
    static const uint8_t c[] = {0x50, 0x68, 0x19, 0x06, 0x00, 0x00,
                                0x68, 0xb1, 0x08, 0x00, 0x00, 0xe8};
    static const uint8_t d[] = {0x56, 0x8b, 0xf1, 0x80, 0x3e, 0x00, 0x74};
    static const uint8_t e[] = {0x57, 0x8b, 0x7c, 0x24, 0x0c};
    static const uint8_t f[] = {0x8d, 0x8e, 0x5c, 0x10, 0x00, 0x00, 0x57, 0x68, 0x16,
                                0x06, 0x00, 0x00, 0x68, 0xb1, 0x08, 0x00, 0x00, 0xe8};
    gg_detection r = {GG_PROFILE_UNKNOWN, 0};
    gg_profile found = GG_PROFILE_UNKNOWN;
    size_t i, j;
    if (!p)
        return r;
    for (i = 0; i < n && n - i >= 34; ++i) {
        uint32_t offset;
        if (memcmp(p + i, a, sizeof a) || memcmp(p + i + 5, b, sizeof b) ||
            memcmp(p + i + 15, c, sizeof c) || memcmp(p + i + 31, "\xc2\x04\x00", 3))
            continue;
        offset = read32(p + i + 11);
        if (offset == 0x105c || offset == 0x1468) {
            found = offset == 0x105c ? GG_PROFILE_CSAUTH2_LEGACY : GG_PROFILE_CSAUTH2_MODERN;
            ++r.matches;
        }
    }
    /* Check both families: mixed candidates are ambiguous and must be rejected. */
    for (i = 0; i < n && n - i >= 96; ++i) {
        if (memcmp(p + i, d, sizeof d) || memcmp(p + i + 8, e, sizeof e))
            continue;
        for (j = i + 13; j < i + 78; ++j) {
            if (!memcmp(p + j, f, sizeof f)) {
                found = GG_PROFILE_CSAUTH1_LEGACY;
                ++r.matches;
                break;
            }
        }
    }
    if (r.matches == 1)
        r.profile = found;
    return r;
}
