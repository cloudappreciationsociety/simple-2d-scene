#ifndef LIB_H
#define LIB_H

#include <iostream>

#include "raylib.h"

#define LOG(argument) std::cout << argument << '\n'

enum AppStatus { TERMINATED, RUNNING };

Color ColorFromHex(const char* hex);

#endif