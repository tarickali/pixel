#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <glm/glm.hpp>

struct TransformComponent {
    glm::vec2 position = glm::vec2(0);
    glm::vec2 scale = glm::vec2(1);
    double rotation = 0.0;

    TransformComponent(glm::vec2 position = glm::vec2(0), glm::vec2 scale = glm::vec2(1), double rotation = 0.0) {
        this->position = position;
        this->scale = scale;
        this->rotation = rotation;
    }
};

struct RigidBodyComponent {
    glm::vec2 velocity = glm::vec2(0);
    glm::vec2 acceleration = glm::vec2(0);
    double mass = 0.0;

    RigidBodyComponent(glm::vec2 velocity = glm::vec2(0), glm::vec2 acceleration = glm::vec2(0), double mass = 0.0) {
        this->velocity = velocity;
        this->acceleration = acceleration;
        this->mass = mass;
    }
};

#endif