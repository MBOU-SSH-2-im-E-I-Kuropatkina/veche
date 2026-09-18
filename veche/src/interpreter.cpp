#include "interpreter.h"
#include "builtins.h"
#include "utf8.h"
#include <iostream>
#include <cmath>
#include <sstream>
#include <string>

namespace veche {

Interpreter::Interpreter() {}

void Interpreter::raise(const std::string& kind, const std::string& msg) {
    throw VecheError(kind, msg, file_);
}

static bool userSaidYes(const std::string& ans) {
    if (ans.empty()) return false;
    size_t pos = 0;
    uint32_t first = utf8::decode(ans, pos);
    return first == 0x0434 || first == 0x0414
        || first == 0x0079 || first == 0x0059;
}

static char firstBadCharInWord(const std::string& s) {
    for (char c : s) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r'
            || c == '-' || c == '_') return c;
    }
    return 0;
}

static bool isAssignableStrict(const std::string& typeName, Type vt) {
    if (typeName == "целое")   return vt == Type::Целое;
    if (typeName == "дробь")   return vt == Type::Целое || vt == Type::Дробное;
    if (typeName == "символ")  return vt == Type::Слово;
    if (typeName == "слово")   return vt == Type::Слово || vt == Type::Строка;
    if (typeName == "строка")  return vt == Type::Строка || vt == Type::Слово;
    if (typeName == "булево")  return vt == Type::Булево;
    if (typeName == "список")  return vt == Type::Список;
    if (typeName == "словарь") return vt == Type::Словарь;
    if (typeName == "кортеж")  return vt == Type::Кортеж;
    if (typeName == "ничто")   return vt == Type::Ничто;
    return true; // для классов не проверяем строго
}

