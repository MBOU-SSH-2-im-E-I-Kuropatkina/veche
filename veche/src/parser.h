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

    // statements
    StmtPtr parseStatement();
    StmtPtr parseVarDecl(bool isConst);
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();
    StmtPtr parseFunction();
    StmtPtr parseClass();
    StmtPtr parseReturn();
    StmtPtr parseTry();
    StmtPtr parseRaise();
    StmtPtr parseBlock();
    StmtPtr parseSimpleOrAssign();
    StmtPtr parsePrintOrExpr();
    StmtPtr parseBreakContinue(TokKind k);

    // expressions (precedence climbing)
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
    std::string typeFromToken(const Token& t);
};

} // namespace veche