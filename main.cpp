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

#include <array>
#include <cstdint>
#include <vector>

#include "lib.h"

constexpr int32_t SCREEN_WIDTH = 1600;
constexpr int32_t SCREEN_HEIGHT = 900;
constexpr int32_t TARGET_FPS = 60;
constexpr Vector2 SCREEN_CENTER = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
constexpr Vector2 SCREEN_SIZE = {
    static_cast<float>(SCREEN_WIDTH),
    static_cast<float>(SCREEN_HEIGHT)
};

// There can be only one of each scene object, and they're loaded in the order of declaration here.
enum SceneObject { TREES_BG, TREES_MG, TREES_FG, SUN, COUNT };

constexpr std::array<const char*, SceneObject::COUNT> TEXTURE_PATHS = {
    "assets/trees-background.png",
    "assets/trees-midground.png",
    "assets/trees-foreground.png",
    "assets/sun.png"
};

// =============================================================================
// Object-specific constants
// =============================================================================

// Parallax backgrounds
constexpr float BG_SCROLL_SPEED = 7.5f;
constexpr float MG_SCROLL_SPEED = 15.0f;
constexpr float FG_SCROLL_SPEED = 25.0f;

// =============================================================================
// App-level globals
// =============================================================================
AppStatus gAppStatus = AppStatus::RUNNING;
float gPreviousTicks = 0.0f;
uint32_t gFrameCount = 0;

// =============================================================================
// Object-specific globals
// =============================================================================

// Parallax
float gBgOffset = 0.0f;
float gMgOffset = 0.0f;
float gFgOffset = 0.0f;

// Wraps all fields needed for rendering
struct Object {
    Texture2D texture;
    Vector2 position;
    Vector2 size;
    float rotation;
    Color tint;

    Vector2 uvOrigin;
    Vector2 uvSize;
};

std::vector<Object> gObjects;

void initialize(void);
void processInput(void);
void update(void);
void render(void);
void shutdown(void);

void initialize(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "An Awesome Scene");
    SetTargetFPS(TARGET_FPS);

    // Load all textures
    for (const char* path : TEXTURE_PATHS) {
        Object object = {0};
        object.texture = LoadTexture(path);
        object.position = SCREEN_CENTER;
        object.tint = WHITE;
        object.uvSize = {
            static_cast<float>(object.texture.width),
            static_cast<float>(object.texture.height)
        };

        gObjects.push_back(object);
    }

    gObjects[SceneObject::SUN].size = {480.0f, 480.0f};
    gObjects[SceneObject::SUN].position = SCREEN_CENTER;

    // Parallax backgrounds take up the entire screen
    gObjects[SceneObject::TREES_FG].size = SCREEN_SIZE;
    gObjects[SceneObject::TREES_MG].size = SCREEN_SIZE;
    gObjects[SceneObject::TREES_BG].size = SCREEN_SIZE;
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

    // Parallax scroll
    gBgOffset += BG_SCROLL_SPEED * deltaTime;
    gMgOffset += MG_SCROLL_SPEED * deltaTime;
    gFgOffset += FG_SCROLL_SPEED * deltaTime;

    gObjects[SceneObject::TREES_BG].uvOrigin.x = gBgOffset;
    gObjects[SceneObject::TREES_MG].uvOrigin.x = gMgOffset;
    gObjects[SceneObject::TREES_FG].uvOrigin.x = gFgOffset;
}

void render(void) {
    BeginDrawing();

    ClearBackground(SKYBLUE);

    for (const auto& object : gObjects) {
        Rectangle textureArea =
            {object.uvOrigin.x, object.uvOrigin.y, object.uvSize.x, object.uvSize.y};

        Rectangle destinationArea =
            {object.position.x, object.position.y, object.size.x, object.size.y};

        Vector2 objectOrigin = {object.size.x / 2.0f, object.size.y / 2.0f};

        DrawTexturePro(
            object.texture,
            textureArea,
            destinationArea,
            objectOrigin,
            object.rotation,
            object.tint
        );
    }

    EndDrawing();
}

void shutdown(void) {
    for (const auto& object : gObjects) {
        UnloadTexture(object.texture);
    }
    gObjects.clear();

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