// ============================================================
//                      ВЫРАЖЕНИЯ
// ============================================================
ValuePtr Interpreter::evalExpr(const ExprPtr& e, ScopePtr env) {
    if (!e) return Value::makeNull();
    switch (e->kind) {

        case ExprKind::Literal:
            return e->literal;

        case ExprKind::Variable: {
            if (env) {
                ValuePtr* p = env->find(e->name);
                if (p) return *p;
            }
            auto it = globals_.find(e->name);
            if (it != globals_.end()) return it->second;

            // встроенные функции — как значения-функции с именем
            if (e->name == "__вывод__" || e->name == "__ввод__"
             || e->name == "__окно__" || e->name == "__цвет__"
             || e->name == "__очистить__" || e->name == "__точка__"
             || e->name == "__линия__" || e->name == "__прямоугольник__"
             || e->name == "__пауза__" || e->name == "__закрыть_окно__") {
                return Value::makeFunction(nullptr);
            }
            auto cit = classes_.find(e->name);
            if (cit != classes_.end()) return Value::makeClass(cit->second);
            raise("ОшибкаИмени", "Имя '" + e->name + "' не определено");
        }

        case ExprKind::Self: {
            if (env && env->self) return env->self;
            raise("ОшибкаИмени", "'сам' вне метода");
        }

        case ExprKind::Base: {
            if (!env || !env->klass || !env->klass->base)
                raise("ОшибкаИмени", "'базовый' вне класса с наследованием");
            auto obj = std::make_shared<Object>();
            obj->klass = env->klass->base;
            if (env->self && env->self->obj) obj->fields = env->self->obj->fields;
            return Value::makeObject(obj);
        }

        case ExprKind::Binary: {
            if (e->op == "и") {
                auto l = evalExpr(e->left, env);
                if (!l->truthy()) return Value::makeBool(false);
                return Value::makeBool(evalExpr(e->right, env)->truthy());
            }
            if (e->op == "или") {
                auto l = evalExpr(e->left, env);
                if (l->truthy()) return Value::makeBool(true);
                return Value::makeBool(evalExpr(e->right, env)->truthy());
            }
            auto a = evalExpr(e->left, env);
            auto b = evalExpr(e->right, env);
            return binaryOp(e->op, a, b);
        }

        case ExprKind::Unary: {
            auto v = evalExpr(e->right, env);
            return unaryOp(e->op, v);
        }

        case ExprKind::ListLit: {
            auto l = Value::makeList();
            for (auto& el : e->elements) l->list->push_back(evalExpr(el, env));
            return l;
        }

        case ExprKind::TupleLit: {
            auto l = Value::makeTuple();
            for (auto& el : e->elements) l->tuple->push_back(evalExpr(el, env));
            return l;
        }

        case ExprKind::DictLit: {
            auto d = Value::makeDict();
            for (auto& kv : e->pairs) {
                auto k = evalExpr(kv.first, env);
                auto v = evalExpr(kv.second, env);
                (*d->dict)[k->toString()] = v;
            }
            return d;
        }

        case ExprKind::Interp: {
            std::string out;
            for (auto& seg : e->segments) {
                if (seg->kind == ExprKind::Literal && seg->literal
                    && seg->literal->type == Type::Строка) {
                    out += seg->literal->s;
                } else {
                    out += evalExpr(seg, env)->toString();
                }
            }
            return Value::makeString(out);
        }

        case ExprKind::Index: {
            auto t = evalExpr(e->target, env);
            auto i = evalExpr(e->index, env);

            if (t->type == Type::Список) {
                if (i->type != Type::Целое)
                    raise("ОшибкаТипа", "Индекс списка — целое");
                int64_t idx = i->i;
                if (idx < 0 || (size_t)idx >= t->list->size())
                    raise("ОшибкаИндекса",
                          "Индекс " + std::to_string(idx) + " вне диапазона");
                return (*t->list)[idx];
            }
            if (t->type == Type::Кортеж) {
                if (i->type != Type::Целое)
                    raise("ОшибкаТипа", "Индекс кортежа — целое");
                int64_t idx = i->i;
                if (idx < 0 || (size_t)idx >= t->tuple->size())
                    raise("ОшибкаИндекса", "Индекс вне диапазона");
                return (*t->tuple)[idx];
            }
            if (t->type == Type::Словарь) {
                auto it = t->dict->find(i->toString());
                if (it == t->dict->end()) return Value::makeNull();
                return it->second;
            }
            if (t->type == Type::Строка || t->type == Type::Слово) {
                if (i->type != Type::Целое)
                    raise("ОшибкаТипа", "Индекс строки — целое");
                int64_t idx = i->i;
                if (idx < 0)
                    raise("ОшибкаИндекса", "Отрицательный индекс");
                size_t pos = 0;
                int64_t cur = 0;
                while (pos < t->s.size()) {
                    size_t st = pos;
                    utf8::decode(t->s, pos);
                    if (cur == idx) {
                        std::string ch = t->s.substr(st, pos - st);
                        if (t->type == Type::Слово)
                            return Value::makeWord(ch);
                        return Value::makeString(ch);
                    }
                    ++cur;
                }
                raise("ОшибкаИндекса", "Индекс вне диапазона");
            }
            raise("ОшибкаТипа",
                  "Индексация неприменима к " + t->typeName());
        }

        case ExprKind::Member: {
            auto t = evalExpr(e->left, env);
            if (t->type == Type::Объект) {
                auto it = t->obj->fields.find(e->name);
                if (it != t->obj->fields.end()) return it->second;
                auto ci = t->obj->klass;
                while (ci) {
                    auto m = ci->methods.find(e->name);
                    if (m != ci->methods.end()) {
                        auto v = Value::makeFunction(m->second);
                        v->obj = t->obj;
                        return v;
                    }
                    ci = ci->base;
                }
                raise("ОшибкаИмени",
                      "Поле/метод '" + e->name + "' не найден");
            }
            raise("ОшибкаТипа",
                  "Обращение к полю у " + t->typeName());
        }

        case ExprKind::Call: {
            // встроенные
            if (e->callee->kind == ExprKind::Variable) {
                const std::string& n = e->callee->name;
                if (n == "__вывод__") {
                    std::vector<ValuePtr> args;
                    for (auto& a : e->args) args.push_back(evalExpr(a, env));
                    return builtinPrint(args);
                }
                if (n == "__ввод__") {
                    std::vector<ValuePtr> args;
                    for (auto& a : e->args) args.push_back(evalExpr(a, env));
                    return builtinInput(args);
                }
                if (n == "__окно__") {
                    std::vector<ValuePtr> args;
                    for (auto& a : e->args) args.push_back(evalExpr(a, env));
                    return builtinWindow(args);
                }
                if (n == "__цвет__") {
                    std::vector<ValuePtr> args;
                    for (auto& a : e->args) args.push_back(evalExpr(a, env));
                    return builtinColor(args);
                }
                if (n == "__очистить__") {
                    std::vector<ValuePtr> args;
                    for (auto& a : e->args) args.push_back(evalExpr(a, env));
                    return builtinClear(args);
                }
                if (n == "__точка__") {
                    std::vector<ValuePtr> args;
                    for (auto& a : e->args) args.push_back(evalExpr(a, env));
                    return builtinDrawPoint(args);
                }
                if (n == "__линия__") {
                    std::vector<ValuePtr> args;
                    for (auto& a : e->args) args.push_back(evalExpr(a, env));
                    return builtinDrawLine(args);
                }
                if (n == "__прямоугольник__") {
                    std::vector<ValuePtr> args;
                    for (auto& a : e->args) args.push_back(evalExpr(a, env));
                    return builtinDrawRect(args);
                }
                if (n == "__пауза__") {
                    std::vector<ValuePtr> args;
                    for (auto& a : e->args) args.push_back(evalExpr(a, env));
                    return builtinSleep(args);
                }
                if (n == "__закрыть_окно__") {
                    std::vector<ValuePtr> args;
                    for (auto& a : e->args) args.push_back(evalExpr(a, env));
                    return builtinCloseWindow(args);
                }
            }

            // метод объекта
            if (e->callee->kind == ExprKind::Member) {
                auto obj = evalExpr(e->callee->left, env);
                if (obj->type == Type::Объект) {
                    auto ci = obj->obj->klass;
                    while (ci) {
                        auto m = ci->methods.find(e->callee->name);
                        if (m != ci->methods.end()) {
                            std::vector<ValuePtr> args;
                            for (auto& a : e->args)
                                args.push_back(evalExpr(a, env));
                            return callFunction(m->second, args, obj, ci, env);
                        }
                        ci = ci->base;
                    }
                    auto it = obj->obj->fields.find(e->callee->name);
                    if (it != obj->obj->fields.end()
                        && it->second->type == Type::Функция) {
                        std::vector<ValuePtr> args;
                        for (auto& a : e->args)
                            args.push_back(evalExpr(a, env));
                        return callFunction(it->second->func, args, obj,
                                            obj->obj->klass, env);
                    }
                    raise("ОшибкаИмени",
                          "Метод '" + e->callee->name + "' не найден");
                }
            }

            // обычная функция
            auto callee = evalExpr(e->callee, env);
            if (callee->type == Type::Функция && callee->func) {
                std::vector<ValuePtr> args;
                for (auto& a : e->args) args.push_back(evalExpr(a, env));
                ValuePtr selfVal = nullptr;
                if (callee->obj) selfVal = Value::makeObject(callee->obj);
                return callFunction(callee->func, args, selfVal, nullptr, env);
            }
            raise("ОшибкаТипа", "Вызов не-функции");
        }

        case ExprKind::New: {
            if (e->callee->kind == ExprKind::Call) {
                auto calleeExpr = e->callee->callee;
                auto ci = classes_.find(calleeExpr->name);
                if (ci == classes_.end())
                    raise("ОшибкаИмени",
                          "Класс '" + calleeExpr->name + "' не найден");

                auto obj = std::make_shared<Object>();
                obj->klass = ci->second;

                auto cur = ci->second;
                while (cur) {
                    for (auto& m : cur->members) {
                        if (m->kind == StmtKind::VarDecl) {
                            ValuePtr v = Value::makeNull();
                            if (m->init) {
                                auto sc = std::make_shared<Scope>();
                                v = evalExpr(m->init, sc);
                            }
                            obj->fields[m->varName] = v;
                        }
                    }
                    cur = cur->base;
                }
                auto v = Value::makeObject(obj);

                auto ci2 = ci->second;
                while (ci2) {
                    auto ctor = ci2->methods.find("создать");
                    if (ctor != ci2->methods.end()) {
                        std::vector<ValuePtr> args;
                        for (auto& a : e->callee->args)
                            args.push_back(evalExpr(a, env));
                        callFunction(ctor->second, args, v, ci2, env);
                        break;
                    }
                    ci2 = ci2->base;
                }
                return v;
            }
            raise("ОшибкаСинтаксиса", "Ожидалось 'новый Класс(...)'");
        }

        default:
            raise("ОшибкаВнутренняя", "Неизвестный узел выражения");
    }
}

