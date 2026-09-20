#include "builtins.h"
#include "graphics.h"
#include <iostream>
#include <string>

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

static int toInt(ValuePtr v, int def = 0) {
    if (!v) return def;
    if (v->type == Type::Целое)   return (int)v->i;
    if (v->type == Type::Дробное) return (int)v->d;
    return def;
}

ValuePtr builtinWindow(const std::vector<ValuePtr>& args) {
    int w = args.size() > 0 ? toInt(args[0], 640) : 640;
    int h = args.size() > 1 ? toInt(args[1], 480) : 480;
    std::string title = "Вече";
    if (args.size() > 2 && args[2]) title = args[2]->toString();
    bool ok = graphics::openWindow(w, h, title);
    return Value::makeBool(ok);
}

ValuePtr builtinColor(const std::vector<ValuePtr>& args) {
    int r = args.size() > 0 ? toInt(args[0]) : 255;
    int g = args.size() > 1 ? toInt(args[1]) : 255;
    int b = args.size() > 2 ? toInt(args[2]) : 255;
    graphics::setColor(r, g, b);
    return Value::makeNull();
}

ValuePtr builtinClear(const std::vector<ValuePtr>&) {
    graphics::clear();
    return Value::makeNull();
}

ValuePtr builtinDrawPoint(const std::vector<ValuePtr>& args) {
    int x = args.size() > 0 ? toInt(args[0]) : 0;
    int y = args.size() > 1 ? toInt(args[1]) : 0;
    graphics::drawPoint(x, y);
    return Value::makeNull();
}

ValuePtr builtinDrawLine(const std::vector<ValuePtr>& args) {
    int x1 = args.size() > 0 ? toInt(args[0]) : 0;
    int y1 = args.size() > 1 ? toInt(args[1]) : 0;
    int x2 = args.size() > 2 ? toInt(args[2]) : 0;
    int y2 = args.size() > 3 ? toInt(args[3]) : 0;
    graphics::drawLine(x1, y1, x2, y2);
    return Value::makeNull();
}

ValuePtr builtinDrawRect(const std::vector<ValuePtr>& args) {
    int x = args.size() > 0 ? toInt(args[0]) : 0;
    int y = args.size() > 1 ? toInt(args[1]) : 0;
    int w = args.size() > 2 ? toInt(args[2]) : 0;
    int h = args.size() > 3 ? toInt(args[3]) : 0;
    graphics::drawRect(x, y, w, h);
    return Value::makeNull();
}

ValuePtr builtinSleep(const std::vector<ValuePtr>& args) {
    int ms = args.size() > 0 ? toInt(args[0]) : 0;
    graphics::sleepMs(ms);
    return Value::makeNull();
}

ValuePtr builtinCloseWindow(const std::vector<ValuePtr>&) {
    graphics::closeWindow();
    return Value::makeNull();
}

} // namespace veche