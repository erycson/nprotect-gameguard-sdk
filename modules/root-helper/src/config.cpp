#include "gg_config.h"

#include <cstddef>
#include <cstring>
#include <string_view>

namespace {

bool is_space(char c) {
    return c == ' ' || c == '\t';
}

std::string_view trim(std::string_view s) {
    while (!s.empty() && is_space(s.front()))
        s.remove_prefix(1);
    while (!s.empty() && is_space(s.back()))
        s.remove_suffix(1);
    return s;
}

bool equal_ascii_ci(std::string_view text, std::string_view name) {
    if (text.size() != name.size())
        return false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        unsigned char a = static_cast<unsigned char>(text[i]);
        unsigned char b = static_cast<unsigned char>(name[i]);
        if (a >= 'A' && a <= 'Z')
            a = static_cast<unsigned char>(a + 'a' - 'A');
        if (b >= 'A' && b <= 'Z')
            b = static_cast<unsigned char>(b + 'a' - 'A');
        if (a != b)
            return false;
    }
    return true;
}

bool valid_identity(std::string_view value) {
    for (char ch : value) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-'))
            return false;
    }
    return true;
}

} // namespace

int gg_parse_config(const uint8_t *ini, size_t length, gg_config *config) {
    if (!ini || !config || !length || length > 65536u)
        return 0;
    gg_config parsed{};
    const char *data = reinterpret_cast<const char *>(ini);
    std::size_t pos = 0;
    bool in_section = false, found_name = false, found_no_update = false;
    while (pos < length) {
        const std::size_t start = pos;
        while (pos < length && ini[pos] != '\r' && ini[pos] != '\n') {
            if (!ini[pos])
                return 0;
            ++pos;
        }
        const std::string_view line = trim({data + start, pos - start});
        while (pos < length && (ini[pos] == '\r' || ini[pos] == '\n'))
            ++pos;
        if (line.empty() || line.front() == ';' || line.front() == '#')
            continue;
        if (line.front() == '[') {
            if (line.size() < 2 || line.back() != ']')
                return 0;
            in_section = equal_ascii_ci(trim(line.substr(1, line.size() - 2)), "GAMEMON");
            continue;
        }
        if (!in_section)
            continue;
        const std::size_t eq = line.find('=');
        if (eq == std::string_view::npos)
            continue;
        const std::string_view key = trim(line.substr(0, eq));
        const std::string_view value = trim(line.substr(eq + 1));
        if (equal_ascii_ci(key, "GAME_NAME")) {
            if (found_name || value.empty() || value.size() > GG_CONFIG_MAX_IDENTITY ||
                !valid_identity(value))
                return 0;
            std::memcpy(parsed.game_name, value.data(), value.size());
            found_name = true;
        } else if (equal_ascii_ci(key, "NO_UPDATE")) {
            if (found_no_update || value.size() != 1 || (value[0] != '0' && value[0] != '1'))
                return 0;
            parsed.no_update = value[0] == '1';
            found_no_update = true;
        }
    }
    if (!found_name)
        return 0;
    *config = parsed;
    return 1;
}