// ============================================================
//                    БИНАРНЫЕ ОПЕРАЦИИ
// ============================================================
ValuePtr Interpreter::binaryOp(const std::string& op, ValuePtr a, ValuePtr b) {
    auto num = [](ValuePtr v) {
        return v->type == Type::Целое || v->type == Type::Дробное;
    };
    auto asD = [](ValuePtr v) -> double {
        return v->type == Type::Целое ? (double)v->i : v->d;
    };
    auto asI = [](ValuePtr v) -> int64_t {
        return v->type == Type::Целое ? v->i : (int64_t)v->d;
    };

    if (op == "+") {
        if (num(a) && num(b)) {
            if (a->type == Type::Дробное || b->type == Type::Дробное)
                return Value::makeDouble(asD(a) + asD(b));
            return Value::makeInt(a->i + b->i);
        }
        if (a->type == Type::Список && b->type == Type::Список) {
            auto l = Value::makeList();
            for (auto& x : *a->list) l->list->push_back(x);
            for (auto& x : *b->list) l->list->push_back(x);
            return l;
        }
        // строковая конкатенация через +
        bool as = a->type == Type::Строка || a->type == Type::Слово;
        bool bs = b->type == Type::Строка || b->type == Type::Слово;
        if (as && bs) return Value::makeString(a->s + b->s);
        if (as || bs) return Value::makeString(a->toString() + b->toString());

        raise("ОшибкаТипа",
              "Операция '+' неприменима к " + a->typeName()
              + " и " + b->typeName());
    }
    if (op == "-") {
        if (!num(a) || !num(b)) raise("ОшибкаТипа", "'-' требует чисел");
        if (a->type == Type::Дробное || b->type == Type::Дробное)
            return Value::makeDouble(asD(a) - asD(b));
        return Value::makeInt(a->i - b->i);
    }
    if (op == "*") {
        if (!num(a) || !num(b)) raise("ОшибкаТипа", "'*' требует чисел");
        if (a->type == Type::Дробное || b->type == Type::Дробное)
            return Value::makeDouble(asD(a) * asD(b));
        return Value::makeInt(a->i * b->i);
    }
    if (op == "/") {
        if (!num(a) || !num(b)) raise("ОшибкаТипа", "'/' требует чисел");
        double db = asD(b);
        if (db == 0.0) raise("ОшибкаДеленияНаНоль", "Деление на ноль");
        return Value::makeDouble(asD(a) / db);
    }
    if (op == "%") {
        if (!num(a) || !num(b)) raise("ОшибкаТипа", "'%' требует чисел");
        int64_t ib = asI(b);
        if (ib == 0) raise("ОшибкаДеленияНаНоль", "Деление на ноль");
        return Value::makeInt(asI(a) % ib);
    }
    if (op == "==") {
        if (num(a) && num(b)) return Value::makeBool(asD(a) == asD(b));
        if ((a->type == Type::Строка || a->type == Type::Слово) &&
            (b->type == Type::Строка || b->type == Type::Слово))
            return Value::makeBool(a->s == b->s);
        if (a->type == Type::Булево && b->type == Type::Булево)
            return Value::makeBool(a->b == b->b);
        if (a->type == Type::Ничто && b->type == Type::Ничто)
            return Value::makeBool(true);
        return Value::makeBool(false);
    }
    if (op == "!=") {
        auto r = binaryOp("==", a, b);
        return Value::makeBool(!r->b);
    }
    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (num(a) && num(b)) {
            double da = asD(a), db = asD(b);
            if (op == "<")  return Value::makeBool(da < db);
            if (op == ">")  return Value::makeBool(da > db);
            if (op == "<=") return Value::makeBool(da <= db);
            return Value::makeBool(da >= db);
        }
        if ((a->type == Type::Строка || a->type == Type::Слово) &&
            (b->type == Type::Строка || b->type == Type::Слово)) {
            if (op == "<")  return Value::makeBool(a->s < b->s);
            if (op == ">")  return Value::makeBool(a->s > b->s);
            if (op == "<=") return Value::makeBool(a->s <= b->s);
            return Value::makeBool(a->s >= b->s);
        }
        raise("ОшибкаТипа", "Сравнение неприменимо");
    }
    raise("ОшибкаВнутренняя", "Неизвестный оператор " + op);
}

