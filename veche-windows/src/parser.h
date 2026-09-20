#pragma once
#include "lexer.h"
#include "ast.h"

namespace veche {

class Parser {
public:
    Parser(std::vector<Token> toks, std::string file,
           const std::vector<std::string>* lines);
    std::vector<StmtPtr> parseProgram();

private:
    std::vector<Token> toks_;
    std::string file_;
    const std::vector<std::string>* lines_;
    size_t pos_ = 0;

    const Token& cur();
    const Token& peek(int n = 1);
    bool check(TokKind k);
    bool match(TokKind k);
    const Token& expect(TokKind k, const std::string& what);

    [[noreturn]] void error(const std::string& kind, const std::string& msg);
    [[noreturn]] void errorAt(const Token& t, const std::string& kind,
                              const std::string& msg);
    std::string srcLine(int ln) const;
    std::string typeFromToken(const Token& t);
    bool isTypeToken(const Token& t);

    void skipSeparators();   // Newline / Semicolon
    StmtPtr parseBlockIndent(); // ожидает Indent ... Dedent

    StmtPtr parseStatement();
    StmtPtr parseVarDecl();
    StmtPtr parseLetStmt();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();
    StmtPtr parseReturn();
    StmtPtr parseTry();
    StmtPtr parseRaise();
    StmtPtr parseBreakContinue(TokKind k);
    StmtPtr parseFunction();
    StmtPtr parseClass();
    StmtPtr parseSimpleOrAssign();

    ExprPtr parseExpr();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseTerm();
    ExprPtr parseFactor();
    ExprPtr parseUnary();
    ExprPtr parsePostfix();
    ExprPtr parsePrimary();
    ExprPtr parseInterpString(const Token& t);
};

} // namespace veche