#include "analyzer.h"
#include "errors.h"

namespace veche {

bool Analyzer::lookup(const std::string& n, std::string& t) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto f = it->vars.find(n);
        if (f != it->vars.end()) { t = f->second; return true; }
    }
    return false;
}

void Analyzer::analyze(const std::vector<StmtPtr>& prog) {
    push();
    for (auto& s : prog) analyzeStmt(s);
    pop();
}

void Analyzer::analyzeStmt(const StmtPtr& s) {
    if (!s) return;
    switch (s->kind) {
        case StmtKind::VarDecl: {
            if (s->init) analyzeExpr(s->init);
            declare(s->varName, s->typeName);
            break;
        }
        case StmtKind::Assign: {
            analyzeExpr(s->assignTarget);
            analyzeExpr(s->assignValue);
            break;
        }
        case StmtKind::ExprStmt: analyzeExpr(s->expr); break;
        case StmtKind::Block: {
            push();
            for (auto& st : s->body) analyzeStmt(st);
            pop();
            break;
        }
        case StmtKind::If: {
            analyzeExpr(s->cond);
            analyzeStmt(s->thenBranch);
            analyzeStmt(s->elseBranch);
            break;
        }
        case StmtKind::While: {
            analyzeExpr(s->cond);
            analyzeStmt(s->whileBody);
            break;
        }
        case StmtKind::ForIn: {
            analyzeExpr(s->loopColl);
            push();
            declare(s->loopVar, "ничто");
            analyzeStmt(s->loopBody);
            pop();
            break;
        }
        case StmtKind::ForClassic: {
            push();
            analyzeStmt(s->forInit);
            if (s->cond) analyzeExpr(s->cond);
            analyzeStmt(s->forStep);
            analyzeStmt(s->loopBody);
            pop();
            break;
        }
        case StmtKind::Return: if (s->returnValue) analyzeExpr(s->returnValue); break;
        case StmtKind::FunctionDecl: {
            push();
            for (auto& p : s->params) declare(p.name, p.type);
            analyzeStmt(s->funcBody);
            pop();
            break;
        }
        case StmtKind::ClassDecl: {
            auto ci = std::make_shared<ClassInfo>();
            ci->name = s->className;
            ci->baseName = s->baseName;
            ci->members = s->classMembers;
            for (auto& m : s->classMembers) {
                if (m->kind == StmtKind::FunctionDecl) {
                    auto fd = std::make_shared<FunctionDecl>();
                    fd->name = m->funcName;
                    fd->params = m->params;
                    fd->returnType = m->returnType;
                    fd->body = m->funcBody;
                    fd->owner = ci;
                    ci->methods[m->funcName] = fd;
                }
            }
            classes_[s->className] = ci;
            declare(s->className, "класс");
            break;
        }
        case StmtKind::Try: {
            analyzeStmt(s->tryBody);
            push();
            declare(s->catchVar, s->catchType);
            analyzeStmt(s->catchBody);
            pop();
            analyzeStmt(s->finallyBody);
            break;
        }
        case StmtKind::Raise: if (s->raiseArg) analyzeExpr(s->raiseArg); break;
        default: break;
    }
}

void Analyzer::analyzeExpr(const ExprPtr& e) {
    if (!e) return;
    switch (e->kind) {
        case ExprKind::Binary:
            analyzeExpr(e->left); analyzeExpr(e->right); break;
        case ExprKind::Unary:
            analyzeExpr(e->right); break;
        case ExprKind::Call:
            analyzeExpr(e->callee);
            for (auto& a : e->args) analyzeExpr(a);
            break;
        case ExprKind::Index:
            analyzeExpr(e->target); analyzeExpr(e->index); break;
        case ExprKind::Member:
            analyzeExpr(e->left); break;
        case ExprKind::ListLit:
        case ExprKind::TupleLit:
            for (auto& el : e->elements) analyzeExpr(el);
            break;
        case ExprKind::DictLit:
            for (auto& kv : e->pairs) { analyzeExpr(kv.first); analyzeExpr(kv.second); }
            break;
        case ExprKind::Interp:
            for (auto& seg : e->segments) analyzeExpr(seg);
            break;
        case ExprKind::New:
            analyzeExpr(e->callee); break;
        default: break;
    }
}

} // namespace veche