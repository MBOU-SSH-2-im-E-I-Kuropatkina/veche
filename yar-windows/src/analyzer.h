#pragma once
#include "ast.h"
#include <unordered_map>
#include <vector>

namespace veche {

// Simple scope-based analyzer. Not strict; mostly validates names and
// detects obvious type issues. The interpreter does final checks.
class Analyzer {
public:
    void analyze(const std::vector<StmtPtr>& prog);

private:
    struct Scope {
        std::unordered_map<std::string, std::string> vars; // name -> type
    };
    std::vector<Scope> scopes_;
    std::unordered_map<std::string, std::shared_ptr<ClassInfo>> classes_;

    void push() { scopes_.emplace_back(); }
    void pop()  { scopes_.pop_back(); }
    void declare(const std::string& n, const std::string& t) { scopes_.back().vars[n] = t; }
    bool lookup(const std::string& n, std::string& t);

    void analyzeStmt(const StmtPtr& s);
    void analyzeExpr(const ExprPtr& e);
};

} // namespace veche