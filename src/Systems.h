#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "ECS/ECS.h"
#include "Components.h"
#include "Logger/Logger.h"
#include "AssetStore/AssetStore.h"
#include "Events/EventBus.h"
#include "Events/Events.h"
#include "Game/Game.h"

#include <string>
#include <algorithm>
#include <SDL2/SDL.h>
#include <imgui/imgui.h>
#include <imgui/imgui_sdl.h>
#include <glm/glm.hpp>

class MovementSystem : public System {
    public:
        MovementSystem() {
            RequireComponent<TransformComponent>();
            RequireComponent<RigidBodyComponent>();
        }

        void SubscribeToEvents(std::unique_ptr<EventBus> &eventBus) {
            eventBus->SubscribeToEvent<MovementSystem, CollisionEvent>(this, &MovementSystem::onCollision);
        }

        void Update(double deltaTime) {
            for (auto entity : GetSystemEntities()) {
                auto &transform = entity.GetComponent<TransformComponent>();
                const auto &rigidbody = entity.GetComponent<RigidBodyComponent>();

                transform.position.x += rigidbody.velocity.x * deltaTime;
                transform.position.y += rigidbody.velocity.y * deltaTime;

                if (entity.HasTag("player")) {
                    int paddingLeft = 10;
                    int paddingTop = 10;
                    int paddingRight = 50;
                    int paddingBottom = 50;
                    transform.position.x = transform.position.x < paddingLeft ? paddingLeft : transform.position.x;
                    transform.position.x = transform.position.x > Game::mapWidth - paddingRight ? Game::mapWidth - paddingRight : transform.position.x;
                    transform.position.y = transform.position.y < paddingTop ? paddingTop : transform.position.y;
                    transform.position.y = transform.position.y > Game::mapHeight - paddingBottom ? Game::mapHeight - paddingBottom : transform.position.y;
                }

                bool isEntityOutsideMap = (
                    transform.position.x < -100 ||
                    transform.position.x > Game::mapWidth + 100 ||
                    transform.position.y < -100 ||
                    transform.position.y > Game::mapHeight + 100
                );

                if (isEntityOutsideMap && !entity.HasTag("player")) {
                    entity.Destroy();
                }
            }
        }

        void onCollision(CollisionEvent &event) {
            Entity a = event.a;
            Entity b = event.b;

            if (a.BelongsToGroup("enemies") && b.BelongsToGroup("obstacles")) {
                onEnemyHitsObstacle(a, b);
            }
            if (a.BelongsToGroup("obstacles") && b.BelongsToGroup("enemies")) {
                onEnemyHitsObstacle(b, a);
            }
        }

        void onEnemyHitsObstacle(Entity enemy, Entity obstacle) {
            (void)obstacle;
            if (enemy.HasComponent<RigidBodyComponent>() && enemy.HasComponent<SpriteComponent>()) {
                auto &rigidbody = enemy.GetComponent<RigidBodyComponent>();
                auto &sprite = enemy.GetComponent<SpriteComponent>();

                if (rigidbody.velocity.x != 0) {
                    rigidbody.velocity.x *= -1;
                    sprite.flip = (sprite.flip == SDL_FLIP_NONE ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
                }
                if (rigidbody.velocity.y != 0) {
                    rigidbody.velocity.y *= -1;
                    sprite.flip = (sprite.flip == SDL_FLIP_NONE ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);
                }
            }
        }
};

class RenderSystem : public System {
    public:
        RenderSystem() {
            RequireComponent<TransformComponent>();
            RequireComponent<SpriteComponent>();
        }

