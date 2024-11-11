#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "ECS.h"
#include "Components.h"

class PhysicsSystem : public System {
    public:
        double gravity;

        PhysicsSystem(double gravity = 9.81) {
            this->gravity = 9.81;

            requireComponent<TransformComponent>();
            requireComponent<RigidBodyComponent>();
        }

        void update(std::unique_ptr<Coordinator> &coordinator, double deltaTime) {
            for (auto entity : getSystemEntities()) {
                auto &transform = coordinator->getComponent<TransformComponent>(entity);
                const auto &rigidbody = coordinator->getComponent<RigidBodyComponent>(entity);

                transform.position.x += rigidbody.velocity.x * deltaTime;
                transform.position.y += rigidbody.velocity.y * deltaTime;

                spdlog::info("new position: " + std::to_string(transform.position.x) + " - " + std::to_string(transform.position.y));
            }
        }
};

#endif