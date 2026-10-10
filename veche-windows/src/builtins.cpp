#include "builtins.h"
#include "graphics.h"
#include "errors.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <windows.h> // <-- ВАЖНО для Windows

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

ValuePtr builtinKeyPressed(const std::vector<ValuePtr>& args) {
    if (args.empty() || !args[0]) return Value::makeBool(false);
    std::string keyName = args[0]->toString();
    std::transform(keyName.begin(), keyName.end(), keyName.begin(), ::tolower);

    static const std::unordered_map<std::string, int> winKeyMap = {
        {"ctrl", VK_CONTROL}, {"lctrl", VK_LCONTROL}, {"rctrl", VK_RCONTROL},
        {"shift", VK_SHIFT}, {"lshift", VK_LSHIFT}, {"rshift", VK_RSHIFT},
        {"alt", VK_MENU}, {"lalt", VK_LMENU}, {"ralt", VK_RMENU},
        {"space", VK_SPACE}, {"enter", VK_RETURN}, {"esc", VK_ESCAPE},
        {"backspace", VK_BACK}, {"tab", VK_TAB},
        {"up", VK_UP}, {"down", VK_DOWN}, {"left", VK_LEFT}, {"right", VK_RIGHT},
        {"f1", VK_F1}, {"f2", VK_F2}, {"f3", VK_F3}, {"f4", VK_F4},
        {"f5", VK_F5}, {"f6", VK_F6}, {"f7", VK_F7}, {"f8", VK_F8},
        {"f9", VK_F9}, {"f10", VK_F10}, {"f11", VK_F11}, {"f12", VK_F12}
    };

    int vk = -1;
    if (auto it = winKeyMap.find(keyName); it != winKeyMap.end()) {
        vk = it->second;
    } else if (keyName.size() == 1) {
        char c = keyName[0];
        if (c >= 'a' && c <= 'z') vk = 'A' + (c - 'a');
        else if (c >= '0' && c <= '9') vk = c;
    }

    if (vk == -1) return Value::makeBool(false);
    
    // Проверяем старший бит (0x8000), чтобы узнать, нажата ли клавиша прямо сейчас
    return Value::makeBool(GetAsyncKeyState(vk) & 0x8000);
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

// ============================================================
//                  ВСТРОЕННЫЕ ФУНКЦИИ СПИСКА
// ============================================================

ValuePtr builtinLen(const std::vector<ValuePtr>& args) {
    if (args.empty() || !args[0]) return Value::makeInt(0);
    auto v = args[0];
    switch (v->type) {
        case Type::Строка:
        case Type::Слово:
            return Value::makeInt((int64_t)v->s.size());
        case Type::Список:
            return Value::makeInt((int64_t)v->list->size());
        case Type::Кортеж:
            return Value::makeInt((int64_t)v->tuple->size());
        case Type::Словарь:
            return Value::makeInt((int64_t)v->dict->size());
        default:
            return Value::makeInt(0);
    }
}

ValuePtr builtinAdd(const std::vector<ValuePtr>& args) {
    if (args.size() < 2) return Value::makeNull();
    auto lst = args[0];
    auto el  = args[1];
    if (!lst || lst->type != Type::Список) {
        throw VecheError("ОшибкаТипа",
            "добавить: первый аргумент должен быть 'список'");
    }
    lst->list->push_back(el);
    return Value::makeNull();
}

ValuePtr builtinRemove(const std::vector<ValuePtr>& args) {
    if (args.size() < 2) return Value::makeNull();
    auto lst = args[0];
    auto idx = args[1];
    if (!lst || lst->type != Type::Список) {
        throw VecheError("ОшибкаТипа",
            "удалить: первый аргумент должен быть 'список'");
    }
    if (!idx || idx->type != Type::Целое) {
        throw VecheError("ОшибкаТипа",
            "удалить: второй аргумент должен быть 'целое'");
    }
    int64_t i = idx->i;
    if (i < 0 || (size_t)i >= lst->list->size()) {
        throw VecheError("ОшибкаИндекса",
            "удалить: индекс " + std::to_string(i) + " вне диапазона");
    }
    lst->list->erase(lst->list->begin() + i);
    return Value::makeNull();
}

ValuePtr builtinSwap(const std::vector<ValuePtr>& args) {
    if (args.size() < 3) return Value::makeNull();
    auto lst = args[0];
    auto a   = args[1];
    auto b   = args[2];
    if (!lst || lst->type != Type::Список) {
        throw VecheError("ОшибкаТипа",
            "обмен: первый аргумент должен быть 'список'");
    }
    if (!a || a->type != Type::Целое || !b || b->type != Type::Целое) {
        throw VecheError("ОшибкаТипа",
            "обмен: индексы должны быть 'целое'");
    }
    int64_t i = a->i, j = b->i;
    size_t sz = lst->list->size();
    if (i < 0 || (size_t)i >= sz || j < 0 || (size_t)j >= sz) {
        throw VecheError("ОшибкаИндекса",
            "обмен: индекс вне диапазона");
    }
    std::swap((*lst->list)[i], (*lst->list)[j]);
    return Value::makeNull();
}

ValuePtr builtinIndex(const std::vector<ValuePtr>& args) {
    if (args.size() < 2) return Value::makeInt(-1);
    auto lst = args[0];
    auto el  = args[1];
    if (!lst || lst->type != Type::Список) {
        throw VecheError("ОшибкаТипа",
            "индекс: первый аргумент должен быть 'список'");
    }
    std::string needle = el->toString();
    for (size_t i = 0; i < lst->list->size(); ++i) {
        if ((*lst->list)[i]->toString() == needle) {
            return Value::makeInt((int64_t)i);
        }
    }
    return Value::makeInt(-1);
}

} // namespace veche