        void Update(SDL_Renderer *renderer, const SDL_Rect &camera, std::unique_ptr<AssetStore> &assetStore) {
            auto entities = GetSystemEntities();

            entities.erase(
                std::remove_if(
                    entities.begin(),
                    entities.end(),
                    [&camera](Entity entity) {
                        const auto &transform = entity.GetComponent<TransformComponent>();
                        const auto &sprite = entity.GetComponent<SpriteComponent>();
                        return (
                            (
                                transform.position.x + (sprite.width * transform.scale.x) < camera.x ||
                                transform.position.x > camera.x + camera.w ||
                                transform.position.y + (sprite.height * transform.scale.y) < camera.y ||
                                transform.position.y > camera.y + camera.h
                            )
                            && !sprite.isFixed
                        );
                    }
                ),
                entities.end()
            );

            std::sort(entities.begin(), entities.end(), [](const Entity &a, const Entity &b) {
                return a.GetComponent<SpriteComponent>().zIndex < b.GetComponent<SpriteComponent>().zIndex;
            });

            for (auto entity : entities) {
                const auto transform = entity.GetComponent<TransformComponent>();
                const auto sprite = entity.GetComponent<SpriteComponent>();

                SDL_Texture *texture = assetStore->GetTexture(sprite.assetId);
                SDL_Rect srcRect = sprite.srcRect;

                SDL_Rect dstRect = {
                    static_cast<int>(transform.position.x - (sprite.isFixed ? 0 : camera.x)),
                    static_cast<int>(transform.position.y - (sprite.isFixed ? 0 : camera.y)),
                    static_cast<int>(sprite.width * transform.scale.x),
                    static_cast<int>(sprite.height * transform.scale.y)
                };

                SDL_RenderCopyEx(
                    renderer,
                    texture,
                    &srcRect,
                    &dstRect,
                    transform.rotation,
                    NULL,
                    sprite.flip
                );
            }
        }
};

class AnimationSystem : public System {
    public:
        AnimationSystem() {
            RequireComponent<SpriteComponent>();
            RequireComponent<AnimationComponent>();
        }

        void Update() {
            for (auto entity : GetSystemEntities()) {
                auto &animation = entity.GetComponent<AnimationComponent>();
                auto &sprite = entity.GetComponent<SpriteComponent>();

                animation.currentFrame = (
                    (SDL_GetTicks() - animation.startTime) * animation.frameSpeedRate / 1000
                ) % animation.numFrames;

                sprite.srcRect.x = animation.currentFrame * sprite.width;
            }
        }
};

class CollisionSystem : public System {
    public:
        CollisionSystem() {
            RequireComponent<TransformComponent>();
            RequireComponent<BoxColliderComponent>();
        }

        void Update(std::unique_ptr<EventBus> &eventBus) {
            const auto entities = GetSystemEntities();
            for (size_t i = 0; i < entities.size(); i++) {
                Entity a = entities[i];
                const auto aTransform = a.GetComponent<TransformComponent>();
                const auto aCollider = a.GetComponent<BoxColliderComponent>();

                for (size_t j = i + 1; j < entities.size(); j++) {
                    Entity b = entities[j];
                    const auto bTransform = b.GetComponent<TransformComponent>();
                    const auto bCollider = b.GetComponent<BoxColliderComponent>();

                    bool collisionHappened = CheckAABBCollision(
                        aTransform.position.x + aCollider.offset.x * aTransform.scale.x,
                        aTransform.position.y + aCollider.offset.y * aTransform.scale.y,
                        aCollider.width * aTransform.scale.x,
                        aCollider.height * aTransform.scale.y,
                        bTransform.position.x + bCollider.offset.x * bTransform.scale.x,
                        bTransform.position.y + bCollider.offset.y * bTransform.scale.y,
                        bCollider.width * bTransform.scale.x,
                        bCollider.height * bTransform.scale.y
                    );

                    if (collisionHappened) {
                        eventBus->EmitEvent<CollisionEvent>(a, b);
                    }
                }
            }
        }

        static bool CheckAABBCollision(double aX, double aY, double aW, double aH, double bX, double bY, double bW, double bH) {
            return aX < bX + bW && aX + aW > bX && aY < bY + bH && aY + aH > bY;
        }
};

class RenderCollisionSystem : public System {
    public:
        RenderCollisionSystem() {
            RequireComponent<TransformComponent>();
            RequireComponent<BoxColliderComponent>();
        }

        void Update(SDL_Renderer *renderer, const SDL_Rect &camera) {
            for (auto entity : GetSystemEntities()) {
                const auto transform = entity.GetComponent<TransformComponent>();
                const auto collider = entity.GetComponent<BoxColliderComponent>();

                SDL_Rect rect = {
                    static_cast<int>(transform.position.x + collider.offset.x - camera.x),
                    static_cast<int>(transform.position.y + collider.offset.y - camera.y),
                    static_cast<int>(collider.width * transform.scale.x),
                    static_cast<int>(collider.height * transform.scale.y)
                };

                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &rect);
            }
        }
};