ValuePtr Interpreter::unaryOp(const std::string& op, ValuePtr a) {
    if (op == "-") {
        if (a->type == Type::Целое)   return Value::makeInt(-a->i);
        if (a->type == Type::Дробное) return Value::makeDouble(-a->d);
        raise("ОшибкаТипа", "Унарный '-' требует числа");
    }
    if (op == "не") return Value::makeBool(!a->truthy());
    raise("ОшибкаВнутренняя", "Неизвестный унарный оператор " + op);
}

// ============================================================
//                     ПРИСВАИВАНИЕ
// ============================================================
void Interpreter::assignTo(const ExprPtr& target, ValuePtr val, ScopePtr env) {
    if (target->kind == ExprKind::Variable) {
        if (env) {
            ValuePtr* p = env->find(target->name);
            if (p) {
                if (env->isConst(target->name)) {
                    raise("ОшибкаИмени",
                          "Нельзя изменить константу '" + target->name + "'");
                }
                *p = val;
                return;
            }
        }
        auto it = globals_.find(target->name);
        if (it != globals_.end()) { it->second = val; return; }
        if (env) { env->vars[target->name] = val; return; }
        globals_[target->name] = val;
        return;
    }
    if (target->kind == ExprKind::Member) {
        auto obj = evalExpr(target->left, env);
        if (obj->type != Type::Объект)
            raise("ОшибкаТипа", "Присваивание полю не-объекту");
        obj->obj->fields[target->name] = val;
        return;
    }
    if (target->kind == ExprKind::Index) {
        auto t = evalExpr(target->target, env);
        auto i = evalExpr(target->index, env);
        if (t->type == Type::Список) {
            int64_t idx = i->i;
            if (idx < 0 || (size_t)idx >= t->list->size())
                raise("ОшибкаИндекса", "Индекс вне диапазона");
            (*t->list)[idx] = val;
            return;
        }
        if (t->type == Type::Словарь) {
            (*t->dict)[i->toString()] = val;
            return;
        }
        raise("ОшибкаТипа", "Индексное присваивание неприменимо");
    }
    raise("ОшибкаСинтаксиса", "Недопустимая цель присваивания");
}

