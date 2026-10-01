#ifndef LIB_H
#define LIB_H

#include <iostream>

#include "raylib.h"

#define LOG(argument) std::cout << (argument) << '\n'

constexpr float clamp(float value, float minimum, float maximum) {
    return (value > maximum) ? maximum : (value < minimum) ? minimum : value;
}

template<typename T>
constexpr T lerp(T a, T b, float t) {
    return a + (b - a) * t;
}

constexpr Color lerpColor(Color a, Color b, float t) {
    return Color {
        lerp(a.r, b.r, t),
        lerp(a.g, b.g, t),
        lerp(a.b, b.b, t),
        lerp(a.a, b.a, t),
    };
}

constexpr Vector2 lerpVector2(Vector2 a, Vector2 b, float t) {
    return Vector2 {lerp(a.x, b.x, t), lerp(a.y, b.y, t)};
}

enum AppStatus { TERMINATED, RUNNING };

Color ColorFromHex(const char* hex);

#endif