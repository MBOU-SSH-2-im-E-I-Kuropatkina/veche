#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace veche {

enum class TokKind {
    End, Newline, Indent, Dedent,
    Ident, Int, Double, String,
    LParen, RParen, LBracket, RBracket,
    Comma, Dot, Colon, Semicolon, Arrow,
    Plus, Minus, Star, Slash, Percent,
    Assign, Eq, Ne, Lt, Gt, Le, Ge,
    And, Or, Not,
    True, False,

    // типы
    KwTypeInt, KwTypeDouble, KwTypeString, KwTypeChar, KwTypeWord,
    KwTypeBool, KwTypeList, KwTypeDict, KwTypeTuple, KwTypeNull,

    // управление
    KwIf, KwElse, KwWhile, KwFor, KwIn, KwBreak, KwContinue,
    KwDo, KwThen,

    // функции/классы
    KwFunction, KwClass, KwExtends, KwOverride,
    KwImmutable, KwMutable, KwReturn, KwNew, KwSelf, KwBase,
    KwCreate, KwEmpty,

    // переменные
    KwLet, KwBe, KwBecome, KwConst, KwStrict,

    // ошибки
    KwTry, KwCatch, KwFinally, KwRaise,

    // ввод/вывод
    KwPrint, KwInput,

    // графика
    KwWindow, KwDrawPoint, KwDrawLine, KwDrawRect,
    KwColor, KwClear, KwSleep, KwClose
};

struct Token {
    TokKind kind;
    std::string text;
    int64_t i = 0;
    double  d = 0.0;
    int line = 1, col = 1;
};

class Lexer {
public:
    Lexer(std::string src, std::string file);
    std::vector<Token> tokenize();
    const std::string& file() const { return file_; }
    const std::vector<std::string>& lines() const { return lines_; }

private:
    std::string src_;
    std::string file_;
    size_t pos_ = 0;
    int line_ = 1, col_ = 1;
    std::vector<std::string> lines_;

    std::vector<int> indentStack_;
    bool lineStart_ = true;
    bool inBlockComment_ = false;

    uint32_t peek();
    uint32_t peek2();
    uint32_t advance();
    bool match(uint32_t cp);
    void skipInlineWhitespace();
    int  currentIndentFromLine();
    Token makeIdentOrKeyword(const std::string& s, int ln, int cl);
    Token lexNumber();
    Token lexString();
};

} // namespace veche