class DamageSystem : public System {
    public:
        DamageSystem() {
            RequireComponent<BoxColliderComponent>();
        }

        void onCollision(CollisionEvent &event) {
            Entity a = event.a;
            Entity b = event.b;

            if (a.BelongsToGroup("projectiles") && b.HasTag("player")) {
                onProjectileHitsPlayer(a, b);
            }
            if (b.BelongsToGroup("projectiles") && a.HasTag("player")) {
                onProjectileHitsPlayer(b, a);
            }
            if (a.BelongsToGroup("projectiles") && b.BelongsToGroup("enemies")) {
                onProjectileHitsEnemy(a, b);
            }
            if (b.BelongsToGroup("projectiles") && a.BelongsToGroup("enemies")) {
                onProjectileHitsEnemy(b, a);
            }
        }

        void onProjectileHitsPlayer(Entity projectile, Entity player) {
            auto projectileComponent = projectile.GetComponent<ProjectileComponent>();
            if (!projectileComponent.isFriendly) {
                auto &health = player.GetComponent<HealthComponent>();
                health.healthPercentage -= projectileComponent.hitPercentDamage;
                if (health.healthPercentage <= 0) {
                    player.Destroy();
                }
                projectile.Destroy();
            }
        }

        void onProjectileHitsEnemy(Entity projectile, Entity enemy) {
            auto projectileComponent = projectile.GetComponent<ProjectileComponent>();
            if (projectileComponent.isFriendly) {
                auto &health = enemy.GetComponent<HealthComponent>();
                health.healthPercentage -= projectileComponent.hitPercentDamage;
                if (health.healthPercentage <= 0) {
                    enemy.Destroy();
                }
                projectile.Destroy();
            }
        }

        void SubscribeToEvents(std::unique_ptr<EventBus> &eventBus) {
            eventBus->SubscribeToEvent<DamageSystem, CollisionEvent>(this, &DamageSystem::onCollision);
        }

        void Update() {}
};

class KeyboardMovementSystem : public System {
    public:
        KeyboardMovementSystem() {
            RequireComponent<KeyboardControlledComponent>();
            RequireComponent<SpriteComponent>();
            RequireComponent<RigidBodyComponent>();
        }

        void onKeyPress(KeyPressedEvent &event) {
            for (auto entity : GetSystemEntities()) {
                const auto keyboardcontrol = entity.GetComponent<KeyboardControlledComponent>();
                auto &sprite = entity.GetComponent<SpriteComponent>();
                auto &rigidbody = entity.GetComponent<RigidBodyComponent>();

                switch (event.symbol) {
                    case SDLK_UP:
                        rigidbody.velocity = keyboardcontrol.upVelocity;
                        sprite.srcRect.y = sprite.height * 0;
                        break;
                    case SDLK_RIGHT:
                        rigidbody.velocity = keyboardcontrol.rightVelocity;
                        sprite.srcRect.y = sprite.height * 1;
                        break;
                    case SDLK_DOWN:
                        rigidbody.velocity = keyboardcontrol.downVelocity;
                        sprite.srcRect.y = sprite.height * 2;
                        break;
                    case SDLK_LEFT:
                        rigidbody.velocity = keyboardcontrol.leftVelocity;
                        sprite.srcRect.y = sprite.height * 3;
                        break;
                    case SDLK_z:
                        rigidbody.velocity = glm::vec2(0);
                        break;
                    default:
                        break;
                }
            }
        }

        void SubscribeToEvents(std::unique_ptr<EventBus> &eventBus) {
            eventBus->SubscribeToEvent<KeyboardMovementSystem, KeyPressedEvent>(this, &KeyboardMovementSystem::onKeyPress);
        }

        void Update() {}
};

class CameraMovementSystem : public System {
    public:
        CameraMovementSystem() {
            RequireComponent<CameraFollowComponent>();
            RequireComponent<TransformComponent>();
        }

