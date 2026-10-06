/**
* Author: Alan Chen
* Assignment: Simple 2D Scene
* Date due: 10/05/2026
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
enum Type { SUN, STARS, TREES_BG, TREES_MG, TREES_FG, ROAD, CAR, UFO, LASER, EXPLOSION, COUNT };
} // namespace SceneObject

constexpr std::array<const char*, SceneObject::COUNT> TEXTURE_PATHS = {
    "assets/sun.png",
    "assets/stars.png",
    "assets/trees-background.png",
    "assets/trees-midground.png",
    "assets/trees-foreground.png",
    "assets/road.png",
    "assets/car.png",
    "assets/ufo.png",
    "assets/laser.png",
    "assets/explosion.png"
};

namespace Event {
enum Type { DAY, NIGHT, UFO_ENTRANCE, UFO_EXIT, SUNRISE, COUNT };
}

constexpr float EVENT_CYCLE_LENGTH = 21.0f;
constexpr float EVENT_TIMESTAMPS[] = {0.0f, 5.0f, 8.0f, 14.0f, 16.0f, EVENT_CYCLE_LENGTH};

constexpr Color DAY_BG_COLOR = SKYBLUE;
constexpr Color SUNSET_BG_COLOR = ORANGE;
constexpr Color NIGHT_BG_COLOR = BLACK;

// Sun lingers until this timestamp, then sets
constexpr float SUNSET_BEGIN = 0.5f;

// Sky fades to black by this timestamp, then stars fade in
constexpr float NIGHT_FADE_END = 0.5f;

// UFO finishes flying to its fire position by this timestamp
constexpr float UFO_ARRIVE_END = 0.5f;
constexpr float LASER_FIRE_BEGIN = 0.75f; // Laser's fired here

// UFO finishes leaving by this timestamp
constexpr float UFO_LEAVE_END = 0.5f;

// Stars fade out
constexpr float SUNRISE_STARS_END = 0.3f;
constexpr float SUNRISE_GLOW_END = 0.6f; // Black -> orange
// Orange -> blue, sun rises

// =============================================================================
// Object-specific constants
// =============================================================================

// Parallax backgrounds
constexpr int32_t BACKGROUND_LAYER_COUNT = 5;
constexpr float BACKGROUND_SCROLL_SPEEDS[] = {7.5f, 15.0f, 25.0f, 45.0f, 90.0f};
constexpr float BACKGROUND_Y_OFFSET = 100.0f;

constexpr float NIGHT_BACKGROUND_SCROLL_SPEED_MULTIPLIER = 8.0f;

// Road
constexpr float ROAD_Y_OFFSET = 150.0f; // Added to the above background offset
constexpr float ROAD_SIZE_Y = 250.0f;

// Sun
constexpr Vector2 SUN_SIZE = {250.0f, 250.0f};
constexpr float SUN_Y = 125.0f;
constexpr float SUN_HIDDEN_Y = 400.0f;
constexpr float SUN_HIDDEN_SCALE = 0.25f;

// UFO
constexpr float UFO_ACTIVE_X = 350.0f;
constexpr float UFO_Y = 300.0f;
constexpr float UFO_BASE_ANGLE = -15.0f;

// Laser
constexpr float LASER_TRAVEL_SEC = 0.6f;
constexpr float LASER_TARGET_X = 650.0f;

// Explosion
constexpr Vector2 EXPLOSION_SIZE = {200.0f, 200.0f};
constexpr float EXPLOSION_MIN_SCALE = 0.8f;
constexpr float EXPLOSION_MAX_SCALE = 2.0f;
constexpr float EXPLOSION_LINGER_SEC = 0.85f;

constexpr int32_t STARS_MAX_ALPHA = 200;

// =============================================================================
// App-level globals
// =============================================================================
AppStatus gAppStatus = AppStatus::RUNNING;
float gPreviousTimestampSec = 0.0f;
Color gBackgroundColor = DAY_BG_COLOR;

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
float gBackgroundOffsets[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
float gBackgroundScrollSpeedMultiplier = 1.0f;

bool gLaserFiredThisCycle = false; // Once-per-cycle guard
bool gLaserInFlight = false; // Is the laser currently traveling to its target?
float gLaserStartSec = 0.0f; // For lerping
Vector2 gLaserTarget = {-1.0f, -1.0f};
Vector2 gLaserOrigin = {0.0f, 0.0f};
float gLaserAngle = 0.0f;

bool gExplosionStarted = false;
float gExplosionStartSec = 0.0f; // For lerping

void initialize(void);
void processInput(void);
void update(void);
void render(void);
void shutdown(void);

void initialize(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Close Encounters of the Close Kind");
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
    for (int32_t i = SceneObject::STARS; i <= SceneObject::ROAD; ++i) {
        gObjects[i].size = SCREEN_SIZE;
        gObjects[i].position.y += BACKGROUND_Y_OFFSET;
    }

    // Stars start off fully transparent
    auto& stars = gObjects[SceneObject::STARS];
    stars.tint.a = 0;
    stars.position.y = SCREEN_CENTER.y;
    stars.uvSize = SCREEN_SIZE;

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
    ufo.position = {-SCREEN_SIZE.x, UFO_Y};
    ufo.rotation = UFO_BASE_ANGLE;

    auto& laser = gObjects[SceneObject::LASER];
    laser.size = {0.1f * laser.size.x, 0.15f * laser.size.y};
    laser.position.x = -SCREEN_SIZE.x;

    auto& explosion = gObjects[SceneObject::EXPLOSION];
    explosion.size = EXPLOSION_SIZE;
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

// Lerps the size and position of the sun for a rising / setting effect
// hiddenAmount = 1 => completely hidden and smallest
// hiddenAmount = 0 => at its zenith
void setSunHiddenAmount(Object& sun, float hiddenAmount) {
    float scale = lerp(1.0f, SUN_HIDDEN_SCALE, hiddenAmount);
    sun.position.y = lerp(SUN_Y, SUN_HIDDEN_Y, hiddenAmount);
    sun.size = {SUN_SIZE.x * scale, SUN_SIZE.y * scale};
}

void update(void) {
    // Compute delta time
    float nowSec = static_cast<float>(GetTime());
    float deltaTime = nowSec - gPreviousTimestampSec;
    gPreviousTimestampSec = nowSec;

    // Parallax scroll
    for (int32_t i = SceneObject::STARS; i <= SceneObject::ROAD; ++i) {
        int32_t layer = i - SceneObject::STARS;

        gBackgroundOffsets[layer] +=
            BACKGROUND_SCROLL_SPEEDS[layer] * gBackgroundScrollSpeedMultiplier * deltaTime;
        gObjects[i].uvOrigin.x = gBackgroundOffsets[layer];
    }

    updateCurrentEvent(nowSec);

    auto& car = gObjects[SceneObject::CAR];
    auto& sun = gObjects[SceneObject::SUN];
    auto& ufo = gObjects[SceneObject::UFO];
    auto& stars = gObjects[SceneObject::STARS];

    const float p = gCurrentEvent.progress;

    switch (gCurrentEvent.id) {
        case (Event::DAY): {
            gLaserFiredThisCycle = false;
            gBackgroundScrollSpeedMultiplier = 1.0f;

            // Begin sunset sequence
            float setT = remap(p, SUNSET_BEGIN, 1.0f);
            setSunHiddenAmount(sun, setT);
            gBackgroundColor = lerpColor(DAY_BG_COLOR, SUNSET_BG_COLOR, setT);

            break;
        }
        case (Event::NIGHT): {
            float fadeT = remap(p, 0.0f, NIGHT_FADE_END);
            float starsT = remap(p, NIGHT_FADE_END, 1.0f);

            setSunHiddenAmount(sun, 1.0f);
            gBackgroundColor = lerpColor(SUNSET_BG_COLOR, NIGHT_BG_COLOR, fadeT);
            stars.tint.a = lerp(0, STARS_MAX_ALPHA, starsT);

            break;
        }
        case (Event::UFO_ENTRANCE): {
            float arriveT = remap(p, 0.0f, UFO_ARRIVE_END);

            // Ramp up scroll speed
            gBackgroundScrollSpeedMultiplier =
                lerp(1.0f, NIGHT_BACKGROUND_SCROLL_SPEED_MULTIPLIER, arriveT);

            // UFO moves to position
            ufo.position.x = lerp(-ufo.size.x, UFO_ACTIVE_X, arriveT);

            // Fire the laser
            if (p > LASER_FIRE_BEGIN && !gLaserFiredThisCycle) {
                gLaserFiredThisCycle = true;
                gLaserInFlight = true;
                gLaserStartSec = nowSec;
                gLaserTarget = {LASER_TARGET_X, car.position.y};
                gLaserOrigin = ufo.position;
                gLaserAngle =
                    std::atan2(gLaserTarget.y - gLaserOrigin.y, gLaserTarget.x - gLaserOrigin.x)
                    * 180.0f / PI;
            }

            setSunHiddenAmount(sun, 1.0f);
            gBackgroundColor = NIGHT_BG_COLOR;
            stars.tint.a = STARS_MAX_ALPHA;

            break;
        }
        case (Event::UFO_EXIT): {
            float leaveT = remap(p, 0.0f, UFO_LEAVE_END);

            // Scroll speed eases back down
            gBackgroundScrollSpeedMultiplier =
                lerp(NIGHT_BACKGROUND_SCROLL_SPEED_MULTIPLIER, 1.0f, p);

            // UFO leaves the scene
            ufo.position.x = lerp(UFO_ACTIVE_X, -ufo.size.x, leaveT);

            setSunHiddenAmount(sun, 1.0f);
            gBackgroundColor = NIGHT_BG_COLOR;
            stars.tint.a = static_cast<unsigned char>(STARS_MAX_ALPHA);

            break;
        }
        case (Event::SUNRISE): {
            float p = gCurrentEvent.progress;
            float starsT = remap(p, 0.0f, SUNRISE_STARS_END);
            float glowT = remap(p, SUNRISE_STARS_END, SUNRISE_GLOW_END);
            float riseT = remap(p, SUNRISE_GLOW_END, 1.0f);

            stars.tint.a = lerp(STARS_MAX_ALPHA, 0, starsT);

            gBackgroundColor = (p < SUNRISE_GLOW_END)
                ? lerpColor(NIGHT_BG_COLOR, SUNSET_BG_COLOR, glowT)
                : lerpColor(SUNSET_BG_COLOR, DAY_BG_COLOR, riseT);

            setSunHiddenAmount(sun, 1.0f - riseT);

            break;
        }
        default:
            break;
    }

    car.position.x = SCREEN_CENTER.x + 10.0f * std::cos(nowSec * 2);
    ufo.position.y = UFO_Y + 25.0f * std::cos(nowSec * 5);
    ufo.rotation = UFO_BASE_ANGLE + 5.0f * std::sin(nowSec * 2);

    auto& laser = gObjects[SceneObject::LASER];
    auto& explosion = gObjects[SceneObject::EXPLOSION];

    if (gLaserInFlight) {
        float t = clamp((nowSec - gLaserStartSec) / LASER_TRAVEL_SEC, 0.0f, 1.0f);
        laser.position = lerpVector2(gLaserOrigin, gLaserTarget, t);
        laser.rotation = gLaserAngle;

        if (t >= 1.0f) {
            gLaserInFlight = false;
            laser.position.x = -SCREEN_SIZE.x;

            gExplosionStarted = true;
            gExplosionStartSec = nowSec;
            explosion.position = gLaserTarget;
        }
    }

    if (gExplosionStarted) {
        auto& explosion = gObjects[SceneObject::EXPLOSION];

        float t = clamp((nowSec - gExplosionStartSec) / EXPLOSION_LINGER_SEC, 0.0f, 1.0f);

        // Shift the explosion off the screen to create effect of driving away
        explosion.position.x -= BACKGROUND_SCROLL_SPEEDS[SceneObject::ROAD - SceneObject::STARS]
            * gBackgroundScrollSpeedMultiplier * deltaTime;

        // Explosion scaling and fading
        float scale = lerp(EXPLOSION_MIN_SCALE, EXPLOSION_MAX_SCALE, t);
        explosion.size = {EXPLOSION_SIZE.x * scale, EXPLOSION_SIZE.y * scale};
        explosion.tint.a = lerp(255, 255 / 2, t);
        explosion.rotation = 180 / PI * std::cos(nowSec * 10);

        if (t >= 1.0f) {
            gExplosionStarted = false;
            explosion.position.x = -SCREEN_SIZE.x;
        }
    }
}

void render(void) {
    BeginDrawing();

    ClearBackground(gBackgroundColor);

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