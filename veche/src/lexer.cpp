#include "lexer.h"
#include "utf8.h"
#include "errors.h"
#include <unordered_map>
#include <sstream>

namespace veche {

static const std::unordered_map<std::string, TokKind> kKeywords = {
    // типы
    {"целое",   TokKind::KwTypeInt},
    {"дробь",   TokKind::KwTypeDouble},
    {"строка",  TokKind::KwTypeString},
    {"символ",  TokKind::KwTypeChar},
    {"слово",   TokKind::KwTypeWord},
    {"булево",  TokKind::KwTypeBool},
    {"список",  TokKind::KwTypeList},
    {"словарь", TokKind::KwTypeDict},
    {"кортеж",  TokKind::KwTypeTuple},
    {"ничто",   TokKind::KwTypeNull},
    {"ничего",  TokKind::KwTypeNull},

    // управление
    {"если",     TokKind::KwIf},
    {"иначе",    TokKind::KwElse},
    {"то",       TokKind::KwThen},
    {"пока",     TokKind::KwWhile},
    {"для",      TokKind::KwFor},
    {"в",        TokKind::KwIn},
    {"делать",   TokKind::KwDo},
    {"прервать", TokKind::KwBreak},
    {"продолжить",TokKind::KwContinue},

    // функции/классы
    {"функция",       TokKind::KwFunction},
    {"класс",         TokKind::KwClass},
    {"наследует",     TokKind::KwExtends},
    {"переопределить",TokKind::KwOverride},
    {"неизменяемый",  TokKind::KwImmutable},
    {"изменяемый",    TokKind::KwMutable},
    {"возврат",       TokKind::KwReturn},
    {"новый",         TokKind::KwNew},
    {"сам",           TokKind::KwSelf},
    {"базовый",       TokKind::KwBase},
    {"пустой",        TokKind::KwEmpty},

    // переменные
    {"пусть",       TokKind::KwLet},
    {"будет",       TokKind::KwBe},
    {"станет",      TokKind::KwBecome},
    {"постоянное",  TokKind::KwConst},
    {"строгое",     TokKind::KwStrict},

    // ошибки
    {"попытка",  TokKind::KwTry},
    {"перехват", TokKind::KwCatch},
    {"наконец",  TokKind::KwFinally},
    {"поднять",  TokKind::KwRaise},

    // ввод/вывод
    {"вывод", TokKind::KwPrint},
    {"ввод",  TokKind::KwInput},

    // графика
    {"окно",                    TokKind::KwWindow},
    {"рисовать_точку",          TokKind::KwDrawPoint},
    {"рисовать_линию",          TokKind::KwDrawLine},
    {"рисовать_прямоугольник",  TokKind::KwDrawRect},
    {"цвет",                    TokKind::KwColor},
    {"очистить",                TokKind::KwClear},
    {"пауза",                   TokKind::KwSleep},
    {"закрыть_окно",            TokKind::KwClose},

    // литералы
    {"истина", TokKind::True},
    {"ложь",   TokKind::False},

    // логика
    {"и",   TokKind::And},
    {"или", TokKind::Or},
    {"не",  TokKind::Not},

    // ВНИМАНИЕ: "создать" НЕ здесь — приходит как Ident,
    // чтобы можно было назвать функцию создать(...).
};

Lexer::Lexer(std::string src, std::string file)
    : src_(std::move(src)), file_(std::move(file)) {
    std::istringstream is(src_);
    std::string l;
    while (std::getline(is, l)) {
        if (!l.empty() && l.back() == '\r') l.pop_back();
        lines_.push_back(l);
    }
    indentStack_.push_back(0);
}

uint32_t Lexer::peek() {
    size_t p = pos_;
    return utf8::decode(src_, p);
}
uint32_t Lexer::peek2() {
    size_t p = pos_;
    utf8::decode(src_, p);
    return utf8::decode(src_, p);
}
uint32_t Lexer::advance() {
    size_t p = pos_;
    uint32_t c = utf8::decode(src_, p);
    pos_ = p;
    if (c == '\n') { ++line_; col_ = 1; lineStart_ = true; }
    else           { ++col_; }
    return c;
}
bool Lexer::match(uint32_t cp) {
    if (peek() == cp) { advance(); return true; }
    return false;
}
int Lexer::currentIndentFromLine() {
    if (line_ <= 0 || (size_t)line_ > lines_.size()) return 0;
    const std::string& l = lines_[line_ - 1];
    int n = 0;
    for (char c : l) {
        if (c == ' ') ++n;
        else if (c == '\t') n += 4;
        else break;
    }
    return n;
}
void Lexer::skipInlineWhitespace() {
    for (;;) {
        uint32_t c = peek();
        if (c == ' ' || c == '\t' || c == '\r') { advance(); continue; }
        break;
    }
}

Token Lexer::makeIdentOrKeyword(const std::string& s, int ln, int cl) {
    Token t; t.text = s; t.line = ln; t.col = cl;
    auto it = kKeywords.find(s);
    if (it != kKeywords.end()) t.kind = it->second;
    else                       t.kind = TokKind::Ident;
    return t;
}

Token Lexer::lexNumber() {
    int ln = line_, cl = col_;
    std::string s;
    bool isDouble = false;
    while (true) {
        uint32_t c = peek();
        if (c >= '0' && c <= '9') s.push_back((char)advance());
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
    advance();
    std::string s;
    while (true) {
        uint32_t c = peek();
        if (c == 0) throw VecheError("ОшибкаСинтаксиса",
            "Незакрытая строка", file_, ln, cl);
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
        if (lineStart_) {
            lineStart_ = false;

            if (line_ <= (int)lines_.size()) {
                const std::string& raw = lines_[line_ - 1];
                size_t i = 0;
                while (i < raw.size() && (raw[i] == ' ' || raw[i] == '\t')) ++i;

                if (i < raw.size() && raw[i] == '@') {
                    inBlockComment_ = !inBlockComment_;
                    while (peek() != '\n' && peek() != 0) advance();
                    if (peek() == '\n') advance();
                    lineStart_ = true;
                    continue;
                }
                if (inBlockComment_) {
                    while (peek() != '\n' && peek() != 0) advance();
                    if (peek() == '\n') advance();
                    lineStart_ = true;
                    continue;
                }
                if (i >= raw.size()) {
                    while (peek() != '\n' && peek() != 0) advance();
                    if (peek() == '\n') advance();
                    lineStart_ = true;
                    continue;
                }
            }

            int indent = currentIndentFromLine();
            int top = indentStack_.back();

            if (indent > top) {
                indentStack_.push_back(indent);
                Token t; t.kind = TokKind::Indent; t.line = line_; t.col = 1;
                out.push_back(t);
            } else {
                while (indent < indentStack_.back()) {
                    indentStack_.pop_back();
                    Token t; t.kind = TokKind::Dedent; t.line = line_; t.col = 1;
                    out.push_back(t);
                }
                if (indent != indentStack_.back()) {
                    throw VecheError("ОшибкаСинтаксиса",
                        "Несогласованный отступ", file_, line_, 1);
                }
            }

            skipInlineWhitespace();
        }

        skipInlineWhitespace();

        uint32_t c = peek();
        int ln = line_, cl = col_;

        if (c == 0) {
            while (indentStack_.size() > 1) {
                indentStack_.pop_back();
                Token t; t.kind = TokKind::Dedent; t.line = ln; t.col = cl;
                out.push_back(t);
            }
            Token t; t.kind = TokKind::End; t.line = ln; t.col = cl;
            out.push_back(t);
            break;
        }
        if (c == '\n') {
            advance();
            Token t; t.kind = TokKind::Newline; t.line = ln; t.col = cl;
            out.push_back(t);
            lineStart_ = true;
            continue;
        }
        if (c == '/' && peek2() == '/') {
            while (peek() != '\n' && peek() != 0) advance();
            continue;
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            c == '_' || (c >= 0x0400 && c <= 0x04FF)) {
            std::string s;
            while (true) {
                uint32_t x = peek();
                if ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') ||
                    (x >= '0' && x <= '9') || x == '_' ||
                    (x >= 0x0400 && x <= 0x04FF)) {
                    utf8::encode(advance(), s);
                } else break;
            }
            out.push_back(makeIdentOrKeyword(s, ln, cl));
            continue;
        }
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