        void Update(SDL_Rect &camera) {
            for (auto entity : GetSystemEntities()) {
                auto transform = entity.GetComponent<TransformComponent>();

                camera.x = static_cast<int>(transform.position.x - camera.w / 2);
                camera.y = static_cast<int>(transform.position.y - camera.h / 2);

                camera.x = std::max(0, std::min(camera.x, Game::mapWidth - camera.w));
                camera.y = std::max(0, std::min(camera.y, Game::mapHeight - camera.h));
            }
        }
};

class ProjectileEmitSystem : public System {
    public:
        ProjectileEmitSystem() {
            RequireComponent<ProjectileEmitterComponent>();
            RequireComponent<TransformComponent>();
        }

        void onKeyPress(KeyPressedEvent &event) {
            if (event.symbol == SDLK_SPACE) {
                for (auto entity : GetSystemEntities()) {
                    if (entity.HasComponent<CameraFollowComponent>()) {
                        const auto projectileEmitter = entity.GetComponent<ProjectileEmitterComponent>();
                        const auto transform = entity.GetComponent<TransformComponent>();
                        const auto rigidbody = entity.GetComponent<RigidBodyComponent>();

                        glm::vec2 projectilePosition = transform.position;
                        if (entity.HasComponent<SpriteComponent>()) {
                            auto sprite = entity.GetComponent<SpriteComponent>();
                            projectilePosition.x += (transform.scale.x * sprite.width / 2);
                            projectilePosition.y += (transform.scale.y * sprite.height / 2);
                        }

                        glm::vec2 projectileVelocity = projectileEmitter.projectileVelocity;
                        int directionX = 0;
                        int directionY = 0;

                        if (rigidbody.velocity.x > 0) directionX = 1;
                        if (rigidbody.velocity.x < 0) directionX = -1;
                        if (rigidbody.velocity.y > 0) directionY = 1;
                        if (rigidbody.velocity.y < 0) directionY = -1;

                        projectileVelocity.x = projectileEmitter.projectileVelocity.x * directionX;
                        projectileVelocity.y = projectileEmitter.projectileVelocity.y * directionY;

                        Entity projectile = entity.world->CreateEntity();
                        projectile.Group("projectiles");
                        projectile.AddComponent<TransformComponent>(projectilePosition, glm::vec2(1.0, 1.0), 0.0);
                        projectile.AddComponent<RigidBodyComponent>(projectileVelocity);
                        projectile.AddComponent<SpriteComponent>("bullet-image", 4, 4, 4);
                        projectile.AddComponent<BoxColliderComponent>(4, 4);
                        projectile.AddComponent<ProjectileComponent>(projectileEmitter.isFriendly, projectileEmitter.hitPercentDamage, projectileEmitter.projectileDuration);
                    }
                }
            }
        }

        void SubscribeToEvents(std::unique_ptr<EventBus> &eventBus) {
            eventBus->SubscribeToEvent<ProjectileEmitSystem, KeyPressedEvent>(this, &ProjectileEmitSystem::onKeyPress);
        }

        void Update(std::unique_ptr<World> &world) {
            for (auto entity : GetSystemEntities()) {
                auto &projectileEmitter = entity.GetComponent<ProjectileEmitterComponent>();
                const auto transform = entity.GetComponent<TransformComponent>();

                if (projectileEmitter.repeatFrequency == 0) {
                    continue;
                }

                if (SDL_GetTicks() - projectileEmitter.lastEmissionTime > static_cast<unsigned int>(projectileEmitter.repeatFrequency)) {
                    glm::vec2 projectilePosition = transform.position;
                    if (entity.HasComponent<SpriteComponent>()) {
                        const auto sprite = entity.GetComponent<SpriteComponent>();
                        projectilePosition.x += (transform.scale.x * sprite.width / 2);
                        projectilePosition.y += (transform.scale.y * sprite.height / 2);
                    }

                    Entity projectile = world->CreateEntity();
                    projectile.Group("projectiles");
                    projectile.AddComponent<TransformComponent>(projectilePosition, glm::vec2(1.0, 1.0), 0.0);
                    projectile.AddComponent<RigidBodyComponent>(projectileEmitter.projectileVelocity);
                    projectile.AddComponent<SpriteComponent>("bullet-image", 4, 4, 4);
                    projectile.AddComponent<BoxColliderComponent>(4, 4);
                    projectile.AddComponent<ProjectileComponent>(projectileEmitter.isFriendly, projectileEmitter.hitPercentDamage, projectileEmitter.projectileDuration);

                    projectileEmitter.lastEmissionTime = SDL_GetTicks();
                }
            }
        }
};

