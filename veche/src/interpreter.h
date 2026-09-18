#pragma once
#include "ast.h"
#include "errors.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace veche {

struct Scope {
    std::unordered_map<std::string, ValuePtr> vars;
    std::unordered_map<std::string, bool> consts;
    std::shared_ptr<Scope> parent;
    ValuePtr self;
    std::shared_ptr<ClassInfo> klass;
    std::unordered_map<std::string, std::shared_ptr<FunctionDecl>> methods;

    ValuePtr* find(const std::string& n) {
        auto it = vars.find(n);
        if (it != vars.end()) return &it->second;
        if (parent) return parent->find(n);
        return nullptr;
    }
    bool isConst(const std::string& n) {
        auto it = consts.find(n);
        if (it != consts.end()) return it->second;
        if (parent) return parent->isConst(n);
        return false;
    }
};

using ScopePtr = std::shared_ptr<Scope>;

class Interpreter {
public:
    Interpreter();
    void run(const std::vector<StmtPtr>& prog);
    // Выполнить программу в уже существующем окружении.
    // Используется REPL-ом, чтобы переменные сохранялись между строками.
    void runIn(const std::vector<StmtPtr>& prog, ScopePtr env);

    ValuePtr evalExpr(const ExprPtr& e, ScopePtr env);
    void execStmt(const StmtPtr& s, ScopePtr env);

    void setSource(const std::string& file, const std::vector<std::string>& lines) {
        file_ = file; lines_ = lines;
    }
    void registerClass(std::shared_ptr<ClassInfo> c) { classes_[c->name] = c; }
    void setGlobals(const std::unordered_map<std::string, ValuePtr>& g) { globals_ = g; }

    // Создать новое пустое окружение (для REPL).
    ScopePtr makeGlobalScope() { return std::make_shared<Scope>(); }

private:
    std::unordered_map<std::string, std::shared_ptr<ClassInfo>> classes_;
    std::unordered_map<std::string, ValuePtr> globals_;
    std::string file_;
    std::vector<std::string> lines_;

    struct ReturnSignal   { ValuePtr value; };
    struct BreakSignal    {};
    struct ContinueSignal {};

    [[noreturn]] void raise(const std::string& kind, const std::string& msg);

    ValuePtr callFunction(std::shared_ptr<FunctionDecl> fn,
                          const std::vector<ValuePtr>& args,
                          ValuePtr self,
                          std::shared_ptr<ClassInfo> klass,
                          ScopePtr enclosing);

    ValuePtr binaryOp(const std::string& op, ValuePtr a, ValuePtr b);
    ValuePtr unaryOp(const std::string& op, ValuePtr a);
    void assignTo(const ExprPtr& target, ValuePtr val, ScopePtr env);

    std::string typeOf(ValuePtr v) { return v->typeName(); }
};

} // namespace veche