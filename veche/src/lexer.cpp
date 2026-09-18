#include "lexer.h"
#include "utf8.h"
#include "errors.h"
#include <unordered_map>
#include <cctype>
#include <sstream>

namespace veche {

// ВНИМАНИЕ: слова "и", "или", "не", "в" НЕ резервируются здесь,
// чтобы их можно было использовать как обычные имена переменных/параметров.
// Их роль как операторов/связок распознаёт парсер по контексту (Ident + текст).
static const std::unordered_map<std::string, TokKind> kKeywords = {
    {"константа", TokKind::KwConst},
    {"строгое",   TokKind::KwStrict},
    {"целое",     TokKind::KwTypeInt},
    {"дробное",   TokKind::KwTypeDouble},
    {"слово",     TokKind::KwTypeWord},
    {"строка",    TokKind::KwTypeString},
    {"булево",    TokKind::KwTypeBool},
    {"список",    TokKind::KwTypeList},
    {"словарь",   TokKind::KwTypeDict},
    {"кортеж",    TokKind::KwTypeTuple},
    {"ничто",     TokKind::KwTypeNull},
    {"если",      TokKind::KwIf},
    {"иначе",     TokKind::KwElse},
    {"пока",      TokKind::KwWhile},
    {"для",       TokKind::KwFor},
    {"прервать",  TokKind::KwBreak},
    {"продолжить",TokKind::KwContinue},
    {"возврат",   TokKind::KwReturn},
    {"функция",   TokKind::KwFunction},
    {"класс",     TokKind::KwClass},
    {"наследует", TokKind::KwExtends},
    {"переопределить", TokKind::KwOverride},
    {"базовый",   TokKind::KwBase},
    {"новый",     TokKind::KwNew},
    {"себя",      TokKind::KwSelf},
    {"попытка",   TokKind::KwTry},
    {"перехват",  TokKind::KwCatch},
    {"наконец",   TokKind::KwFinally},
    {"поднять",   TokKind::KwRaise},
    {"печать",    TokKind::KwPrint},
    {"ввод",      TokKind::KwInput},
    {"истина",    TokKind::True},
    {"ложь",      TokKind::False},
};

Lexer::Lexer(std::string src, std::string file)
    : src_(std::move(src)), file_(std::move(file)) {
    std::istringstream is(src_);
    std::string l;
    while (std::getline(is, l)) {
        if (!l.empty() && l.back() == '\r') l.pop_back();
        lines_.push_back(l);
    }
}

uint32_t Lexer::peek() {
    size_t p = pos_;
    return utf8::decode(src_, p);
}

uint32_t Lexer::peek2() {
    size_t p = pos_;
    utf8::decode(src_, p); // skip first
    return utf8::decode(src_, p);
}

uint32_t Lexer::advance() {
    size_t p = pos_;
    uint32_t c = utf8::decode(src_, p);
    pos_ = p;
    if (c == '\n') { ++line_; col_ = 1; } else { ++col_; }
    return c;
}

bool Lexer::match(uint32_t cp) {
    if (peek() == cp) { advance(); return true; }
    return false;
}

void Lexer::skipWhitespaceAndComments() {
    for (;;) {
        uint32_t c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { advance(); continue; }
        if (c == '/' && peek2() == '/') {
            while (peek() != '\n' && peek() != 0) advance();
            continue;
        }
        break;
    }
}

static bool isIdentStart(uint32_t cp) {
    if (cp == '_') return true;
    if (cp >= 'a' && cp <= 'z') return true;
    if (cp >= 'A' && cp <= 'Z') return true;
    if (cp >= 0x0400 && cp <= 0x04FF) return true; // Cyrillic
    return false;
}
static bool isIdentPart(uint32_t cp) {
    if (isIdentStart(cp)) return true;
    if (cp >= '0' && cp <= '9') return true;
    return false;
}

Token Lexer::makeIdentOrKeyword(const std::string& s, int ln, int cl) {
    Token t;
    t.text = s; t.line = ln; t.col = cl;
    auto it = kKeywords.find(s);
    if (it != kKeywords.end()) t.kind = it->second;
    else                       t.kind = TokKind::Ident;
    return t;
}

Token Lexer::lexWordOrIdent() {
    int ln = line_, cl = col_;
    std::string s;
    while (isIdentPart(peek())) {
        uint32_t c = advance();
        utf8::encode(c, s);
    }
    return makeIdentOrKeyword(s, ln, cl);
}

Token Lexer::lexNumber() {
    int ln = line_, cl = col_;
    std::string s;
    bool isDouble = false;
    while (true) {
        uint32_t c = peek();
        if (c >= '0' && c <= '9') { s.push_back((char)advance()); }
        else if (c == '.' && !isDouble && peek2() >= '0' && peek2() <= '9') {
            isDouble = true; s.push_back((char)advance());
        } else break;
    }
    Token t; t.line = ln; t.col = cl; t.text = s;
    if (isDouble) { t.kind = TokKind::Double; t.d = std::stod(s); }
    else          { t.kind = TokKind::Int;    t.i = std::stoll(s); }
    return t;
}

Token Lexer::lexString() {
    int ln = line_, cl = col_;
    advance(); // opening "
    std::string s;
    while (true) {
        uint32_t c = peek();
        if (c == 0) {
            throw VecheError("ОшибкаСинтаксиса",
                "Незакрытая строка", file_, ln, cl);
        }
        if (c == '"') { advance(); break; }
        if (c == '\\') {
            advance();
            uint32_t e = advance();
            switch (e) {
                case 'n': s.push_back('\n'); break;
                case 't': s.push_back('\t'); break;
                case '\\': s.push_back('\\'); break;
                case '"': s.push_back('"'); break;
                default: utf8::encode(e, s); break;
            }
            continue;
        }
        uint32_t a = advance();
        utf8::encode(a, s);
    }
    Token t; t.kind = TokKind::String; t.text = s; t.line = ln; t.col = cl;
    return t;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> out;
    for (;;) {
        skipWhitespaceAndComments();
        uint32_t c = peek();
        int ln = line_, cl = col_;
        if (c == 0) {
            Token t; t.kind = TokKind::End; t.line = ln; t.col = cl;
            out.push_back(t); break;
        }

        if (isIdentStart(c)) { out.push_back(lexWordOrIdent()); continue; }
        if (c >= '0' && c <= '9') { out.push_back(lexNumber()); continue; }
        if (c == '"') { out.push_back(lexString()); continue; }

        auto push = [&](TokKind k, const std::string& s = "") {
            Token t; t.kind = k; t.text = s; t.line = ln; t.col = cl;
            out.push_back(t);
        };

        advance();
        switch (c) {
            case '(': push(TokKind::LParen); break;
            case ')': push(TokKind::RParen); break;
            case '{': push(TokKind::LBrace); break;
            case '}': push(TokKind::RBrace); break;
            case '[': push(TokKind::LBracket); break;
            case ']': push(TokKind::RBracket); break;
            case ',': push(TokKind::Comma); break;
            case '.': push(TokKind::Dot); break;
            case ':': push(TokKind::Colon); break;
            case ';': push(TokKind::Semicolon); break;
            case '+': push(TokKind::Plus); break;
            case '*': push(TokKind::Star); break;
            case '%': push(TokKind::Percent); break;
            case '/': push(TokKind::Slash); break;
            case '-':
                if (match('>')) push(TokKind::Arrow);
                else push(TokKind::Minus);
                break;
            case '=':
                if (match('=')) push(TokKind::Eq);
                else push(TokKind::Assign);
                break;
            case '!':
                if (match('=')) push(TokKind::Ne);
                else throw VecheError("ОшибкаСинтаксиса",
                    "Неожиданный символ '!'", file_, ln, cl);
                break;
            case '<':
                if (match('=')) push(TokKind::Le);
                else push(TokKind::Lt);
                break;
            case '>':
                if (match('=')) push(TokKind::Ge);
                else push(TokKind::Gt);
                break;
            default:
                throw VecheError("ОшибкаСинтаксиса",
                    "Неожиданный символ", file_, ln, cl);
        }
    }
    return out;
}

} // namespace veche