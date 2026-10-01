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
#include <cmath>
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

// Enums are enclosed in a namespace so to avoid scope conflicts with "COUNT" members
namespace SceneObject {

// There can be only one of each scene object, and they're loaded in the order of declaration here.
enum Type { TREES_BG, TREES_MG, TREES_FG, ROAD, SUN, CAR, UFO, LASER, EXPLOSION, COUNT };
} // namespace SceneObject

constexpr std::array<const char*, SceneObject::COUNT> TEXTURE_PATHS = {
    "assets/trees-background.png",
    "assets/trees-midground.png",
    "assets/trees-foreground.png",
    "assets/road.png",
    "assets/sun.png",
    "assets/car.png",
    "assets/ufo.png",
    "assets/laser.png",
    "assets/explosion.png"
};

namespace Event {
enum Type { DAY, NIGHT, UFO_ENTRANCE, UFO_EXIT, COUNT };
}

constexpr float EVENT_TIMESTAMPS[] = {0.0f, 3.0f, 4.0f, 10.0f};
constexpr float EVENT_CYCLE_LENGTH = 12.0f;

// =============================================================================
// Object-specific constants
// =============================================================================

// Parallax backgrounds
constexpr float BACKGROUND_SCROLL_SPEEDS[] = {15.0f, 25.0f, 45.0f, 90.0f};
constexpr float BACKGROUND_Y_OFFSET = 100.0f;

constexpr float NIGHT_BACKGROUND_SCROLL_SPEED_MULTIPLIER = 3.0f;

// Road
constexpr float ROAD_Y_OFFSET = 150.0f; // Added to the above background offset
constexpr float ROAD_SIZE_Y = 250.0f;

// Sun
constexpr Vector2 SUN_SIZE = {250.0f, 250.0f};
constexpr float SUN_Y = 125.0f;
constexpr float SUN_HIDDEN_Y = 750.0f;

// =============================================================================
// App-level globals
// =============================================================================
AppStatus gAppStatus = AppStatus::RUNNING;
float gPreviousTimestampSec = 0.0f;

struct CurrentEvent {
    Event::Type id;
    float elapsedTime;
    float progress;
};

CurrentEvent gCurrentEvent = {Event::DAY, 0.0f, 0.0f};

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

// =============================================================================
// Object-specific globals
// =============================================================================

// Parallax background UV offsets
float gBackgroundOffsets[] = {0.0f, 0.0f, 0.0f, 0.0f};
float gBackgroundScrollSpeedMultiplier = 1.0f;

void initialize(void);
void processInput(void);
void updateCurrentEvent(float nowSec);
void update(void);
void render(void);
void shutdown(void);

void initialize(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "An Awesome Scene");
    SetTargetFPS(TARGET_FPS);

    // Load textures for all scene objects
    for (const char* path : TEXTURE_PATHS) {
        Object object = {0};
        object.texture = LoadTexture(path);
        object.position = SCREEN_CENTER;
        object.tint = WHITE;

        object.uvSize = {
            static_cast<float>(object.texture.width),
            static_cast<float>(object.texture.height)
        };
        object.size = object.uvSize;

        gObjects.push_back(object);
    }

    // Initialize scene objects

    // Parallax backgrounds take up the entire screen. Also shift them down a bit.
    for (int32_t i = SceneObject::TREES_BG; i <= SceneObject::ROAD; ++i) {
        gObjects[i].size = SCREEN_SIZE;
        gObjects[i].position.y += BACKGROUND_Y_OFFSET;
    }

    auto& road = gObjects[SceneObject::ROAD];
    road.position.y += ROAD_Y_OFFSET;
    road.size.x = SCREEN_SIZE.x;
    road.size.y = ROAD_SIZE_Y;

    auto& sun = gObjects[SceneObject::SUN];
    sun.size = SUN_SIZE;
    sun.position.y = SUN_Y;

    auto& car = gObjects[SceneObject::CAR];
    car.size = {2.0f * car.size.x, 2.0f * car.size.y};
    car.position.y = road.position.y - road.size.y / 4.0f;

    // These objects start off the screen
    auto& ufo = gObjects[SceneObject::UFO];
    ufo.size = {0.5f * ufo.size.x, 0.5f * ufo.size.y};
    ufo.position.x = -SCREEN_SIZE.x;
    ufo.rotation = -15.0f;

    auto& laser = gObjects[SceneObject::LASER];
    laser.size = {0.25f * laser.size.x, 0.25f * laser.size.y};
    laser.position.x = -SCREEN_SIZE.x;

    auto& explosion = gObjects[SceneObject::EXPLOSION];
    explosion.size = {0.5f * explosion.size.x, 0.5f * explosion.size.y};
    explosion.position.x = -SCREEN_SIZE.x;
}

void processInput(void) {
    if (WindowShouldClose()) {
        gAppStatus = TERMINATED;
    }
}

void updateCurrentEvent(float nowSec) {
    float eventTime = std::fmod(nowSec, EVENT_CYCLE_LENGTH);

    for (int32_t i = Event::COUNT - 1; i >= 0; --i) {
        if (eventTime >= EVENT_TIMESTAMPS[i]) {
            gCurrentEvent.id = static_cast<Event::Type>(i);
            gCurrentEvent.elapsedTime = eventTime - EVENT_TIMESTAMPS[i];
            gCurrentEvent.progress =
                gCurrentEvent.elapsedTime / (EVENT_TIMESTAMPS[i + 1] - EVENT_TIMESTAMPS[i]);
            return;
        }
    }
}

void update(void) {
    // Compute delta time
    float nowSec = static_cast<float>(GetTime());
    float deltaTime = nowSec - gPreviousTimestampSec;
    gPreviousTimestampSec = nowSec;

    // Parallax scroll
    for (int32_t i = SceneObject::TREES_BG; i <= SceneObject::ROAD; ++i) {
        gBackgroundOffsets[i] +=
            BACKGROUND_SCROLL_SPEEDS[i] * gBackgroundScrollSpeedMultiplier * deltaTime;
        gObjects[i].uvOrigin.x = gBackgroundOffsets[i];
    }

    updateCurrentEvent(nowSec);

    auto& car = gObjects[SceneObject::CAR];
    auto& sun = gObjects[SceneObject::SUN];
    auto& ufo = gObjects[SceneObject::UFO];

    switch (gCurrentEvent.id) {
        case (Event::NIGHT): {
            sun.position.y = SUN_Y + (SUN_HIDDEN_Y - SUN_Y) * gCurrentEvent.progress;
            gBackgroundScrollSpeedMultiplier = 1.0f;

            break;
        }
        case (Event::UFO_ENTRANCE): {
            gBackgroundScrollSpeedMultiplier = NIGHT_BACKGROUND_SCROLL_SPEED_MULTIPLIER;
            sun.position.y = SUN_HIDDEN_Y;

            break;
        }
        case (Event::UFO_EXIT): {
            sun.position.y = SUN_HIDDEN_Y + (SUN_Y - SUN_HIDDEN_Y) * gCurrentEvent.progress;

            break;
        }
        case (Event::DAY): {
            gBackgroundScrollSpeedMultiplier = 1.0f;

            float sizeMultiplier = 1.0f + (0.25f - 1.0f) * gCurrentEvent.progress;
            sun.size = {sizeMultiplier * sun.size.x, sizeMultiplier * sun.size.y};

            break;
        }
        default:
            break;
    }

    car.position.x = SCREEN_CENTER.x + 10.0f * std::cos(nowSec * 2);
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