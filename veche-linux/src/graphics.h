#pragma once
#include <string>
#include <cstdint>

namespace veche {
namespace graphics {

bool openWindow(int width, int height, const std::string& title = "Вече");
void setColor(int r, int g, int b);
void clear();
void drawPoint(int x, int y);
void drawLine(int x1, int y1, int x2, int y2);
void drawRect(int x, int y, int w, int h);
void sleepMs(int ms);
void closeWindow();
void pumpMessages();

} // namespace graphics
} // namespace veche