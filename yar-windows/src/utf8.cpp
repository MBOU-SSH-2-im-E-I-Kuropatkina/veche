#include "utf8.h"
#ifdef _WIN32
#  include <windows.h>
#endif

namespace veche {
namespace utf8 {

uint32_t decode(const std::string& s, size_t& i) {
    if (i >= s.size()) return 0;
    unsigned char c = (unsigned char)s[i];
    if (c < 0x80) { i += 1; return c; }
    uint32_t cp = 0xFFFD;
    if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
        cp = ((c & 0x1F) << 6) | ((unsigned char)s[i+1] & 0x3F);
        i += 2;
    } else if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) {
        cp = ((c & 0x0F) << 12) | (((unsigned char)s[i+1] & 0x3F) << 6)
             | ((unsigned char)s[i+2] & 0x3F);
        i += 3;
    } else if ((c & 0xF8) == 0xF0 && i + 3 < s.size()) {
        cp = ((c & 0x07) << 18) | (((unsigned char)s[i+1] & 0x3F) << 12)
             | (((unsigned char)s[i+2] & 0x3F) << 6)
             | ((unsigned char)s[i+3] & 0x3F);
        i += 4;
    } else {
        i += 1;
    }
    return cp;
}

void encode(uint32_t cp, std::string& out) {
    if (cp < 0x80) {
        out.push_back((char)cp);
    } else if (cp < 0x800) {
        out.push_back((char)(0xC0 | (cp >> 6)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back((char)(0xE0 | (cp >> 12)));
        out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    } else {
        out.push_back((char)(0xF0 | (cp >> 18)));
        out.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    }
}

size_t length(const std::string& s) {
    size_t i = 0, n = 0;
    while (i < s.size()) { decode(s, i); ++n; }
    return n;
}

} // namespace utf8
} // namespace veche