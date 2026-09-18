#pragma once
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "value.h"   // тянет Param, FunctionDecl, ClassInfo, StmtPtr

namespace veche {

struct Expr;
struct Stmt;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

// ---------------- Expressions ----------------
enum class ExprKind {
    Literal, Variable, Binary, Unary, Call, Index,
    Member, ListLit, DictLit, TupleLit, Interp, New, Self, Base
};

struct Expr {
    ExprKind kind;
    int line = 0, col = 0;

    ValuePtr literal;                        // Literal
    std::string name;                        // Variable / Member / Self / Base
    std::string op;                          // Binary / Unary
    ExprPtr left, right;                     // Binary / Unary / Member
    ExprPtr callee;                          // Call / New
    std::vector<ExprPtr> args;               // Call
    ExprPtr target, index;                   // Index
    std::vector<ExprPtr> elements;           // ListLit / TupleLit
    std::vector<std::pair<ExprPtr, ExprPtr>> pairs; // DictLit
    std::vector<ExprPtr> segments;           // Interp
};

// ---------------- Statements ----------------
enum class StmtKind {
    VarDecl, Assign, ExprStmt, Block, If, While, ForIn, ForClassic,
    Break, Continue, Return, FunctionDecl, ClassDecl, Try,
    Raise, Print, ExprAssign
};

struct Stmt {
    StmtKind kind;
    int line = 0, col = 0;

    // VarDecl
    std::string typeName;
    std::string varName;
    bool isConst = false;
    bool isStrict = false;
    ExprPtr init;

    // Assign
    ExprPtr assignTarget;
    ExprPtr assignValue;

    // ExprStmt
    ExprPtr expr;

    // Block
    std::vector<StmtPtr> body;

    // If / While
    ExprPtr cond;
    StmtPtr thenBranch, elseBranch;
    StmtPtr whileBody;

    // ForIn
    std::string loopVar;
    ExprPtr loopColl;
    StmtPtr loopBody;

    // ForClassic
    StmtPtr forInit, forStep;

    // Return
    ExprPtr returnValue;

    // FunctionDecl
    std::string funcName;
    std::vector<Param> params;   // Param приходит из value.h
    std::string returnType;
    StmtPtr funcBody;

    // ClassDecl
    std::string className;
    std::string baseName;
    std::vector<StmtPtr> classMembers;

    // Try
    StmtPtr tryBody;
    std::string catchType, catchVar;
    StmtPtr catchBody, finallyBody;

    // Raise
    std::string raiseKind;
    ExprPtr raiseArg;

    // Print
    std::vector<ExprPtr> printArgs;
};

} // namespace veche