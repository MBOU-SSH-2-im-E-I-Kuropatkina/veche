#include "builtins.h"
#include <iostream>
#include <limits>

namespace veche {

ValuePtr builtinPrint(const std::vector<ValuePtr>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (i) std::cout << ' ';
        std::cout << args[i]->toString();
    }
    std::cout << '\n';
    return Value::makeNull();
}

ValuePtr builtinInput(const std::vector<ValuePtr>& args) {
    if (!args.empty()) {
        std::cout << args[0]->toString();
        std::cout.flush();
    }
    std::string line;
    if (!std::getline(std::cin, line)) return Value::makeNull();
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return Value::makeString(line);
}

ValuePtr builtinLen(const std::vector<ValuePtr>& args) {
    if (args.empty()) return Value::makeInt(0);
    auto v = args[0];
    switch (v->type) {
        case Type::Строка:
        case Type::Слово:  return Value::makeInt((int64_t)v->s.size());
        case Type::Список: return Value::makeInt((int64_t)v->list->size());
        case Type::Кортеж: return Value::makeInt((int64_t)v->tuple->size());
        case Type::Словарь:return Value::makeInt((int64_t)v->dict->size());
        default: return Value::makeInt(0);
    }
}

ValuePtr builtinStr(const std::vector<ValuePtr>& args) {
    if (args.empty()) return Value::makeString("");
    return Value::makeString(args[0]->toString());
}

} // namespace veche