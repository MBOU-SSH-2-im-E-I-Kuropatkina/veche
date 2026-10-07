#include "value.h"
#include <sstream>

namespace veche {

// ---------------- Factory ----------------
ValuePtr Value::makeInt(int64_t v) {
    auto p = std::make_shared<Value>();
    p->type = Type::Целое; p->i = v; return p;
}
ValuePtr Value::makeDouble(double v) {
    auto p = std::make_shared<Value>();
    p->type = Type::Дробное; p->d = v; return p;
}
ValuePtr Value::makeBool(bool v) {
    auto p = std::make_shared<Value>();
    p->type = Type::Булево; p->b = v; return p;
}
ValuePtr Value::makeWord(const std::string& s) {
    auto p = std::make_shared<Value>();
    p->type = Type::Слово; p->s = s; return p;
}
ValuePtr Value::makeString(const std::string& s) {
    auto p = std::make_shared<Value>();
    p->type = Type::Строка; p->s = s; return p;
}
ValuePtr Value::makeList() {
    auto p = std::make_shared<Value>();
    p->type = Type::Список;
    p->list = std::make_shared<std::vector<ValuePtr>>();
    return p;
}
ValuePtr Value::makeDict() {
    auto p = std::make_shared<Value>();
    p->type = Type::Словарь;
    p->dict = std::make_shared<std::unordered_map<std::string, ValuePtr>>();
    return p;
}
ValuePtr Value::makeTuple() {
    auto p = std::make_shared<Value>();
    p->type = Type::Кортеж;
    p->tuple = std::make_shared<std::vector<ValuePtr>>();
    return p;
}
ValuePtr Value::makeNull() {
    auto p = std::make_shared<Value>();
    p->type = Type::Ничто; return p;
}
ValuePtr Value::makeObject(ObjPtr o) {
    auto p = std::make_shared<Value>();
    p->type = Type::Объект; p->obj = std::move(o); return p;
}
ValuePtr Value::makeClass(std::shared_ptr<ClassInfo> k) {
    auto p = std::make_shared<Value>();
    p->type = Type::Класс; p->klass = std::move(k); return p;
}
ValuePtr Value::makeFunction(std::shared_ptr<FunctionDecl> f) {
    auto p = std::make_shared<Value>();
    p->type = Type::Функция; p->func = std::move(f); return p;
}

// ---------------- typeName ----------------
std::string Value::typeName() const {
    switch (type) {
        case Type::Целое:   return "целое";
        case Type::Дробное: return "дробное";
        case Type::Слово:   return "слово";
        case Type::Строка:  return "строка";
        case Type::Булево:  return "булево";
        case Type::Список:  return "список";
        case Type::Словарь: return "словарь";
        case Type::Кортеж:  return "кортеж";
        case Type::Ничто:   return "ничто";
        case Type::Объект:
            // ClassInfo is fully defined in value.h — safe to dereference.
            return (obj && obj->klass) ? obj->klass->name : std::string("объект");
        case Type::Функция: return "функция";
        case Type::Класс:   return "класс";
    }
    return "неизвестно";
}

// ---------------- toString ----------------
std::string Value::toString() const {
    std::ostringstream os;
    switch (type) {
        case Type::Целое:   os << i; return os.str();
        case Type::Дробное: os << d; return os.str();
        case Type::Булево:  return b ? "истина" : "ложь";
        case Type::Слово:
        case Type::Строка:  return s;
        case Type::Ничто:   return "ничто";
        case Type::Список: {
            os << "[";
            if (list) {
                for (size_t k = 0; k < list->size(); ++k) {
                    if (k) os << ", ";
                    os << (*list)[k]->toString();
                }
            }
            os << "]";
            return os.str();
        }
        case Type::Кортеж: {
            os << "(";
            if (tuple) {
                for (size_t k = 0; k < tuple->size(); ++k) {
                    if (k) os << ", ";
                    os << (*tuple)[k]->toString();
                }
                if (tuple->size() == 1) os << ",";
            }
            os << ")";
            return os.str();
        }
        case Type::Словарь: {
            os << "{";
            if (dict) {
                bool first = true;
                for (auto& kv : *dict) {
                    if (!first) os << ", ";
                    first = false;
                    os << "\"" << kv.first << "\": " << kv.second->toString();
                }
            }
            os << "}";
            return os.str();
        }
        case Type::Объект:
            return "<объект " + typeName() + ">";
        case Type::Функция:
            return "<функция>";
        case Type::Класс:
            // klass is std::shared_ptr<ClassInfo>; ClassInfo fully defined.
            return "<класс " + (klass ? klass->name : std::string("?")) + ">";
    }
    return os.str();
}

// ---------------- truthy ----------------
bool Value::truthy() const {
    switch (type) {
        case Type::Ничто:   return false;
        case Type::Булево:  return b;
        case Type::Целое:   return i != 0;
        case Type::Дробное: return d != 0.0;
        case Type::Слово:
        case Type::Строка:  return !s.empty();
        case Type::Список:  return list && !list->empty();
        case Type::Кортеж:  return tuple && !tuple->empty();
        case Type::Словарь: return dict && !dict->empty();
        default:            return true;
    }
}

} // namespace veche