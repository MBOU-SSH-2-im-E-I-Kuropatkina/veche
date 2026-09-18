#include "errors.h"
#include <iostream>
#include "utf8.h"

namespace veche {

// Display a caret under column col (1-based, codepoint-aware).
static void printCaret(const std::string& line, int col) {
    std::string prefix;
    size_t i = 0;
    int cp = 1;
    while (i < line.size() && cp < col) {
        uint32_t c = utf8::decode(line, i);
        // approximate width: Cyrillic/ASCII = 1, wide = 2 (best effort)
        prefix += (c < 0x80) ? ' ' : ' ';
        ++cp;
    }
    std::cerr << "  > " << line << "\n";
    std::cerr << "     " << prefix << "^\n";
}

void printError(const VecheError& e) {
    std::cerr << "[Вече: " << e.kind << "] ";
    if (!e.file.empty()) std::cerr << e.file << ":" << e.line << ":" << e.col;
    std::cerr << "\n  " << e.message << "\n";
    if (!e.sourceLine.empty()) printCaret(e.sourceLine, e.col);
}

} // namespace veche