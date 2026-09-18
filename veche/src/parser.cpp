#include "parser.h"
#include "errors.h"
#include <unordered_set>

namespace veche {

Parser::Parser(std::vector<Token> toks, std::string file,
               const std::vector<std::string>* lines)
    : toks_(std::move(toks)), file_(std::move(file)), lines_(lines) {}

const Token& Parser::cur() { return toks_[pos_]; }
const Token& Parser::peek(int n) {
    size_t p = pos_ + n;
    if (p >= toks_.size()) p = toks_.size() - 1;
    return toks_[p];
}
bool Parser::check(TokKind k) { return cur().kind == k; }
bool Parser::match(TokKind k) { if (check(k)) { ++pos_; return true; } return false; }

std::string Parser::srcLine(int ln) const {
    if (!lines_ || ln <= 0 || (size_t)ln > lines_->size()) return "";
    return (*lines_)[ln - 1];
}

void Parser::errorAt(const Token& t, const std::string& kind, const std::string& msg) {
    throw VecheError(kind, msg, file_, t.line, t.col, srcLine(t.line));
}
void Parser::error(const std::string& kind, const std::string& msg) {
    errorAt(cur(), kind, msg);
}

const Token& Parser::expect(TokKind k, const std::string& what) {
    if (!check(k)) error("ОшибкаСинтаксиса", "Ожидалось " + what);
    return toks_[pos_++];
}

std::string Parser::typeFromToken(const Token& t) {
    switch (t.kind) {
        case TokKind::KwTypeInt:    return "целое";
        case TokKind::KwTypeDouble: return "дробное";
        case TokKind::KwTypeWord:   return "слово";
        case TokKind::KwTypeString: return "строка";
        case TokKind::KwTypeBool:   return "булево";
        case TokKind::KwTypeList:   return "список";
        case TokKind::KwTypeDict:   return "словарь";
        case TokKind::KwTypeTuple:  return "кортеж";
        case TokKind::KwTypeNull:   return "ничто";
        default: return "";
    }
}

// ---------------- Program ----------------
std::vector<StmtPtr> Parser::parseProgram() {
    std::vector<StmtPtr> prog;
    while (!check(TokKind::End)) prog.push_back(parseStatement());
    return prog;
}

// ---------------- Statements ----------------
StmtPtr Parser::parseStatement() {
    const Token& t = cur();
    switch (t.kind) {
        case TokKind::KwConst:   return parseVarDecl(true);
        case TokKind::KwStrict:  return parseVarDecl(false);
        case TokKind::KwTypeInt:
        case TokKind::KwTypeDouble:
        case TokKind::KwTypeWord:
        case TokKind::KwTypeString:
        case TokKind::KwTypeBool:
        case TokKind::KwTypeList:
        case TokKind::KwTypeDict:
        case TokKind::KwTypeTuple:
        case TokKind::KwTypeNull:
            return parseVarDecl(false);
        case TokKind::KwIf:      return parseIf();
        case TokKind::KwWhile:   return parseWhile();
        case TokKind::KwFor:     return parseFor();
        case TokKind::KwFunction:return parseFunction();
        case TokKind::KwClass:   return parseClass();
        case TokKind::KwReturn:  return parseReturn();
        case TokKind::KwTry:     return parseTry();
        case TokKind::KwRaise:   return parseRaise();
        case TokKind::KwBreak:
        case TokKind::KwContinue:return parseBreakContinue(t.kind);
        case TokKind::LBrace:    return parseBlock();
        default:                 return parseSimpleOrAssign();
    }
}

StmtPtr Parser::parseVarDecl(bool isConst) {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::VarDecl;
    s->line = cur().line; s->col = cur().col;
    s->isConst = isConst;

    if (isConst) expect(TokKind::KwConst, "'константа'");
    if (match(TokKind::KwStrict)) s->isStrict = true;

    // тип
    if (s->isStrict && !check(TokKind::KwTypeInt) && !check(TokKind::KwTypeDouble)
        && !check(TokKind::KwTypeWord) && !check(TokKind::KwTypeString)
        && !check(TokKind::KwTypeBool) && !check(TokKind::KwTypeList)
        && !check(TokKind::KwTypeDict) && !check(TokKind::KwTypeTuple)
        && !check(TokKind::KwTypeNull)) {
        error("ОшибкаСинтаксиса", "После 'строгое' ожидался тип");
    }
    s->typeName = typeFromToken(cur());
    ++pos_;

    // имя
    if (!check(TokKind::Ident)) {
        error("ОшибкаСинтаксиса",
            "Ожидалось имя переменной после типа '" + s->typeName + "'");
    }
    s->varName = cur().text; ++pos_;
    expect(TokKind::Assign, "'='");
    s->init = parseExpr();
    match(TokKind::Semicolon);
    return s;
}

StmtPtr Parser::parseIf() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::If; s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwIf, "'если'");
    expect(TokKind::LParen, "'('");
    s->cond = parseExpr();
    expect(TokKind::RParen, "')'");
    s->thenBranch = parseBlock();
    if (match(TokKind::KwElse)) {
        if (check(TokKind::KwIf)) s->elseBranch = parseIf();
        else                      s->elseBranch = parseBlock();
    }
    return s;
}