// ============================================================
//                       ВЫЗОВ ФУНКЦИИ
// ============================================================
ValuePtr Interpreter::callFunction(std::shared_ptr<FunctionDecl> fn,
                                   const std::vector<ValuePtr>& args,
                                   ValuePtr self,
                                   std::shared_ptr<ClassInfo> klass,
                                   ScopePtr enclosing) {
    auto scope = std::make_shared<Scope>();
    scope->parent = enclosing;
    scope->self = self;
    scope->klass = klass;
    if (self && self->type == Type::Объект && self->obj) {
        scope->methods = self->obj->klass->methods;
    }
    if (args.size() != fn->params.size()) {
        raise("ОшибкаТипа",
              "Функция '" + fn->name + "' ожидает "
              + std::to_string(fn->params.size()) + " аргументов, получено "
              + std::to_string(args.size()));
    }
    for (size_t i = 0; i < args.size(); ++i) {
        scope->vars[fn->params[i].name] = args[i];
    }
    try {
        execStmt(fn->body, scope);
    } catch (ReturnSignal& r) {
        return r.value ? r.value : Value::makeNull();
    }
    return Value::makeNull();
}

// ============================================================
//                      ИНСТРУКЦИИ
// ============================================================
void Interpreter::execStmt(const StmtPtr& s, ScopePtr env) {
    if (!s) return;
    switch (s->kind) {

        case StmtKind::VarDecl: {
            ValuePtr v = Value::makeNull();
            if (s->init) v = evalExpr(s->init, env);

            bool declaredString = (s->typeName == "строка");
            bool declaredWord   = (s->typeName == "слово");

            // строгое — проверяем совместимость
            if (s->isStrict && !isAssignableStrict(s->typeName, v->type)) {
                raise("ОшибкаТипа",
                      "Переменная '" + s->varName + "' объявлена как '"
                      + s->typeName + "', но значение имеет тип '"
                      + v->typeName() + "'");
            }

            // умное поведение: сумма строк -> числовой тип
            bool fromConcat = false;
            if (s->init && s->init->kind == ExprKind::Binary
                && s->init->op == "+") {
                fromConcat = true;
            }
            if (!s->isStrict && !declaredString && !declaredWord
                && v->type == Type::Строка && fromConcat) {
                std::cerr << "[Вече] Предупреждение: значение переменной '"
                          << s->varName
                          << "' имеет тип 'строка', но объявлено как '"
                          << s->typeName
                          << "'. Сделать переменной типа 'строка'? (д/н) ";
                std::string ans;
                std::getline(std::cin, ans);
                if (!userSaidYes(ans)) {
                    if (s->typeName == "целое") {
                        try { v = Value::makeInt(std::stoll(v->s)); }
                        catch (...) {
                            raise("ОшибкаТипа",
                                  "Нельзя привести строку '" + v->s
                                  + "' к 'целое'");
                        }
                    } else if (s->typeName == "дробь") {
                        try { v = Value::makeDouble(std::stod(v->s)); }
                        catch (...) {
                            raise("ОшибкаТипа",
                                  "Нельзя привести строку '" + v->s
                                  + "' к 'дробь'");
                        }
                    } else if (s->typeName == "булево") {
                        v = Value::makeBool(!v->s.empty()
                                         && v->s != "0" && v->s != "ложь");
                    }
                }
            }

            // слово = без пробелов, '-' и '_'
            if (declaredWord && v->type == Type::Строка) {
                char bad = firstBadCharInWord(v->s);
                if (bad != 0) {
                    std::cerr << "[Вече] Предупреждение: значение переменной '"
                              << s->varName
                              << "' не является 'словом' (содержит '"
                              << bad
                              << "'); тип переменной понижен до 'строка'\n";
                } else {
                    v = Value::makeWord(v->s);
                }
            }

            // символ = один codepoint
            if (s->typeName == "символ"
                && (v->type == Type::Строка || v->type == Type::Слово)) {
                if (!v->s.empty()) {
                    size_t p = 0;
                    utf8::decode(v->s, p);
                    v = Value::makeWord(v->s.substr(0, p));
                }
            }

            if (!env) globals_[s->varName] = v;
            else {
                env->vars[s->varName] = v;
                env->consts[s->varName] = s->isConst;
            }
            break;
        }

        case StmtKind::Assign: {
            auto v = evalExpr(s->assignValue, env);
            assignTo(s->assignTarget, v, env);
            break;
        }

        case StmtKind::ExprStmt:
            evalExpr(s->expr, env);
            break;

        case StmtKind::Block: {
            auto child = std::make_shared<Scope>();
            child->parent = env;
            child->self  = env ? env->self  : nullptr;
            child->klass = env ? env->klass : nullptr;
            for (auto& st : s->body) execStmt(st, child);
            break;
        }

        case StmtKind::If:
            if (evalExpr(s->cond, env)->truthy()) execStmt(s->thenBranch, env);
            else if (s->elseBranch) execStmt(s->elseBranch, env);
            break;

        case StmtKind::While:
            while (evalExpr(s->cond, env)->truthy()) {
                try { execStmt(s->whileBody, env); }
                catch (BreakSignal&)    { break; }
                catch (ContinueSignal&) { continue; }
            }
            break;

        case StmtKind::ForIn: {
            auto coll = evalExpr(s->loopColl, env);
            auto child = std::make_shared<Scope>();
            child->parent = env;
            child->self  = env ? env->self  : nullptr;
            child->klass = env ? env->klass : nullptr;
            auto runOne = [&](ValuePtr v) {
                child->vars[s->loopVar] = v;
                execStmt(s->loopBody, child);
            };
            try {
                if (coll->type == Type::Список) {
                    for (auto& v : *coll->list) runOne(v);
                } else if (coll->type == Type::Кортеж) {
                    for (auto& v : *coll->tuple) runOne(v);
                } else if (coll->type == Type::Словарь) {
                    for (auto& kv : *coll->dict)
                        runOne(Value::makeString(kv.first));
                } else if (coll->type == Type::Строка
                        || coll->type == Type::Слово) {
                    size_t p = 0;
                    while (p < coll->s.size()) {
                        size_t st = p;
                        utf8::decode(coll->s, p);
                        runOne(Value::makeWord(coll->s.substr(st, p - st)));
                    }
                } else {
                    raise("ОшибкаТипа",
                          "Нельзя итерировать по " + coll->typeName());
                }
            } catch (BreakSignal&) { /* выход */ }
            break;
        }

        case StmtKind::ForClassic: {
            auto child = std::make_shared<Scope>();
            child->parent = env;
            if (s->forInit) execStmt(s->forInit, child);
            while (evalExpr(s->cond, child)->truthy()) {
                try { execStmt(s->loopBody, child); }
                catch (BreakSignal&)    { break; }
                catch (ContinueSignal&) { /* идём к шагу */ }
                if (s->forStep) execStmt(s->forStep, child);
            }
            break;
        }

        case StmtKind::Break:    throw BreakSignal{};
        case StmtKind::Continue: throw ContinueSignal{};

        case StmtKind::Return: {
            ReturnSignal r;
            r.value = s->returnValue ? evalExpr(s->returnValue, env)
                                     : Value::makeNull();
            throw r;
        }

        case StmtKind::FunctionDecl: {
            auto fd = std::make_shared<FunctionDecl>();
            fd->name = s->funcName;
            fd->params = s->params;
            fd->returnType = s->returnType;
            fd->body = s->funcBody;
            auto v = Value::makeFunction(fd);
            if (env) env->vars[s->funcName] = v;
            else     globals_[s->funcName] = v;
            break;
        }

        case StmtKind::ClassDecl: {
            auto ci = std::make_shared<ClassInfo>();
            ci->name = s->className;
            ci->baseName = s->baseName;
            if (!s->baseName.empty()) {
                auto it = classes_.find(s->baseName);
                if (it == classes_.end())
                    raise("ОшибкаИмени",
                          "Базовый класс '" + s->baseName + "' не найден");
                ci->base = it->second;
            }
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
            auto v = Value::makeClass(ci);
            if (env) env->vars[s->className] = v;
            else     globals_[s->className] = v;
            break;
        }

        case StmtKind::Try: {
            try {
                execStmt(s->tryBody, env);
            } catch (VecheError& e) {
                if (!s->catchBody) throw;
                auto child = std::make_shared<Scope>();
                child->parent = env;
                auto obj = std::make_shared<Object>();
                obj->fields["сообщение"] = Value::makeString(e.message);
                obj->fields["тип"]       = Value::makeString(e.kind);
                child->vars[s->catchVar] = Value::makeObject(obj);
                execStmt(s->catchBody, child);
            } catch (ReturnSignal&)   { throw; }
              catch (BreakSignal&)    { throw; }
              catch (ContinueSignal&) { throw; }
            if (s->finallyBody) execStmt(s->finallyBody, env);
            break;
        }

        case StmtKind::Raise: {
            std::string msg = "возбуждено";
            if (s->raiseArg) msg = evalExpr(s->raiseArg, env)->toString();
            raise(s->raiseKind.empty() ? "Ошибка" : s->raiseKind, msg);
        }

        case StmtKind::Print: {
            std::vector<ValuePtr> args;
            for (auto& a : s->printArgs) args.push_back(evalExpr(a, env));
            builtinPrint(args);
            break;
        }

        default: break;
    }
}

void Interpreter::runIn(const std::vector<StmtPtr>& prog, ScopePtr env) {
    if (!env) env = std::make_shared<Scope>();
    for (auto& s : prog) {
        try { execStmt(s, env); }
        catch (ReturnSignal&) { break; }
    }
}

void Interpreter::run(const std::vector<StmtPtr>& prog) {
    auto env = std::make_shared<Scope>();
    runIn(prog, env);
}

} // namespace veche