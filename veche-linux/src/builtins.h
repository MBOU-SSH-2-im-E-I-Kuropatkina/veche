#pragma once
#include "value.h"
#include <string>
#include <vector>

namespace veche {

ValuePtr builtinPrint(const std::vector<ValuePtr>& args);
ValuePtr builtinInput(const std::vector<ValuePtr>& args);

// Графика
ValuePtr builtinWindow(const std::vector<ValuePtr>& args);
ValuePtr builtinColor(const std::vector<ValuePtr>& args);
ValuePtr builtinClear(const std::vector<ValuePtr>& args);
ValuePtr builtinDrawPoint(const std::vector<ValuePtr>& args);
ValuePtr builtinDrawLine(const std::vector<ValuePtr>& args);
ValuePtr builtinDrawRect(const std::vector<ValuePtr>& args);
ValuePtr builtinSleep(const std::vector<ValuePtr>& args);
ValuePtr builtinCloseWindow(const std::vector<ValuePtr>& args);

} // namespace veche