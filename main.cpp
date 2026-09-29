/**
* Author: Alan Chen
* Assignment: Simple 2D Scene
* Date due: 08/05/2026
*
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "lib.h"

constexpr int SCREEN_WIDTH = 1600;
constexpr int SCREEN_HEIGHT = 900;
constexpr int TARGET_FPS = 60;

constexpr Vector2 SCREEN_CENTER = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};

std::string TEXTURE_PATHS[] = {"assets/bruh.png", "assets/2.png"};

AppStatus gAppStatus = AppStatus::RUNNING;
float gPreviousTicks = 0.0f;

void initialize(void);
void processInput(void);
void update(void);
void render(void);
void shutdown(void);

void initialize(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "An Awesome Scene");
    SetTargetFPS(TARGET_FPS);
}

void processInput(void) {
    if (WindowShouldClose()) {
        gAppStatus = TERMINATED;
    }
}

void update(void) {
    float ticks = static_cast<float>(GetTime());
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks = ticks;
}

void render(void) {
    BeginDrawing();

    EndDrawing();
}

void shutdown(void) {
    CloseWindow();
}

int main(void) {
    initialize();

    while (gAppStatus == RUNNING) {
        processInput();
        update();
        render();
    }

    shutdown();

    return 0;
}