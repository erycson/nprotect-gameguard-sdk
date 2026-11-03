#include "gg_contract.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

constexpr std::uint32_t read_le32(const std::uint8_t *p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

constexpr std::array<std::uint8_t, 4> kAuth2Prefix{0x80, 0x39, 0x00, 0x74};
constexpr std::array<std::uint8_t, 6> kAuth2Context{0x8b, 0x44, 0x24, 0x04, 0x81, 0xc1};
constexpr std::array<std::uint8_t, 12> kAuth2Command{0x50, 0x68, 0x19, 0x06, 0x00, 0x00,
                                                     0x68, 0xb1, 0x08, 0x00, 0x00, 0xe8};
constexpr std::array<std::uint8_t, 7> kAuth1Head{0x56, 0x8b, 0xf1, 0x80, 0x3e, 0x00, 0x74};
constexpr std::array<std::uint8_t, 5> kAuth1Enter{0x57, 0x8b, 0x7c, 0x24, 0x0c};
constexpr std::array<std::uint8_t, 18> kAuth1Command{0x8d, 0x8e, 0x5c, 0x10, 0x00, 0x00,
                                                     0x57, 0x68, 0x16, 0x06, 0x00, 0x00,
                                                     0x68, 0xb1, 0x08, 0x00, 0x00, 0xe8};

template <std::size_t N>
bool matches(const std::uint8_t *p, const std::array<std::uint8_t, N> &pattern) {
    return std::memcmp(p, pattern.data(), N) == 0;
}

} // namespace

gg_detection gg_detect_sdk(const uint8_t *p, size_t n) {
    gg_detection r{GG_PROFILE_UNKNOWN, 0};
    gg_profile found = GG_PROFILE_UNKNOWN;
    if (!p)
        return r;
    for (std::size_t i = 0; i < n && n - i >= 34; ++i) {
        if (!matches(p + i, kAuth2Prefix) || !matches(p + i + 5, kAuth2Context) ||
            !matches(p + i + 15, kAuth2Command) || std::memcmp(p + i + 31, "\xc2\x04\x00", 3))
            continue;
        const std::uint32_t offset = read_le32(p + i + 11);
        if (offset == 0x105c || offset == 0x1468) {
            found = offset == 0x105c ? GG_PROFILE_CSAUTH2_LEGACY : GG_PROFILE_CSAUTH2_MODERN;
            ++r.matches;
        }
    }
    // Check both families: mixed candidates are ambiguous and must be rejected.
    for (std::size_t i = 0; i < n && n - i >= 96; ++i) {
        if (!matches(p + i, kAuth1Head) || !matches(p + i + 8, kAuth1Enter))
            continue;
        for (std::size_t j = i + 13; j < i + 78; ++j) {
            if (matches(p + j, kAuth1Command)) {
                found = GG_PROFILE_CSAUTH1_LEGACY;
                ++r.matches;
                break;
            }
        }
    }
    if (r.matches == 1) {
#if GG_AUTH_MODE == GG_AUTH_CSAUTH1
        if (found == GG_PROFILE_CSAUTH1_LEGACY)
            r.profile = found;
#elif GG_AUTH_MODE == GG_AUTH_CSAUTH2
        if (found == GG_PROFILE_CSAUTH2_LEGACY || found == GG_PROFILE_CSAUTH2_MODERN)
            r.profile = found;
#else
        r.profile = found;
#endif
    }
    return r;
}
