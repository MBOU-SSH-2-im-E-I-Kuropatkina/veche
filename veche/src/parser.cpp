#include "parser.h"
#include "errors.h"

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

bool Parser::match(TokKind k) {
    if (check(k)) { ++pos_; return true; }
    return false;
}

std::string Parser::srcLine(int ln) const {
    if (!lines_ || ln <= 0 || (size_t)ln > lines_->size()) return "";
    return (*lines_)[ln - 1];
}

void Parser::errorAt(const Token& t, const std::string& kind,
                     const std::string& msg) {
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
        case TokKind::KwTypeDouble: return "дробь";
        case TokKind::KwTypeString: return "строка";
        case TokKind::KwTypeChar:   return "символ";
        case TokKind::KwTypeWord:   return "слово";
        case TokKind::KwTypeBool:   return "булево";
        case TokKind::KwTypeList:   return "список";
        case TokKind::KwTypeDict:   return "словарь";
        case TokKind::KwTypeTuple:  return "кортеж";
        case TokKind::KwTypeNull:   return "ничто";
        case TokKind::Ident:        return t.text;   // имя класса — тоже тип
        default: return "";
    }
}

bool Parser::isTypeToken(const Token& t) {
    return !typeFromToken(t).empty();
}

// ============================================================
//                          ПРОГРАММА
// ============================================================
std::vector<StmtPtr> Parser::parseProgram() {
    std::vector<StmtPtr> prog;
    while (!check(TokKind::End)) {
        skipSeparators();
        if (check(TokKind::End)) break;
        prog.push_back(parseStatement());
    }
    return prog;
}

void Parser::skipSeparators() {
    while (check(TokKind::Newline) || check(TokKind::Semicolon)) ++pos_;
}

// Тело блока: Indent ... Dedent
StmtPtr Parser::parseBlockIndent() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::Block;
    s->line = cur().line; s->col = cur().col;
    expect(TokKind::Indent, "отступ (тело блока)");
    while (!check(TokKind::Dedent) && !check(TokKind::End)) {
        skipSeparators();
        if (check(TokKind::Dedent) || check(TokKind::End)) break;
        s->body.push_back(parseStatement());
    }
    expect(TokKind::Dedent, "конец блока");
    return s;
}

// ============================================================
//                          ОПЕРАТОРЫ
// ============================================================
StmtPtr Parser::parseStatement() {
    const Token& t = cur();
    switch (t.kind) {
        case TokKind::KwLet:      return parseLetStmt();
        case TokKind::KwIf:       return parseIf();
        case TokKind::KwWhile:    return parseWhile();
        case TokKind::KwFor:      return parseFor();
        case TokKind::KwReturn:   return parseReturn();
        case TokKind::KwFunction: return parseFunction();
        case TokKind::KwImmutable:
        case TokKind::KwMutable:  return parseFunction();
        case TokKind::KwClass:    return parseClass();
        case TokKind::KwTry:      return parseTry();
        case TokKind::KwRaise:    return parseRaise();
        case TokKind::KwBreak:    return parseBreakContinue(TokKind::KwBreak);
        case TokKind::KwContinue: return parseBreakContinue(TokKind::KwContinue);
        default:                  return parseSimpleOrAssign();
    }
}

