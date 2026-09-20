#pragma once
#include <stdexcept>
#include <string>

namespace veche {

// Base Veche error. All parser/analyzer/runtime errors derive from it.
struct VecheError : std::exception {
    std::string kind;    // e.g. "ОшибкаСинтаксиса"
    std::string message; // human-readable
    std::string file;
    int line = 0;
    int col  = 0;
    std::string sourceLine; // for caret display

    VecheError(std::string k, std::string m, std::string f = "",
               int ln = 0, int c = 0, std::string src = "")
        : kind(std::move(k)), message(std::move(m)), file(std::move(f)),
          line(ln), col(c), sourceLine(std::move(src)) {}

    const char* what() const noexcept override { return message.c_str(); }
};

// Runtime value thrown via `поднять` / `raise`.
struct VecheRaise {
    std::string kind;    // "ОшибкаТипа" etc.
    std::string message;
};

// Pretty-print a VecheError to stderr in the spec format.
void printError(const VecheError& e);

} // namespace veche