class ProjectileLifecycleSystem : public System {
    public:
        ProjectileLifecycleSystem() {
            RequireComponent<ProjectileComponent>();
        }

        void Update() {
            for (auto entity : GetSystemEntities()) {
                auto projectile = entity.GetComponent<ProjectileComponent>();
                if (SDL_GetTicks() - projectile.startTime > static_cast<unsigned int>(projectile.duration)) {
                    entity.Destroy();
                }
            }
        }
};

class RenderTextSystem : public System {
    public:
        RenderTextSystem() {
            RequireComponent<TextLabelComponent>();
        }

        void Update(SDL_Renderer *renderer, SDL_Rect camera, std::unique_ptr<AssetStore> &assetStore) {
            for (auto entity : GetSystemEntities()) {
                const auto textlabel = entity.GetComponent<TextLabelComponent>();

                SDL_Surface *surface = TTF_RenderText_Blended(
                    assetStore->GetFont(textlabel.assetId),
                    textlabel.text.c_str(),
                    textlabel.color
                );
                if (!surface) continue;

                SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
                if (!texture) continue;

                int labelWidth = 0;
                int labelHeight = 0;
                SDL_QueryTexture(texture, NULL, NULL, &labelWidth, &labelHeight);

                SDL_Rect dstRect = {
                    static_cast<int>(textlabel.position.x - (textlabel.isFixed ? 0 : camera.x)),
                    static_cast<int>(textlabel.position.y - (textlabel.isFixed ? 0 : camera.y)),
                    labelWidth,
                    labelHeight
                };

                SDL_RenderCopy(renderer, texture, NULL, &dstRect);
                SDL_DestroyTexture(texture);
            }
        }
};

class RenderHealthSystem : public System {
    public:
        RenderHealthSystem() {
            RequireComponent<HealthComponent>();
            RequireComponent<TransformComponent>();
            RequireComponent<HealthBarComponent>();
            RequireComponent<SpriteComponent>();
        }

        void Update(SDL_Renderer *renderer, SDL_Rect camera, std::unique_ptr<AssetStore> &assetStore) {
            for (auto entity : GetSystemEntities()) {
                const auto health = entity.GetComponent<HealthComponent>();
                const auto transform = entity.GetComponent<TransformComponent>();
                const auto healthbar = entity.GetComponent<HealthBarComponent>();
                const auto sprite = entity.GetComponent<SpriteComponent>();

                SDL_Color healthColor;
                if (health.healthPercentage >= 80) {
                    healthColor = {0, 255, 0, 255};
                } else if (health.healthPercentage >= 40) {
                    healthColor = {255, 255, 0, 255};
                } else {
                    healthColor = {255, 0, 0, 255};
                }

                SDL_Surface *surface = TTF_RenderText_Blended(
                    assetStore->GetFont(healthbar.assetId),
                    (std::to_string(health.healthPercentage) + "%").c_str(),
                    healthColor
                );
                if (!surface) continue;
                SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
                if (!texture) continue;

                int healthWidth, healthHeight;
                SDL_QueryTexture(texture, NULL, NULL, &healthWidth, &healthHeight);

                int x = static_cast<int>(transform.position.x + sprite.width * transform.scale.x - camera.x);
                int y = static_cast<int>(transform.position.y - 10 - camera.y);

                SDL_Rect dstRect = { x, y, healthWidth, healthHeight };
                SDL_RenderCopy(renderer, texture, NULL, &dstRect);

                SDL_Rect healthBar = {
                    x,
                    y + healthHeight,
                    static_cast<int>(healthbar.barWidth * (health.healthPercentage / 100.0)),
                    healthbar.barHeight
                };

                SDL_SetRenderDrawColor(renderer, healthColor.r, healthColor.g, healthColor.b, 255);
                SDL_RenderFillRect(renderer, &healthBar);

                SDL_DestroyTexture(texture);
            }
        }
};

