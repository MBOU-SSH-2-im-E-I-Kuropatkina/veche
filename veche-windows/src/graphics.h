#pragma once
#include <string>
#include <cstdint>

namespace veche {
namespace graphics {

// Инициализация окна (создаёт окно, если ещё не создано).
// Возвращает true при успехе. На не-Windows — заглушка.
bool openWindow(int width, int height, const std::string& title = "Вече");

// Примитивы. Координаты — пиксели, (0,0) — левый верхний угол.
void setColor(int r, int g, int b);
void clear();
void drawPoint(int x, int y);
void drawLine(int x1, int y1, int x2, int y2);
void drawRect(int x, int y, int w, int h);

// Пауза в миллисекундах (для анимации).
void sleepMs(int ms);

// Закрыть окно (если открыто).
void closeWindow();

// Обработать сообщения окна (вызывается автоматически в sleep).
void pumpMessages();

} // namespace graphics
} // namespace veche