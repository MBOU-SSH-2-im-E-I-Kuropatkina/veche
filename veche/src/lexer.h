#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace veche {

enum class TokKind {
    End, Ident, Int, Double, String, Word, // Word not lexically distinct; used by parser
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Dot, Colon, Semicolon, Arrow,
    Plus, Minus, Star, Slash, Percent,
    Assign, Eq, Ne, Lt, Gt, Le, Ge,
    And, Or, Not,
    True, False, Null,
    // keywords
    KwConst, KwStrict, KwTypeInt, KwTypeDouble, KwTypeWord, KwTypeString,
    KwTypeBool, KwTypeList, KwTypeDict, KwTypeTuple, KwTypeNull,
    KwIf, KwElse, KwWhile, KwFor, KwIn, KwBreak, KwContinue,
    KwReturn, KwFunction, KwClass, KwExtends, KwOverride, KwBase,
    KwNew, KwSelf, KwTry, KwCatch, KwFinally, KwRaise, KwPrint,
    KwInput
};

struct Token {
    TokKind kind;
    std::string text;   // raw lexeme
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

    uint32_t peek();
    uint32_t peek2();
    uint32_t advance();
    bool match(uint32_t cp);
    void skipWhitespaceAndComments();
    Token makeIdentOrKeyword(const std::string& s, int ln, int cl);
    Token lexNumber();
    Token lexString();
    Token lexWordOrIdent();
};

} // namespace veche