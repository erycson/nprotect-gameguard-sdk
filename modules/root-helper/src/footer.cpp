#include "gg_contract.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace {

constexpr std::uint32_t read_le32(const std::uint8_t *p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

constexpr std::uint32_t kFooterTail = 0x32812622u;
constexpr std::uint32_t kFooterHead = 0x32812621u;
constexpr std::uint32_t kSignatureLength = 64u;
constexpr std::uint32_t kCrcPolynomial = 0xedb88320u;

} // namespace

int gg_parse_footer(const uint8_t *b, size_t n, gg_footer *out) {
    if (!b || !out || n < 16)
        return 0;
    const std::size_t end = n - 16;
    if (read_le32(b + end) != kFooterTail || read_le32(b + end + 12) != kFooterHead)
        return 0;
    const std::uint32_t name = read_le32(b + end + 4);
    const std::uint32_t sig = read_le32(b + end + 8);
    if (sig != kSignatureLength || name < 2 || name > 256 ||
        static_cast<std::size_t>(name) + sig > end)
        return 0;
    const std::size_t start = end - sig - name;
    if (b[start + name - 1] != 0)
        return 0;
    for (std::size_t i = 0; i + 1 < name; ++i) {
        const std::uint8_t c = b[start + i];
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
    if (!p && n)
        return 0;
    std::uint32_t c = 0xffffffffu;
    for (std::size_t i = 0; i < n; ++i) {
        c ^= p[i];
        for (int bit = 0; bit < 8; ++bit)
            c = (c >> 1) ^ ((0u - (c & 1u)) & kCrcPolynomial);
    }
    return ~c;
}
