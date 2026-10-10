#include "builtins.h"
#include "graphics.h"
#include "errors.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_map>

#if defined(__has_include)
#  if __has_include(<SDL2/SDL.h>)
#    define VECHE_HAS_SDL2 1
#  endif
#endif

#ifdef VECHE_HAS_SDL2
#include <SDL2/SDL.h>
#endif

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

#ifdef VECHE_HAS_SDL2
    // Инициализируем SDL, если он еще не инициализирован графикой
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_Init(SDL_INIT_VIDEO);
    }

    static const std::unordered_map<std::string, SDL_Scancode> sdlKeyMap = {
        {"ctrl", SDL_SCANCODE_LCTRL}, {"lctrl", SDL_SCANCODE_LCTRL}, {"rctrl", SDL_SCANCODE_RCTRL},
        {"shift", SDL_SCANCODE_LSHIFT}, {"lshift", SDL_SCANCODE_LSHIFT}, {"rshift", SDL_SCANCODE_RSHIFT},
        {"alt", SDL_SCANCODE_LALT}, {"lalt", SDL_SCANCODE_LALT}, {"ralt", SDL_SCANCODE_RALT},
        {"space", SDL_SCANCODE_SPACE}, {"enter", SDL_SCANCODE_RETURN}, {"esc", SDL_SCANCODE_ESCAPE},
        {"backspace", SDL_SCANCODE_BACKSPACE}, {"tab", SDL_SCANCODE_TAB},
        {"up", SDL_SCANCODE_UP}, {"down", SDL_SCANCODE_DOWN}, {"left", SDL_SCANCODE_LEFT}, {"right", SDL_SCANCODE_RIGHT},
        {"f1", SDL_SCANCODE_F1}, {"f2", SDL_SCANCODE_F2}, {"f3", SDL_SCANCODE_F3}, {"f4", SDL_SCANCODE_F4},
        {"f5", SDL_SCANCODE_F5}, {"f6", SDL_SCANCODE_F6}, {"f7", SDL_SCANCODE_F7}, {"f8", SDL_SCANCODE_F8},
        {"f9", SDL_SCANCODE_F9}, {"f10", SDL_SCANCODE_F10}, {"f11", SDL_SCANCODE_F11}, {"f12", SDL_SCANCODE_F12}
    };

    SDL_Scancode sc = SDL_SCANCODE_UNKNOWN;
    if (auto it = sdlKeyMap.find(keyName); it != sdlKeyMap.end()) {
        sc = it->second;
    } else if (keyName.size() == 1) {
        char c = keyName[0];
        if (c >= 'a' && c <= 'z') sc = (SDL_Scancode)(SDL_SCANCODE_A + (c - 'a'));
        else if (c >= '0' && c <= '9') sc = (SDL_Scancode)(SDL_SCANCODE_0 + (c - '0'));
    }

    if (sc == SDL_SCANCODE_UNKNOWN) return Value::makeBool(false);

    SDL_PumpEvents(); // Обновляем состояние клавиатуры
    const Uint8* state = SDL_GetKeyboardState(NULL);
    
    // Для модификаторов проверяем обе стороны (левую и правую)
    if (keyName == "ctrl") {
        return Value::makeBool(state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL]);
    }
    if (keyName == "shift") {
        return Value::makeBool(state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT]);
    }
    if (keyName == "alt") {
        return Value::makeBool(state[SDL_SCANCODE_LALT] || state[SDL_SCANCODE_RALT]);
    }
    
    return Value::makeBool(state[sc] != 0);
#else
    return Value::makeBool(false);
#endif
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