// пусть [постоянное] [строгое] <тип> <имя> будет <expr>
// пусть <target> станет <expr>
// пусть <target> будет <expr>
StmtPtr Parser::parseLetStmt() {
    auto s = std::make_shared<Stmt>();
    s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwLet, "'пусть'");

    bool haveConst  = match(TokKind::KwConst);
    bool haveStrict = match(TokKind::KwStrict);

    // Признак объявления: <тип/класс> <Ident> будет
    bool looksLikeDecl =
        isTypeToken(cur())
        && peek(1).kind == TokKind::Ident
        && peek(2).kind == TokKind::KwBe;

    if (haveConst || haveStrict) looksLikeDecl = true;

    if (looksLikeDecl) {
        s->kind = StmtKind::VarDecl;
        s->isConst  = haveConst;
        s->isStrict = haveStrict;
        s->typeName = typeFromToken(cur());
        ++pos_;
        if (!check(TokKind::Ident)) {
            error("ОшибкаСинтаксиса",
                "Ожидалось имя переменной после типа '" + s->typeName + "'");
        }
        s->varName = cur().text; ++pos_;
        expect(TokKind::KwBe, "'будет'");
        s->init = parseExpr();
        match(TokKind::Semicolon);
        return s;
    }

    if (haveConst || haveStrict) {
        error("ОшибкаСинтаксиса",
            "'постоянное'/'строгое' допустимы только при объявлении");
    }

    // Присваивание
    s->kind = StmtKind::Assign;
    s->assignTarget = parsePostfix();
    if (match(TokKind::KwBecome)) {
        // станет
    } else if (match(TokKind::KwBe)) {
        // будет
    } else {
        error("ОшибкаСинтаксиса", "Ожидалось 'станет' или 'будет'");
    }
    s->assignValue = parseExpr();
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
    expect(TokKind::KwThen, "'то'");
    skipSeparators();
    s->thenBranch = parseBlockIndent();
    skipSeparators();
    if (match(TokKind::KwElse)) {
        if (match(TokKind::KwIf)) {
            // иначе если (...) то ...
            auto nested = std::make_shared<Stmt>();
            nested->kind = StmtKind::If;
            nested->line = cur().line; nested->col = cur().col;
            expect(TokKind::LParen, "'('");
            nested->cond = parseExpr();
            expect(TokKind::RParen, "')'");
            expect(TokKind::KwThen, "'то'");
            skipSeparators();
            nested->thenBranch = parseBlockIndent();
            skipSeparators();
            if (match(TokKind::KwElse)) {
                if (check(TokKind::KwIf)) {
                    --pos_;
                    nested->elseBranch = parseIf();
                } else {
                    skipSeparators();
                    nested->elseBranch = parseBlockIndent();
                }
            }
            s->elseBranch = nested;
        } else {
            skipSeparators();
            s->elseBranch = parseBlockIndent();
        }
    }
    return s;
}

StmtPtr Parser::parseWhile() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::While; s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwWhile, "'пока'");
    expect(TokKind::LBracket, "'['");
    s->cond = parseExpr();
    expect(TokKind::RBracket, "']'");
    expect(TokKind::KwDo, "'делать'");
    skipSeparators();
    s->whileBody = parseBlockIndent();
    return s;
}

StmtPtr Parser::parseFor() {
    auto s = std::make_shared<Stmt>();
    s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwFor, "'для'");
    expect(TokKind::LParen, "'('");

    // Классический: для (пусть <тип> и будет ...; ...; пусть ... станет ...)
    if (check(TokKind::KwLet)) {
        s->kind = StmtKind::ForClassic;
        s->forInit = parseLetStmt();
        match(TokKind::Semicolon);
        s->cond = parseExpr();
        expect(TokKind::Semicolon, "';'");
        if (!check(TokKind::KwLet)) {
            error("ОшибкаСинтаксиса", "Ожидалось 'пусть' в шаге цикла");
        }
        s->forStep = parseLetStmt();
        expect(TokKind::RParen, "')'");
        expect(TokKind::KwDo, "'делать'");
        skipSeparators();
        s->loopBody = parseBlockIndent();
        return s;
    }

    // для (<имя> в <колл>) делать
    s->kind = StmtKind::ForIn;
    if (!check(TokKind::Ident)) {
        error("ОшибкаСинтаксиса", "Ожидалось имя переменной цикла");
    }
    s->loopVar = cur().text; ++pos_;
    expect(TokKind::KwIn, "'в'");
    s->loopColl = parseExpr();
    expect(TokKind::RParen, "')'");
    expect(TokKind::KwDo, "'делать'");
    skipSeparators();
    s->loopBody = parseBlockIndent();
    return s;
}

StmtPtr Parser::parseReturn() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::Return; s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwReturn, "'возврат'");
    if (!check(TokKind::Newline) && !check(TokKind::Semicolon)
        && !check(TokKind::Dedent) && !check(TokKind::End)) {
        s->returnValue = parseExpr();
    }
    match(TokKind::Semicolon);
    return s;
}

