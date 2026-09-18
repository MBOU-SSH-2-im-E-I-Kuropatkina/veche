#pragma once
#include "value.h"
#include <string>
#include <vector>

namespace veche {

// Built-in functions and helpers.
ValuePtr builtinPrint(const std::vector<ValuePtr>& args);
ValuePtr builtinInput(const std::vector<ValuePtr>& args);
ValuePtr builtinLen(const std::vector<ValuePtr>& args);
ValuePtr builtinStr(const std::vector<ValuePtr>& args);

} // namespace veche