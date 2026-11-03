#include "gg_startup.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace {

// Splits the payload on spaces, mirroring the SDK's field tokenizer.
class FieldReader {
  public:
    FieldReader(const std::uint8_t *data, std::size_t length)
        : data_(reinterpret_cast<const char *>(data)), length_(length) {}

    bool next(std::string_view &field) {
        skip_spaces();
        const std::size_t start = pos_;
        while (pos_ < length_ && data_[pos_] != ' ')
            ++pos_;
        if (pos_ == start)
            return false;
        field = {data_ + start, pos_ - start};
        return true;
    }

    void skip_spaces() {
        while (pos_ < length_ && data_[pos_] == ' ')
            ++pos_;
    }

    bool at_end() const {
        return pos_ == length_;
    }

  private:
    const char *data_;
    std::size_t length_;
    std::size_t pos_ = 0;
};

bool parse_uint32(std::string_view value, unsigned base, std::uint32_t &result) {
    if (value.empty())
        return false;
    std::uint32_t number = 0;
    for (char ch : value) {
        const unsigned char c = static_cast<unsigned char>(ch);
        unsigned digit;
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (base == 16u && c >= 'a' && c <= 'f')
            digit = c - 'a' + 10u;
        else if (base == 16u && c >= 'A' && c <= 'F')
            digit = c - 'A' + 10u;
        else
            return false;
        if (digit >= base || number > (UINT32_MAX - digit) / base)
            return false;
        number = number * base + digit;
    }
    result = number;
    return true;
}

bool parse_game_name(std::string_view value, char *output) {
    if (value.empty() || value.size() > GG_MAX_GAME_NAME)
        return false;
    for (char ch : value) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-'))
            return false;
    }
    std::memcpy(output, value.data(), value.size());
    output[value.size()] = '\0';
    return true;
}

} // namespace

int gg_parse_startup_parameters(const uint8_t *payload, size_t length,
                                gg_startup_parameters *parameters) {
    if (!payload || !parameters || length == 0 || length > 2048u)
        return 0;
    gg_startup_parameters parsed{};
    const std::array<std::uint32_t *, 5> hex_fields{&parsed.startup_event, &parsed.inherited_handle,
                                                    &parsed.game_pid, &parsed.extra_values[0],
                                                    &parsed.extra_values[1]};
    FieldReader reader(payload, length);
    std::string_view field;
    if (!reader.next(field) || !parse_uint32(field, 10u, parsed.initial_value) ||
        !reader.next(field) || !parse_game_name(field, parsed.game_name))
        return 0;
    // Legacy SDKs supply four hexadecimal fields; newer ones supply five.
    for (std::size_t i = 0; i < 4u; ++i) {
        if (!reader.next(field) || !parse_uint32(field, 16u, *hex_fields[i]))
            return 0;
    }
    reader.skip_spaces();
    if (!reader.at_end()) {
        if (!reader.next(field) || !parse_uint32(field, 16u, *hex_fields[4]))
            return 0;
    }
    reader.skip_spaces();
    if (!reader.at_end())
        return 0;
    *parameters = parsed;
    return 1;
}