StmtPtr Parser::parseTry() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::Try; s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwTry, "'попытка'");
    skipSeparators();
    s->tryBody = parseBlockIndent();
    skipSeparators();
    if (match(TokKind::KwCatch)) {
        expect(TokKind::LParen, "'('");
        if (!check(TokKind::Ident)) error("ОшибкаСинтаксиса", "Ожидался тип ошибки");
        s->catchType = cur().text; ++pos_;
        if (!check(TokKind::Ident)) error("ОшибкаСинтаксиса", "Ожидалось имя переменной");
        s->catchVar = cur().text; ++pos_;
        expect(TokKind::RParen, "')'");
        skipSeparators();
        s->catchBody = parseBlockIndent();
        skipSeparators();
    }
    if (match(TokKind::KwFinally)) {
        skipSeparators();
        s->finallyBody = parseBlockIndent();
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

// [неизменяемый|изменяемый] функция <имя>(<тип> <имя>; ...) [возврат <тип>] <блок>
StmtPtr Parser::parseFunction() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::FunctionDecl;
    s->line = cur().line; s->col = cur().col;

    if (match(TokKind::KwImmutable))      s->isImmutable = true;
    else if (match(TokKind::KwMutable))   s->isMutable   = true;

    expect(TokKind::KwFunction, "'функция'");

    if (check(TokKind::Ident)) {
        s->funcName = cur().text; ++pos_;
    } else {
        error("ОшибкаСинтаксиса", "Ожидалось имя функции");
    }
    s->isConstructor = (s->funcName == "создать");

    if (match(TokKind::LParen)) {
        if (!check(TokKind::RParen)) {
            do {
                Param p;
                if (!isTypeToken(cur())) {
                    error("ОшибкаСинтаксиса", "Ожидался тип параметра");
                }
                p.type = typeFromToken(cur()); ++pos_;
                if (!check(TokKind::Ident)) {
                    error("ОшибкаСинтаксиса", "Ожидалось имя параметра");
                }
                p.name = cur().text; ++pos_;
                s->params.push_back(p);
            } while (match(TokKind::Semicolon));
        }
        expect(TokKind::RParen, "')'");
    }

    if (match(TokKind::KwReturn)) {
        if (!isTypeToken(cur())) {
            error("ОшибкаСинтаксиса", "Ожидался тип возврата");
        }
        s->returnType = typeFromToken(cur());
        ++pos_;
    } else {
        s->returnType = "ничего";
    }

    skipSeparators();
    s->funcBody = parseBlockIndent();
    return s;
}

// класс <Имя> [наследует <Имя>] <блок с полями и методами>
StmtPtr Parser::parseClass() {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::ClassDecl; s->line = cur().line; s->col = cur().col;
    expect(TokKind::KwClass, "'класс'");
    if (!check(TokKind::Ident)) {
        error("ОшибкаСинтаксиса", "Ожидалось имя класса");
    }
    s->className = cur().text; ++pos_;
    if (match(TokKind::KwExtends)) {
        if (!check(TokKind::Ident)) {
            error("ОшибкаСинтаксиса", "Ожидалось имя базового класса");
        }
        s->baseName = cur().text; ++pos_;
    }
    skipSeparators();
    expect(TokKind::Indent, "отступ тела класса");
    while (!check(TokKind::Dedent) && !check(TokKind::End)) {
        skipSeparators();
        if (check(TokKind::Dedent) || check(TokKind::End)) break;

        if (check(TokKind::KwEmpty)) {
            // пустой <тип> <имя>
            ++pos_;
            auto f = std::make_shared<Stmt>();
            f->kind = StmtKind::VarDecl;
            f->line = cur().line; f->col = cur().col;
            if (!isTypeToken(cur())) {
                error("ОшибкаСинтаксиса", "Ожидался тип поля");
            }
            f->typeName = typeFromToken(cur()); ++pos_;
            if (!check(TokKind::Ident)) {
                error("ОшибкаСинтаксиса", "Ожидалось имя поля");
            }
            f->varName = cur().text; ++pos_;
            s->classMembers.push_back(f);
        } else if (check(TokKind::KwFunction) ||
                   check(TokKind::KwImmutable) ||
                   check(TokKind::KwMutable) ||
                   check(TokKind::KwOverride)) {
            match(TokKind::KwOverride);
            s->classMembers.push_back(parseFunction());
        } else if (check(TokKind::KwLet)) {
            s->classMembers.push_back(parseLetStmt());
        } else {
            error("ОшибкаСинтаксиса",
                "В классе ожидалось 'пустой', 'пусть' или 'функция'");
        }
    }
    expect(TokKind::Dedent, "конец тела класса");
    return s;
}

StmtPtr Parser::parseSimpleOrAssign() {
    auto s = std::make_shared<Stmt>();
    s->line = cur().line; s->col = cur().col;
    ExprPtr e = parseExpr();
    s->kind = StmtKind::ExprStmt;
    s->expr = e;
    match(TokKind::Semicolon);
    return s;
}