StmtPtr Parser::parseWhile() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::While; s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwWhile, "'пока'");
    expect(TokKind::LParen, "'('");
    s->cond = parseExpr();
    expect(TokKind::RParen, "')'");
    s->whileBody = parseBlock();
    return s;
}

StmtPtr Parser::parseFor() {
    auto s = std::make_shared<Stmt>();
    s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwFor, "'для'");
    expect(TokKind::LParen, "'('");

    // классический: for (целое и = 0; ...)
    bool classic = check(TokKind::KwTypeInt) || check(TokKind::KwTypeDouble)
                || check(TokKind::KwTypeWord) || check(TokKind::KwTypeString)
                || check(TokKind::KwTypeBool) || check(TokKind::KwTypeList)
                || check(TokKind::KwTypeDict) || check(TokKind::KwTypeTuple);
    if (classic) {
        s->kind = StmtKind::ForClassic;
        s->forInit = parseVarDecl(false);
        match(TokKind::Semicolon);
        s->cond = parseExpr();
        expect(TokKind::Semicolon, "';'");
        s->forStep = parseSimpleOrAssign();
        expect(TokKind::RParen, "')'");
        s->loopBody = parseBlock();
        return s;
    }

    // for (var in coll) — 'в' может быть KwIn или Ident "в"
    s->kind = StmtKind::ForIn;
    if (!check(TokKind::Ident)) {
        error("ОшибкаСинтаксиса", "Ожидалось имя переменной цикла");
    }
    s->loopVar = cur().text; ++pos_;

    bool isIn = check(TokKind::KwIn)
             || (check(TokKind::Ident) && cur().text == "в");
    if (!isIn) error("ОшибкаСинтаксиса", "Ожидалось 'в'");
    ++pos_; // съесть 'в'

    s->loopColl = parseExpr();
    expect(TokKind::RParen, "')'");
    s->loopBody = parseBlock();
    return s;
}

StmtPtr Parser::parseFunction() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::FunctionDecl; s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwFunction, "'функция'");
    if (!check(TokKind::Ident)) error("ОшибкаСинтаксиса", "Ожидалось имя функции");
    s->funcName = cur().text; ++pos_;
    expect(TokKind::LParen, "'('");
    if (!check(TokKind::RParen)) {
        do {
            Param p;
            p.type = typeFromToken(cur());
            if (p.type.empty()) error("ОшибкаСинтаксиса", "Ожидался тип параметра");
            ++pos_;
            if (!check(TokKind::Ident)) error("ОшибкаСинтаксиса", "Ожидалось имя параметра");
            p.name = cur().text; ++pos_;
            s->params.push_back(p);
        } while (match(TokKind::Comma));
    }
    expect(TokKind::RParen, "')'");
    if (match(TokKind::Arrow)) {
        s->returnType = typeFromToken(cur());
        if (s->returnType.empty()) error("ОшибкаСинтаксиса", "Ожидался тип возврата");
        ++pos_;
    }
    s->funcBody = parseBlock();
    return s;
}

StmtPtr Parser::parseClass() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::ClassDecl; s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwClass, "'класс'");
    if (!check(TokKind::Ident)) error("ОшибкаСинтаксиса", "Ожидалось имя класса");
    s->className = cur().text; ++pos_;
    if (match(TokKind::KwExtends)) {
        if (!check(TokKind::Ident)) error("ОшибкаСинтаксиса", "Ожидалось имя базового класса");
        s->baseName = cur().text; ++pos_;
    }
    expect(TokKind::LBrace, "'{'");
    while (!check(TokKind::RBrace) && !check(TokKind::End)) {
        if (check(TokKind::KwFunction)) {
            s->classMembers.push_back(parseFunction());
        } else if (match(TokKind::KwOverride)) {
            s->classMembers.push_back(parseFunction());
        } else {
            auto f = std::make_shared<Stmt>();
            f->kind = StmtKind::VarDecl;
            f->line = cur().line; f->col = cur().col;
            f->typeName = typeFromToken(cur());
            if (f->typeName.empty()) error("ОшибкаСинтаксиса", "Ожидался тип поля");
            ++pos_;
            if (!check(TokKind::Ident)) error("ОшибкаСинтаксиса", "Ожидалось имя поля");
            f->varName = cur().text; ++pos_;
            if (match(TokKind::Assign)) f->init = parseExpr();
            match(TokKind::Semicolon);
            s->classMembers.push_back(f);
        }
    }
    expect(TokKind::RBrace, "'}'");
    return s;
}

