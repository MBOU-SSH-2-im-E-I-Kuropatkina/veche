#include "graphics.h"
#include <iostream>

#if defined(__has_include)
#  if __has_include(<SDL2/SDL.h>)
#    define VECHE_HAS_SDL2 1
#  endif
#endif

#ifdef VECHE_HAS_SDL2

#include <SDL2/SDL.h>

namespace veche { namespace graphics {

static SDL_Window*   g_win = nullptr;
static SDL_Renderer* g_ren = nullptr;
static int           g_w = 0;
static int           g_h = 0;
static Uint8         g_r = 255, g_g = 255, g_b = 255;

bool openWindow(int width, int height, const std::string& title) {
    if (g_win) return true;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "[Вече] SDL_Init: " << SDL_GetError() << "\n";
        return false;
    }

    g_win = SDL_CreateWindow(title.c_str(),
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             width, height, SDL_WINDOW_SHOWN);
    if (!g_win) {
        std::cerr << "[Вече] SDL_CreateWindow: " << SDL_GetError() << "\n";
        SDL_Quit();
        return false;
    }

    g_ren = SDL_CreateRenderer(g_win, -1,
                               SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_ren) g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_SOFTWARE);
    if (!g_ren) {
        std::cerr << "[Вече] SDL_CreateRenderer: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(g_win); g_win = nullptr;
        SDL_Quit();
        return false;
    }

    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
    SDL_RenderClear(g_ren);
    SDL_RenderPresent(g_ren);

    g_w = width;
    g_h = height;
    return true;
}

void setColor(int r, int g, int b) {
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    g_r = (Uint8)r; g_g = (Uint8)g; g_b = (Uint8)b;
    if (g_ren) SDL_SetRenderDrawColor(g_ren, g_r, g_g, g_b, 255);
}

void clear() {
    if (!g_ren) return;
    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
    SDL_RenderClear(g_ren);
    SDL_SetRenderDrawColor(g_ren, g_r, g_g, g_b, 255);
}

void drawPoint(int x, int y) {
    if (!g_ren) return;
    SDL_RenderDrawPoint(g_ren, x, y);
}

void drawLine(int x1, int y1, int x2, int y2) {
    if (!g_ren) return;
    SDL_RenderDrawLine(g_ren, x1, y1, x2, y2);
}

void drawRect(int x, int y, int w, int h) {
    if (!g_ren) return;
    SDL_Rect r{x, y, w, h};
    SDL_RenderDrawRect(g_ren, &r);
}

void pumpMessages() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) closeWindow();
    }
}

void sleepMs(int ms) {
    if (g_ren) SDL_RenderPresent(g_ren);
    Uint32 end = SDL_GetTicks() + (Uint32)ms;
    while (SDL_GetTicks() < end) {
        pumpMessages();
        SDL_Delay(5);
    }
    if (g_ren) SDL_RenderPresent(g_ren);
}

void closeWindow() {
    if (g_ren) { SDL_DestroyRenderer(g_ren); g_ren = nullptr; }
    if (g_win) { SDL_DestroyWindow(g_win);  g_win = nullptr; }
    SDL_Quit();
}

}} // namespace

#else

namespace veche { namespace graphics {

bool openWindow(int, int, const std::string&) {
    std::cerr << "[Вече] Графика недоступна: установите SDL2 "
                 "(sudo apt install libsdl2-dev) и пересоберите\n";
    return false;
}
void setColor(int, int, int) {}
void clear() {}
void drawPoint(int, int) {}
void drawLine(int, int, int, int) {}
void drawRect(int, int, int, int) {}
void sleepMs(int ms) { (void)ms; }
void closeWindow() {}
void pumpMessages() {}

}} // namespace

#endif