// ============================================================
//                          ВЫРАЖЕНИЯ
// ============================================================
ExprPtr Parser::parseExpr() { return parseOr(); }

ExprPtr Parser::parseOr() {
    auto l = parseAnd();
    while (check(TokKind::Or)) {
        auto e = std::make_shared<Expr>();
        e->kind = ExprKind::Binary; e->op = "или";
        e->line = cur().line; e->col = cur().col; ++pos_;
        e->left = l; e->right = parseAnd();
        l = e;
    }
    return l;
}

ExprPtr Parser::parseAnd() {
    auto l = parseEquality();
    while (check(TokKind::And)) {
        auto e = std::make_shared<Expr>();
        e->kind = ExprKind::Binary; e->op = "и";
        e->line = cur().line; e->col = cur().col; ++pos_;
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
        e->line = cur().line; e->col = cur().col; ++pos_;
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
        e->line = cur().line; e->col = cur().col; ++pos_;
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
        e->line = cur().line; e->col = cur().col; ++pos_;
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
        e->line = cur().line; e->col = cur().col; ++pos_;
        e->left = l; e->right = parseUnary();
        l = e;
    }
    return l;
}

ExprPtr Parser::parseUnary() {
    if (check(TokKind::Minus) || check(TokKind::Not)) {
        auto e = std::make_shared<Expr>();
        e->kind = ExprKind::Unary;
        e->op = check(TokKind::Minus) ? "-" : "не";
        e->line = cur().line; e->col = cur().col; ++pos_;
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
            c->callee = e; ++pos_;
            if (!check(TokKind::RParen)) {
                do { c->args.push_back(parseExpr()); } while (match(TokKind::Semicolon));
            }
            expect(TokKind::RParen, "')'");
            e = c;
        } else if (check(TokKind::LBracket)) {
            auto ix = std::make_shared<Expr>();
            ix->kind = ExprKind::Index;
            ix->line = cur().line; ix->col = cur().col;
            ix->target = e; ++pos_;
            ix->index = parseExpr();
            expect(TokKind::RBracket, "']'");
            e = ix;
        } else if (check(TokKind::Dot)) {
            auto m = std::make_shared<Expr>();
            m->kind = ExprKind::Member;
            m->line = cur().line; m->col = cur().col;
            m->left = e; ++pos_;
            if (!check(TokKind::Ident)) {
                error("ОшибкаСинтаксиса", "Ожидалось имя поля");
            }
            m->name = cur().text; ++pos_;
            e = m;
        } else break;
    }
    return e;
}