StmtPtr Parser::parseReturn() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::Return; s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwReturn, "'возврат'");
    if (!check(TokKind::RBrace) && !check(TokKind::Semicolon)
        && !check(TokKind::End) && !check(TokKind::KwFinally)
        && !check(TokKind::KwCatch)) {
        s->returnValue = parseExpr();
    }
    match(TokKind::Semicolon);
    return s;
}

StmtPtr Parser::parseTry() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::Try; s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwTry, "'попытка'");
    s->tryBody = parseBlock();
    if (match(TokKind::KwCatch)) {
        expect(TokKind::LParen, "'('");
        if (!check(TokKind::Ident)) error("ОшибкаСинтаксиса", "Ожидался тип ошибки");
        s->catchType = cur().text; ++pos_;
        if (!check(TokKind::Ident)) error("ОшибкаСинтаксиса", "Ожидалось имя переменной ошибки");
        s->catchVar = cur().text; ++pos_;
        expect(TokKind::RParen, "')'");
        s->catchBody = parseBlock();
    }
    if (match(TokKind::KwFinally)) {
        s->finallyBody = parseBlock();
    }
    return s;
}

StmtPtr Parser::parseRaise() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::Raise; s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwRaise, "'поднять'");
    if (check(TokKind::Ident)) { s->raiseKind = cur().text; ++pos_; }
    if (match(TokKind::LParen)) {
        s->raiseArg = parseExpr();
        expect(TokKind::RParen, "')'");
    }
    match(TokKind::Semicolon);
    return s;
}

StmtPtr Parser::parseBreakContinue(TokKind k) {
    auto s = std::make_shared<Stmt>();
    s->kind = (k == TokKind::KwBreak) ? StmtKind::Break : StmtKind::Continue;
    s->line = cur().line; s->col = cur().col;
    ++pos_;
    match(TokKind::Semicolon);
    return s;
}

StmtPtr Parser::parseBlock() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::Block; s->line = cur().line; s->col = cur().col;
    expect(TokKind::LBrace, "'{'");
    while (!check(TokKind::RBrace) && !check(TokKind::End)) {
        s->body.push_back(parseStatement());
    }
    expect(TokKind::RBrace, "'}'");
    return s;
}

StmtPtr Parser::parseSimpleOrAssign() {
    auto s = std::make_shared<Stmt>();
    s->line = cur().line; s->col = cur().col;
    ExprPtr e = parseExpr();

    if (check(TokKind::Assign)) {
        ++pos_;
        s->kind = StmtKind::Assign;
        s->assignTarget = e;
        s->assignValue = parseExpr();
        match(TokKind::Semicolon);
        return s;
    }
    s->kind = StmtKind::ExprStmt;
    s->expr = e;
    match(TokKind::Semicolon);
    return s;
}

StmtPtr Parser::parsePrintOrExpr() {
    return parseSimpleOrAssign();
}

// ---------------- Expressions ----------------
ExprPtr Parser::parseExpr() { return parseOr(); }

ExprPtr Parser::parseOr() {
    auto l = parseAnd();
    while (check(TokKind::Or) ||
           (check(TokKind::Ident) && cur().text == "или")) {
        auto e = std::make_shared<Expr>();
        e->kind = ExprKind::Binary; e->op = "или";
        e->line = cur().line; e->col = cur().col;
        ++pos_;
        e->left = l; e->right = parseAnd();
        l = e;
    }
    return l;
}

ExprPtr Parser::parseAnd() {
    auto l = parseEquality();
    while (check(TokKind::And) ||
           (check(TokKind::Ident) && cur().text == "и")) {
        auto e = std::make_shared<Expr>();
        e->kind = ExprKind::Binary; e->op = "и";
        e->line = cur().line; e->col = cur().col;
        ++pos_;
        e->left = l; e->right = parseEquality();
        l = e;
    }
    return l;
}

