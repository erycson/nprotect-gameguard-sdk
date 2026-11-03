#include "gg_launch.h"

#include <cstddef>
#include <cstdint>

namespace {

constexpr std::size_t kHeaderBytes = 4u;
constexpr std::size_t kMaxPayload = 2048u;
constexpr std::size_t kEncodedByteChars = 4u;

constexpr int hex_digit(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

// The v584 SDK writes each byte as the four characters backslash, x and two hex digits.
bool encoded_byte(const char *input, std::uint8_t &value) {
    if (input[0] != '\\' || input[1] != 'x')
        return false;
    const int high = hex_digit(input[2]);
    const int low = hex_digit(input[3]);
    if (high < 0 || low < 0)
        return false;
    value = static_cast<std::uint8_t>((high << 4) | low);
    return true;
}

} // namespace

int gg_decode_launch_frame(const char *encoded, uint8_t *output, size_t output_capacity,
                           size_t *output_length) {
    if (!encoded || !output || !output_length)
        return 0;
    *output_length = 0;

    std::size_t chars = 0;
    while (chars <= kEncodedByteChars * (kMaxPayload + kHeaderBytes) && encoded[chars] != '\0')
        ++chars;
    if (chars < kEncodedByteChars * kHeaderBytes || chars % kEncodedByteChars ||
        chars > kEncodedByteChars * (kMaxPayload + kHeaderBytes))
        return 0;

    std::uint8_t value = 0;
    // Validate the whole input before writing any decoded bytes.
    for (std::size_t i = 0; i < chars; i += kEncodedByteChars)
        if (!encoded_byte(encoded + i, value))
            return 0;

    std::uint8_t header[kHeaderBytes];
    for (std::size_t i = 0; i < kHeaderBytes; ++i)
        if (!encoded_byte(encoded + kEncodedByteChars * i, header[i]))
            return 0;
    if (header[0] != 1u)
        return 0;

    std::uint8_t state =
        static_cast<std::uint8_t>((static_cast<unsigned>(header[1]) * 3u + 1u) * 3u);
    const std::size_t length =
        (static_cast<std::size_t>(header[2] ^ static_cast<std::uint8_t>(header[1] * 3u + 0x65u))
         << 8) |
        static_cast<std::size_t>(header[3] ^ static_cast<std::uint8_t>(state + 0x65u));
    if (length > kMaxPayload || output_capacity < length ||
        chars != kEncodedByteChars * (length + kHeaderBytes))
        return 0;

    for (std::size_t i = 0; i < length; ++i) {
        if (!encoded_byte(encoded + kEncodedByteChars * (i + kHeaderBytes), value))
            return 0;
        state = static_cast<std::uint8_t>((static_cast<unsigned>(state) + 1u) * 3u);
        output[i] = static_cast<std::uint8_t>(value ^ static_cast<std::uint8_t>(state + 0x65u));
    }
    *output_length = length;
    return 1;
}
