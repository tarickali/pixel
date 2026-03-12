#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <string>
#include <glm/glm.hpp>
#include <SDL2/SDL.h>

struct TransformComponent {
    glm::vec2 position;
    glm::vec2 scale;
    double rotation;

    TransformComponent(glm::vec2 position = glm::vec2(0, 0), glm::vec2 scale = glm::vec2(1, 1), double rotation = 0.0) {
        this->position = position;
        this->scale = scale;
        this->rotation = rotation;
    }
};

struct RigidBodyComponent {
    glm::vec2 velocity;

    RigidBodyComponent(glm::vec2 velocity = glm::vec2(0.0, 0.0)) {
        this->velocity = velocity;
    }
};

struct SpriteComponent {
    std::string assetId;
    int width;
    int height;
    int zIndex;
    bool isFixed;
    SDL_Rect srcRect;
    SDL_RendererFlip flip;

    SpriteComponent(std::string assetId = "", int width = 0, int height = 0, int zIndex = 0, bool isFixed = false, int srcRectX = 0, int srcRectY = 0) {
        this->assetId = assetId;
        this->width = width;
        this->height = height;
        this->zIndex = zIndex;
        this->isFixed = isFixed;
        this->srcRect = {srcRectX, srcRectY, this->width, this->height};
        this->flip = SDL_FLIP_NONE;
    }
};

struct AnimationComponent {
    int numFrames;
    int frameSpeedRate;
    bool isLoop;
    int currentFrame;
    int startTime;

    AnimationComponent(int numFrames = 1, int frameSpeedRate = 1, bool isLoop = true) {
        this->numFrames = numFrames;
        this->frameSpeedRate = frameSpeedRate;
        this->isLoop = isLoop;
        this->currentFrame = 1;
        this->startTime = SDL_GetTicks();
    }
};

struct BoxColliderComponent {
    int width;
    int height;
    glm::vec2 offset;

    BoxColliderComponent(int width = 0, int height = 0, glm::vec2 offset = glm::vec2(0)) {
        this->width = width;
        this->height = height;
        this->offset = offset;
    }
};

struct KeyboardControlledComponent {
    glm::vec2 upVelocity;
    glm::vec2 rightVelocity;
    glm::vec2 downVelocity;
    glm::vec2 leftVelocity;

    KeyboardControlledComponent(
        glm::vec2 upVelocity = glm::vec2(0),
        glm::vec2 rightVelocity = glm::vec2(0),
        glm::vec2 downVelocity = glm::vec2(0),
        glm::vec2 leftVelocity = glm::vec2(0)
    ) {
        this->upVelocity = upVelocity;
        this->rightVelocity = rightVelocity;
        this->downVelocity = downVelocity;
        this->leftVelocity = leftVelocity;
    }
};

struct CameraFollowComponent {
    CameraFollowComponent() = default;
};

struct ProjectileEmitterComponent {
    glm::vec2 projectileVelocity;
    int repeatFrequency;
    int projectileDuration;
    int hitPercentDamage;
    bool isFriendly;
    int lastEmissionTime;

    ProjectileEmitterComponent(glm::vec2 projectileVelocity = glm::vec2(0), int repeatFrequency = 0, int projectileDuration = 10000, int hitPercentDamage = 10, bool isFriendly = false) {
        this->projectileVelocity = projectileVelocity;
        this->repeatFrequency = repeatFrequency;
        this->projectileDuration = projectileDuration;
        this->hitPercentDamage = hitPercentDamage;
        this->isFriendly = isFriendly;
        this->lastEmissionTime = SDL_GetTicks();
    }
};

struct HealthComponent {
    int healthPercentage;

    HealthComponent(int healthPercentage = 0) {
        this->healthPercentage = healthPercentage;
    }
};

struct ProjectileComponent {
    bool isFriendly;
    int hitPercentDamage;
    int duration;
    int startTime;

    ProjectileComponent(bool isFriendly = false, int hitPercentDamage = 0, int duration = 0) {
        this->isFriendly = isFriendly;
        this->hitPercentDamage = hitPercentDamage;
        this->duration = duration;
        this->startTime = SDL_GetTicks();
    }
};

struct TextLabelComponent {
    glm::vec2 position;
    std::string text;
    std::string assetId;
    SDL_Color color;
    bool isFixed;

    TextLabelComponent(
        glm::vec2 position = glm::vec2(0),
        std::string text = "",
        std::string assetId = "",
        SDL_Color color = {0, 0, 0, 255},
        bool isFixed = true
    ) {
        this->position = position;
        this->text = text;
        this->assetId = assetId;
        this->color = color;
        this->isFixed = isFixed;
    }
};

struct HealthBarComponent {
    std::string assetId;
    int barWidth;
    int barHeight;
    bool isVisible;

    HealthBarComponent(std::string assetId = "", int barWidth = 0, int barHeight = 0, bool isVisible = true) {
        this->assetId = assetId;
        this->barWidth = barWidth;
        this->barHeight = barHeight;
        this->isVisible = isVisible;
    }
};

#endif