ExprPtr Parser::parseEquality() {
    auto l = parseComparison();
    while (check(TokKind::Eq) || check(TokKind::Ne)) {
        auto e = std::make_shared<Expr>();
        e->kind = ExprKind::Binary;
        e->op = check(TokKind::Eq) ? "==" : "!=";
        e->line = cur().line; e->col = cur().col;
        ++pos_;
        e->left = l; e->right = parseComparison();
        l = e;
    }
    return l;
}

ExprPtr Parser::parseComparison() {
    auto l = parseTerm();
    while (check(TokKind::Lt) || check(TokKind::Gt)
        || check(TokKind::Le) || check(TokKind::Ge)) {
        auto e = std::make_shared<Expr>();
        e->kind = ExprKind::Binary;
        switch (cur().kind) {
            case TokKind::Lt: e->op = "<"; break;
            case TokKind::Gt: e->op = ">"; break;
            case TokKind::Le: e->op = "<="; break;
            case TokKind::Ge: e->op = ">="; break;
            default: break;
        }
        e->line = cur().line; e->col = cur().col;
        ++pos_;
        e->left = l; e->right = parseTerm();
        l = e;
    }
    return l;
}

ExprPtr Parser::parseTerm() {
    auto l = parseFactor();
    while (check(TokKind::Plus) || check(TokKind::Minus)) {
        auto e = std::make_shared<Expr>();
        e->kind = ExprKind::Binary;
        e->op = check(TokKind::Plus) ? "+" : "-";
        e->line = cur().line; e->col = cur().col;
        ++pos_;
        e->left = l; e->right = parseFactor();
        l = e;
    }
    return l;
}

ExprPtr Parser::parseFactor() {
    auto l = parseUnary();
    while (check(TokKind::Star) || check(TokKind::Slash) || check(TokKind::Percent)) {
        auto e = std::make_shared<Expr>();
        e->kind = ExprKind::Binary;
        switch (cur().kind) {
            case TokKind::Star:    e->op = "*"; break;
            case TokKind::Slash:   e->op = "/"; break;
            case TokKind::Percent: e->op = "%"; break;
            default: break;
        }
        e->line = cur().line; e->col = cur().col;
        ++pos_;
        e->left = l; e->right = parseUnary();
        l = e;
    }
    return l;
}

ExprPtr Parser::parseUnary() {
    if (check(TokKind::Minus) || check(TokKind::Not)
        || (check(TokKind::Ident) && cur().text == "не")) {
        auto e = std::make_shared<Expr>();
        e->kind = ExprKind::Unary;
        e->op = check(TokKind::Minus) ? "-" : "не";
        e->line = cur().line; e->col = cur().col;
        ++pos_;
        e->right = parseUnary();
        return e;
    }
    return parsePostfix();
}

ExprPtr Parser::parsePostfix() {
    auto e = parsePrimary();
    for (;;) {
        if (check(TokKind::LParen)) {
            auto c = std::make_shared<Expr>();
            c->kind = ExprKind::Call;
            c->line = cur().line; c->col = cur().col;
            c->callee = e;
            ++pos_;
            if (!check(TokKind::RParen)) {
                do { c->args.push_back(parseExpr()); } while (match(TokKind::Comma));
            }
            expect(TokKind::RParen, "')'");
            e = c;
        } else if (check(TokKind::LBracket)) {
            auto ix = std::make_shared<Expr>();
            ix->kind = ExprKind::Index;
            ix->line = cur().line; ix->col = cur().col;
            ix->target = e;
            ++pos_;
            ix->index = parseExpr();
            expect(TokKind::RBracket, "']'");
            e = ix;
        } else if (check(TokKind::Dot)) {
            auto m = std::make_shared<Expr>();
            m->kind = ExprKind::Member;
            m->line = cur().line; m->col = cur().col;
            m->left = e;
            ++pos_;
            if (!check(TokKind::Ident)) error("ОшибкаСинтаксиса", "Ожидалось имя поля/метода");
            m->name = cur().text; ++pos_;
            e = m;
        } else break;
    }
    return e;
}