// Строка с интерполяцией {выражение}
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
            size_t depth = 1, j = i + 1;
            std::string inner;
            while (j < s.size() && depth > 0) {
                if (s[j] == '{') ++depth;
                else if (s[j] == '}') { --depth; if (depth == 0) break; }
                inner.push_back(s[j]); ++j;
            }
            if (depth != 0) errorAt(t, "ОшибкаСинтаксиса", "Незакрытая '{'");
            Lexer lx(inner, file_);
            auto toks = lx.tokenize();
            Parser sub(std::move(toks), file_, lines_);
            e->segments.push_back(sub.parseExpr());
            i = j + 1;
        } else if (c == '}') {
            errorAt(t, "ОшибкаСинтаксиса", "Неожиданная '}'");
        } else {
            lit.push_back(c); ++i;
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
            e->literal = Value::makeInt(t.i); ++pos_; return e;
        }
        case TokKind::Double: {
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Literal; e->line = t.line; e->col = t.col;
            e->literal = Value::makeDouble(t.d); ++pos_; return e;
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
            e->literal = Value::makeNull(); ++pos_; return e;
        }
        case TokKind::String: {
            auto e = parseInterpString(t); ++pos_; return e;
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
            if (!check(TokKind::Ident)) {
                error("ОшибкаСинтаксиса", "Ожидалось имя класса после 'новый'");
            }
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
                do { e->args.push_back(parseExpr()); } while (match(TokKind::Semicolon));
            }
            expect(TokKind::RParen, "')'");
            return e;
        }
        case TokKind::KwPrint: {
            ++pos_;
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Call; e->line = t.line; e->col = t.col;
            auto f = std::make_shared<Expr>();
            f->kind = ExprKind::Variable; f->name = "__вывод__";
            e->callee = f;
            expect(TokKind::LParen, "'('");
            if (!check(TokKind::RParen)) {
                do { e->args.push_back(parseExpr()); } while (match(TokKind::Semicolon));
            }
            expect(TokKind::RParen, "')'");
            return e;
        }

        // ---- Приведения: целое(x), дробь(x), строка(x), символ(x) ----
        // Срабатывают только если сразу после типа идёт '(' —
        // иначе это объявление переменной вроде 'целое х будет …'.
        case TokKind::KwTypeInt:
        case TokKind::KwTypeDouble:
        case TokKind::KwTypeString:
        case TokKind::KwTypeChar: {
            if (peek(1).kind != TokKind::LParen) {
                error("ОшибкаСинтаксиса",
                      "Тип нельзя использовать как значение");
            }
            std::string fname;
            switch (t.kind) {
                case TokKind::KwTypeInt:    fname = "__целое__"; break;
                case TokKind::KwTypeDouble: fname = "__дробь__"; break;
                case TokKind::KwTypeString: fname = "__строка__"; break;
                case TokKind::KwTypeChar:   fname = "__символ__"; break;
                default: break;
            }
            ++pos_;   // съесть сам токен типа
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Call; e->line = t.line; e->col = t.col;
            auto f = std::make_shared<Expr>();
            f->kind = ExprKind::Variable; f->name = fname;
            e->callee = f;
            expect(TokKind::LParen, "'('");
            if (!check(TokKind::RParen)) {
                do { e->args.push_back(parseExpr()); } while (match(TokKind::Semicolon));
            }
            expect(TokKind::RParen, "')'");
            return e;
        }

        // ---- Графика ----
        case TokKind::KwWindow:
        case TokKind::KwDrawPoint:
        case TokKind::KwDrawLine:
        case TokKind::KwDrawRect:
        case TokKind::KwColor:
        case TokKind::KwClear:
        case TokKind::KwSleep:
        case TokKind::KwClose: {
            std::string fname;
            switch (t.kind) {
                case TokKind::KwWindow:    fname = "__окно__"; break;
                case TokKind::KwDrawPoint: fname = "__точка__"; break;
                case TokKind::KwDrawLine:  fname = "__линия__"; break;
                case TokKind::KwDrawRect:  fname = "__прямоугольник__"; break;
                case TokKind::KwColor:     fname = "__цвет__"; break;
                case TokKind::KwClear:     fname = "__очистить__"; break;
                case TokKind::KwSleep:     fname = "__пауза__"; break;
                case TokKind::KwClose:     fname = "__закрыть_окно__"; break;
                default: break;
            }
            ++pos_;
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Call; e->line = t.line; e->col = t.col;
            auto f = std::make_shared<Expr>();
            f->kind = ExprKind::Variable; f->name = fname;
            e->callee = f;
            expect(TokKind::LParen, "'('");
            if (!check(TokKind::RParen)) {
                do { e->args.push_back(parseExpr()); } while (match(TokKind::Semicolon));
            }
            expect(TokKind::RParen, "')'");
            return e;
        }

        // ---- Скобки: либо выражение, либо кортеж (если внутри ';') ----
        case TokKind::LParen: {
            ++pos_;
            auto first = parseExpr();
            if (check(TokKind::Semicolon)) {
                auto e = std::make_shared<Expr>();
                e->kind = ExprKind::TupleLit;
                e->line = t.line; e->col = t.col;
                e->elements.push_back(first);
                while (match(TokKind::Semicolon)) {
                    if (check(TokKind::RParen)) break;
                    e->elements.push_back(parseExpr());
                }
                expect(TokKind::RParen, "')'");
                return e;
            }
            expect(TokKind::RParen, "')'");
            return first;
        }

        case TokKind::LBracket: {
            ++pos_;
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::ListLit; e->line = t.line; e->col = t.col;
            if (!check(TokKind::RBracket)) {
                do { e->elements.push_back(parseExpr()); } while (match(TokKind::Semicolon));
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
                } while (match(TokKind::Semicolon));
            }
            expect(TokKind::RBrace, "'}'");
            return e;
        }

        case TokKind::Ident: {
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Variable; e->name = t.text;
            e->line = t.line; e->col = t.col; ++pos_; return e;
        }

        default:
            error("ОшибкаСинтаксиса", "Неожиданный токен");
    }
}

} // namespace veche