#include "ECS.h"

#include <spdlog/spdlog.h>

////////////////////////////////////////////////////////////////////////////////
// Initialize static members
////////////////////////////////////////////////////////////////////////////////
ComponentId IComponent::nextId = 0;

////////////////////////////////////////////////////////////////////////////////
// System
////////////////////////////////////////////////////////////////////////////////
void System::addEntityToSystem(Entity entity) {
    entities.push_back(entity);
}

void System::removeEntityToSystem(Entity entity) {
    entities.erase(
        std::remove_if(
            entities.begin(),
            entities.end(),
            [&entity](Entity other) {
                return entity.getId() == other.getId();
            }
        ),
        entities.end()
    );
}

std::vector<Entity> System::getSystemEntities() const {
    return entities;
}

const ComponentSignature System::getComponentSignature() const {
    return componentSignature;
}

////////////////////////////////////////////////////////////////////////////////
// Coordinator
////////////////////////////////////////////////////////////////////////////////
Coordinator::Coordinator() {
    spdlog::info("Coordinator constructor called.");
}

Coordinator::~Coordinator() {
    spdlog::info("Coordinator destructor called.");
}

Entity Coordinator::create() {
    EntityId entityId;

    if (freeIds.empty()) {
        entityId = numEntites++;
        if (entityId >= entityComponentSignatures.size()) {
            int newSize = entityComponentSignatures.size() == 0 ? 2 : 2 * entityComponentSignatures.size();
            entityComponentSignatures.resize(newSize);
        }
    } else {
        entityId = freeIds.front();
        freeIds.pop_front();
    }

    Entity entity(entityId);
    entitiesToBeCreated.insert(entity);

    spdlog::info("Entity created with id = " + std::to_string(entityId));

    return entity;
}

void Coordinator::destroy(Entity entity) {
    entitiesToBeDestroyed.insert(entity);
}

void Coordinator::addEntityToSystems(Entity entity) {
    const auto entityId = entity.getId();

    spdlog::info("added entity to system " + std::to_string(entityId));
    if (entityId >= entityComponentSignatures.size()) {
        return;
    }

    const auto entityComponentSignature = entityComponentSignatures[entityId];

    for (auto &system : systems) {
        const auto systemComponentSignature = system.second->getComponentSignature();

        bool isInterested = (entityComponentSignature & systemComponentSignature) == systemComponentSignature;
        if (isInterested) {
            system.second->addEntityToSystem(entity);
        }
    }
}

void Coordinator::removeEntityFromSystems(Entity entity) {
    for (auto &system : systems) {
        system.second->removeEntityToSystem(entity);
    }
}

void Coordinator::update() {
    for (auto entity : entitiesToBeCreated) {
        addEntityToSystems(entity);
    }
    entitiesToBeCreated.clear();

    for (auto entity : entitiesToBeDestroyed) {
        // Remove the entity from all systems
        removeEntityFromSystems(entity);

        // Reset the component signature for the destroyed entity
        entityComponentSignatures[entity.getId()].reset();

        // Remove the entity from the component pools
        for (auto pool : componentPools) {
            if (pool) {
                pool->remove(entity.getId());
            }
        }

        // Make the entity id available to be reused
        freeIds.push_back(entity.getId());
    }
    entitiesToBeDestroyed.clear();
}