class RenderGUISystem : public System {
    public:
        RenderGUISystem() = default;

        void Update(const std::unique_ptr<World> &world, const SDL_Rect &camera) {
            static int posX = 0;
            static int posY = 0;
            static int scaleX = 1;
            static int scaleY = 1;
            static float rotation = 0.0f;
            static int velX = 0;
            static int velY = 0;
            static int health = 100;
            static float projAngle = 0.0f;
            static float projSpeed = 100.0f;
            static int projRepeat = 10;
            static int projDuration = 10;
            static int projHitPercent = 10;
            const char* sprites[] = {"tank-image", "truck-image"};
            static int selectedSpriteIndex = 0;

            ImGui::NewFrame();

            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav;
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always, ImVec2(0, 0));
            ImGui::SetNextWindowBgAlpha(0.9f);
            if (ImGui::Begin("Map Coordinates", NULL, windowFlags)) {
                ImGui::Text(
                    "Map coordinates (x=%.1f, y=%.1f)",
                    ImGui::GetIO().MousePos.x + camera.x,
                    ImGui::GetIO().MousePos.y + camera.y
                );
            }
            ImGui::End();

            if (ImGui::IsMouseDoubleClicked(0)) {
                posX = static_cast<int>(ImGui::GetMousePos().x + camera.x);
                posY = static_cast<int>(ImGui::GetMousePos().y + camera.y);
            }

            if (ImGui::Begin("Spawn enemies", NULL)) {
                if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Combo("texture id", &selectedSpriteIndex, sprites, IM_ARRAYSIZE(sprites));
                }
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::InputInt("position x", &posX);
                    ImGui::InputInt("position y", &posY);
                    ImGui::SliderInt("scale x", &scaleX, 1, 10);
                    ImGui::SliderInt("scale y", &scaleY, 1, 10);
                    ImGui::SliderAngle("rotation (deg)", &rotation, 0, 360);
                }
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Rigid Body", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::InputInt("velocity x", &velX);
                    ImGui::InputInt("velocity y", &velY);
                }
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Projectile emitter", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::SliderAngle("angle (deg)", &projAngle, 0, 360);
                    ImGui::SliderFloat("speed (px/sec)", &projSpeed, 10, 500);
                    ImGui::InputInt("repeat (sec)", &projRepeat);
                    ImGui::InputInt("duration (sec)", &projDuration);
                    ImGui::SliderInt("hit percent", &projHitPercent, 1, 100);
                }
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Health", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::SliderInt("%", &health, 0, 100);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("Spawn Enemy")) {
                    auto enemy = world->CreateEntity();
                    enemy.Group("enemies");
                    enemy.AddComponent<TransformComponent>(
                        glm::vec2(static_cast<float>(posX), static_cast<float>(posY)),
                        glm::vec2(static_cast<float>(scaleX), static_cast<float>(scaleY)),
                        glm::degrees(rotation)
                    );
                    enemy.AddComponent<RigidBodyComponent>(glm::vec2(static_cast<float>(velX), static_cast<float>(velY)));
                    enemy.AddComponent<SpriteComponent>(sprites[selectedSpriteIndex], 32, 32, 2, false);
                    enemy.AddComponent<BoxColliderComponent>(32, 32, glm::vec2(0));
                    glm::vec2 projVel(cos(projAngle) * projSpeed, sin(projAngle) * projSpeed);
                    enemy.AddComponent<ProjectileEmitterComponent>(projVel, projRepeat * 1000, projDuration * 1000, projHitPercent, false);
                    enemy.AddComponent<HealthComponent>(health);
                    enemy.AddComponent<HealthBarComponent>("health-font", 30, 5, true);

                    posX = posY = 0;
                    rotation = projAngle = 0.0f;
                    scaleX = scaleY = 1;
                    projRepeat = projDuration = 10;
                    projSpeed = 100.0f;
                    health = 10;
                    projHitPercent = 10;
                }
            }
            ImGui::End();

            ImGui::Render();
            ImGuiSDL::Render(ImGui::GetDrawData());
        }
};

#endif
