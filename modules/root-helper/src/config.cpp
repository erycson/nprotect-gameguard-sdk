#include "gg_config.h"
#include <string.h>

typedef struct {
    const uint8_t *data;
    size_t length;
} span;

static int space(uint8_t c) {
    return c == ' ' || c == '\t';
}
static span trim(span s) {
    while (s.length && space(*s.data)) {
        ++s.data;
        --s.length;
    }
    while (s.length && space(s.data[s.length - 1]))
        --s.length;
    return s;
}
static int equal_ascii(span s, const char *name) {
    size_t i;
    for (i = 0; i < s.length && name[i]; ++i) {
        unsigned char a = s.data[i], b = (unsigned char)name[i];
        if (a >= 'A' && a <= 'Z')
            a = (unsigned char)(a + 'a' - 'A');
        if (b >= 'A' && b <= 'Z')
            b = (unsigned char)(b + 'a' - 'A');
        if (a != b)
            return 0;
    }
    return i == s.length && name[i] == '\0';
}

int gg_parse_config(const uint8_t *ini, size_t length, gg_config *config) {
    gg_config parsed = {{0}, 0};
    size_t pos = 0;
    int in_section = 0, found_name = 0, found_no_update = 0;
    if (!ini || !config || !length || length > 65536u)
        return 0;
    while (pos < length) {
        size_t start = pos, i;
        span line, key, value;
        while (pos < length && ini[pos] != '\r' && ini[pos] != '\n') {
            if (!ini[pos])
                return 0;
            ++pos;
        }
        line.data = ini + start;
        line.length = pos - start;
        line = trim(line);
        while (pos < length && (ini[pos] == '\r' || ini[pos] == '\n'))
            ++pos;
        if (!line.length || line.data[0] == ';' || line.data[0] == '#')
            continue;
        if (line.data[0] == '[') {
            if (line.length < 2 || line.data[line.length - 1] != ']')
                return 0;
            line.data++;
            line.length -= 2;
            in_section = equal_ascii(trim(line), "GAMEMON");
            continue;
        }
        if (!in_section)
            continue;
        for (i = 0; i < line.length && line.data[i] != '='; ++i) {
        }
        if (i == line.length)
            continue;
        key.data = line.data;
        key.length = i;
        key = trim(key);
        value.data = line.data + i + 1;
        value.length = line.length - i - 1;
        value = trim(value);
        if (equal_ascii(key, "GAME_NAME")) {
            if (found_name || !value.length || value.length > GG_CONFIG_MAX_IDENTITY)
                return 0;
            for (i = 0; i < value.length; ++i) {
                uint8_t c = value.data[i];
                if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                      c == '_' || c == '-'))
                    return 0;
            }
            memcpy(parsed.game_name, value.data, value.length);
            found_name = 1;
        } else if (equal_ascii(key, "NO_UPDATE")) {
            if (found_no_update || value.length != 1 ||
                (value.data[0] != '0' && value.data[0] != '1'))
                return 0;
            parsed.no_update = value.data[0] == '1';
            found_no_update = 1;
        }
    }
    if (!found_name)
        return 0;
    *config = parsed;
    return 1;
}
