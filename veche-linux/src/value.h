#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <utility>

namespace veche {

// ---- Forward declarations ----
struct Value;
struct Stmt;
struct ClassInfo;
struct FunctionDecl;

using ValuePtr = std::shared_ptr<Value>;
using ListPtr  = std::shared_ptr<std::vector<ValuePtr>>;
using DictPtr  = std::shared_ptr<std::unordered_map<std::string, ValuePtr>>;
using TuplePtr = std::shared_ptr<std::vector<ValuePtr>>;
using ObjPtr   = std::shared_ptr<struct Object>;
using StmtPtr  = std::shared_ptr<Stmt>;

// ---- Param / FunctionDecl / ClassInfo: full definitions BEFORE Value ----
struct Param {
    std::string type;
    std::string name;
};

struct FunctionDecl {
    std::string name;
    std::vector<Param> params;
    std::string returnType;
    StmtPtr body;
    std::shared_ptr<ClassInfo> owner;
};

struct ClassInfo {
    std::string name;
    std::string baseName;
    std::shared_ptr<ClassInfo> base;
    std::vector<StmtPtr> members;
    std::unordered_map<std::string, std::shared_ptr<FunctionDecl>> methods;
};

// ---- Type tag ----
enum class Type {
    Целое, Дробное, Слово, Строка, Булево,
    Список, Словарь, Кортеж, Ничто,
    Объект, Функция, Класс
};

// ---- Value ----
struct Value {
    Type type = Type::Ничто;

    int64_t i = 0;
    double  d = 0.0;
    bool    b = false;
    std::string s;          // строка / слово
    ListPtr  list;
    DictPtr  dict;
    TuplePtr tuple;
    ObjPtr   obj;
    std::shared_ptr<ClassInfo>    klass;
    std::shared_ptr<FunctionDecl> func;

    Value() = default;

    static ValuePtr makeInt(int64_t v);
    static ValuePtr makeDouble(double v);
    static ValuePtr makeBool(bool v);
    static ValuePtr makeWord(const std::string& s);
    static ValuePtr makeString(const std::string& s);
    static ValuePtr makeList();
    static ValuePtr makeDict();
    static ValuePtr makeTuple();
    static ValuePtr makeNull();
    static ValuePtr makeObject(ObjPtr o);
    static ValuePtr makeClass(std::shared_ptr<ClassInfo> k);
    static ValuePtr makeFunction(std::shared_ptr<FunctionDecl> f);

    std::string typeName() const;
    std::string toString() const;
    bool truthy() const;
};

struct Object {
    std::shared_ptr<ClassInfo> klass;
    std::unordered_map<std::string, ValuePtr> fields;
};

} // namespace veche