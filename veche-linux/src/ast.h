#pragma once
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "value.h"

namespace veche {

struct Expr;
struct Stmt;
using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

enum class ExprKind {
    Literal, Variable, Binary, Unary, Call, Index,
    Member, ListLit, DictLit, TupleLit, Interp, New, Self, Base
};

struct Expr {
    ExprKind kind;
    int line = 0, col = 0;

    ValuePtr literal;
    std::string name;
    std::string op;
    ExprPtr left, right;
    ExprPtr callee;
    std::vector<ExprPtr> args;
    ExprPtr target, index;
    std::vector<ExprPtr> elements;
    std::vector<std::pair<ExprPtr, ExprPtr>> pairs;
    std::vector<ExprPtr> segments;
};

enum class StmtKind {
    VarDecl, Assign, AssignNew, ExprStmt, Block, If, While, ForIn, ForClassic,
    Break, Continue, Return, FunctionDecl, ClassDecl, Try,
    Raise, Print
};

struct Stmt {
    StmtKind kind;
    int line = 0, col = 0;

    std::string typeName;
    std::string varName;
    bool isConst = false;
    bool isStrict = false;
    ExprPtr init;

    ExprPtr assignTarget;
    ExprPtr assignValue;

    ExprPtr expr;

    std::vector<StmtPtr> body;

    ExprPtr cond;
    StmtPtr thenBranch, elseBranch;

    StmtPtr whileBody;

    std::string loopVar;
    ExprPtr loopColl;
    StmtPtr loopBody;
    StmtPtr forInit, forStep;

    ExprPtr returnValue;

    std::string funcName;
    std::vector<Param> params;
    std::string returnType;
    bool isImmutable = false;
    bool isMutable   = false;
    bool isConstructor = false;
    StmtPtr funcBody;

    std::string className;
    std::string baseName;
    std::vector<StmtPtr> classMembers;

    StmtPtr tryBody;
    std::string catchType, catchVar;
    StmtPtr catchBody, finallyBody;

    std::string raiseKind;
    ExprPtr raiseArg;

    std::vector<ExprPtr> printArgs;
};

} // namespace veche