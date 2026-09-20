#pragma once
#include <string>
#include <cstdint>

// UTF-8 utilities: decode next codepoint, encode codepoint.
// Windows-specific console setup lives in main.cpp under #ifdef _WIN32.

namespace veche {
namespace utf8 {

// Decode one UTF-8 codepoint starting at s[i]. Advances i past it.
// Returns U+FFFD on invalid sequence.
uint32_t decode(const std::string& s, size_t& i);

// Encode a codepoint to UTF-8, append to out.
void encode(uint32_t cp, std::string& out);

// Number of codepoints in a UTF-8 string.
size_t length(const std::string& s);

} // namespace utf8
} // namespace veche