ExprPtr Parser::parseInterpString(const Token& t) {
    auto e = std::make_shared<Expr>();
    e->kind = ExprKind::Interp;
    e->line = t.line; e->col = t.col;

    const std::string& s = t.text;
    std::string lit;
    size_t i = 0;
    while (i < s.size()) {
        char c = s[i];
        if (c == '{') {
            if (!lit.empty()) {
                auto l = std::make_shared<Expr>();
                l->kind = ExprKind::Literal;
                l->literal = Value::makeString(lit);
                e->segments.push_back(l);
                lit.clear();
            }
            size_t depth = 1;
            size_t j = i + 1;
            std::string inner;
            while (j < s.size() && depth > 0) {
                if (s[j] == '{') ++depth;
                else if (s[j] == '}') { --depth; if (depth == 0) break; }
                inner.push_back(s[j]);
                ++j;
            }
            if (depth != 0) {
                errorAt(t, "ОшибкаСинтаксиса", "Незакрытая '{' в строке");
            }
            Lexer lx(inner, file_);
            auto toks = lx.tokenize();
            Parser sub(std::move(toks), file_, lines_);
            auto ex = sub.parseExpr();
            e->segments.push_back(ex);
            i = j + 1;
        } else if (c == '}') {
            errorAt(t, "ОшибкаСинтаксиса", "Неожиданная '}' в строке");
        } else {
            lit.push_back(c);
            ++i;
        }
    }
    if (!lit.empty()) {
        auto l = std::make_shared<Expr>();
        l->kind = ExprKind::Literal;
        l->literal = Value::makeString(lit);
        e->segments.push_back(l);
    }
    return e;
}

ExprPtr Parser::parsePrimary() {
    const Token& t = cur();
    switch (t.kind) {
        case TokKind::Int: {
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Literal; e->line = t.line; e->col = t.col;
            e->literal = Value::makeInt(t.i);
            ++pos_; return e;
        }
        case TokKind::Double: {
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Literal; e->line = t.line; e->col = t.col;
            e->literal = Value::makeDouble(t.d);
            ++pos_; return e;
        }
        case TokKind::True:
        case TokKind::False: {
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Literal; e->line = t.line; e->col = t.col;
            e->literal = Value::makeBool(t.kind == TokKind::True);
            ++pos_; return e;
        }
        case TokKind::KwTypeNull: {
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Literal; e->line = t.line; e->col = t.col;
            e->literal = Value::makeNull();
            ++pos_; return e;
        }
        case TokKind::String: {
            auto e = parseInterpString(t);
            ++pos_;
            return e;
        }
        case TokKind::KwSelf: {
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Self; e->line = t.line; e->col = t.col;
            ++pos_; return e;
        }
        case TokKind::KwBase: {
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Base; e->line = t.line; e->col = t.col;
            ++pos_; return e;
        }
        case TokKind::KwNew: {
            ++pos_;
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::New; e->line = t.line; e->col = t.col;
            e->callee = parsePostfix();
            return e;
        }
        case TokKind::KwInput: {
            ++pos_;
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Call; e->line = t.line; e->col = t.col;
            auto f = std::make_shared<Expr>();
            f->kind = ExprKind::Variable; f->name = "__ввод__";
            e->callee = f;
            expect(TokKind::LParen, "'('");
            if (!check(TokKind::RParen)) {
                do { e->args.push_back(parseExpr()); } while (match(TokKind::Comma));
            }
            expect(TokKind::RParen, "')'");
            return e;
        }
        case TokKind::KwPrint: {
            ++pos_;
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Call; e->line = t.line; e->col = t.col;
            auto f = std::make_shared<Expr>();
            f->kind = ExprKind::Variable; f->name = "__печать__";
            e->callee = f;
            expect(TokKind::LParen, "'('");
            if (!check(TokKind::RParen)) {
                do { e->args.push_back(parseExpr()); } while (match(TokKind::Comma));
            }
            expect(TokKind::RParen, "')'");
            return e;
        }
        case TokKind::LParen: {
            ++pos_;
            auto e = parseExpr();
            expect(TokKind::RParen, "')'");
            return e;
        }
        case TokKind::LBracket: {
            ++pos_;
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::ListLit; e->line = t.line; e->col = t.col;
            if (!check(TokKind::RBracket)) {
                do { e->elements.push_back(parseExpr()); } while (match(TokKind::Comma));
            }
            expect(TokKind::RBracket, "']'");
            return e;
        }
        case TokKind::LBrace: {
            ++pos_;
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::DictLit; e->line = t.line; e->col = t.col;
            if (!check(TokKind::RBrace)) {
                do {
                    auto k = parseExpr();
                    expect(TokKind::Colon, "':'");
                    auto v = parseExpr();
                    e->pairs.emplace_back(k, v);
                } while (match(TokKind::Comma));
            }
            expect(TokKind::RBrace, "'}'");
            return e;
        }
        case TokKind::Ident: {
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Variable; e->name = t.text;
            e->line = t.line; e->col = t.col;
            ++pos_; return e;
        }
        default:
            error("ОшибкаСинтаксиса", "Неожиданный токен");
    }
}

} // namespace veche