#include "ECS.h"

#include <spdlog/spdlog.h>
#include <iostream>

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

void Coordinator::tagEntity(Entity entity, const std::string &tag) {
    if (entityPerTag.find(tag) == entityPerTag.end()) {
        entityPerTag.emplace(tag, entity);
        tagPerEntityId.emplace(entity.getId(), tag);
    }

    // std::cout << "Current tags after addition" << std::endl;
    // for (auto entityTag : tagPerEntityId) {
    //     std::cout << "Entity: " << entityTag.first << ", Tag: " << entityTag.second << std::endl;
    // }
}

bool Coordinator::entityHasTag(Entity entity, const std::string &tag) const {
    return (
        entityPerTag.find(tag) != entityPerTag.end()
        &&
        entityPerTag.at(tag) == entity
    );
}

std::optional<Entity> Coordinator::getEntityByTag(const std::string &tag) const {
    if (entityPerTag.find(tag) == entityPerTag.end()) {
        return std::nullopt;
    }
    return entityPerTag.at(tag);
}

void Coordinator::removeEntityTag(Entity entity) {
    auto entityTag = tagPerEntityId.find(entity.getId());
    if (entityTag != tagPerEntityId.end()) {
        tagPerEntityId.erase(entityTag->first);
        entityPerTag.erase(entityTag->second);
    }

    // std::cout << "Current tags after removal" << std::endl;
    // for (auto entityTag : tagPerEntityId) {
    //     std::cout << "Entity: " << entityTag.first << ", Tag: " << entityTag.second << std::endl;
    // }
}

void Coordinator::removeTag(const std::string &tag) {
    auto tagEntity = entityPerTag.find(tag);
    if (tagEntity != entityPerTag.end()) {
        entityPerTag.erase(tagEntity->first);
        tagPerEntityId.erase(tagEntity->second.getId());
    }
}

void Coordinator::groupEntity(Entity entity, const std::string &group) {
    if (entitiesPerGroup.find(group) == entitiesPerGroup.end()) {
        entitiesPerGroup.emplace(group, std::set<Entity>());
    }
    if (groupsPerEntityId.find(entity.getId()) == groupsPerEntityId.end()) {
        groupsPerEntityId.emplace(entity.getId(), std::set<std::string>());
    }

    entitiesPerGroup.at(group).insert(entity);
    groupsPerEntityId.at(entity.getId()).insert(group);
}

bool Coordinator::entityBelongsToGroup(Entity entity, const std::string &group) {
    if (entitiesPerGroup.find(group) == entitiesPerGroup.end()) {
        return false;
    }
    auto groupEntities = entitiesPerGroup.at(group);
    return groupEntities.find(entity) != groupEntities.end();
}

std::vector<Entity> Coordinator::getEntitiesByGroup(const std::string &group) {
    if (entitiesPerGroup.find(group) == entitiesPerGroup.end()) {
        return std::vector<Entity>();
    }
    // TODO: FIXME: Does this need to return a reference?
    auto groupEntities = entitiesPerGroup.at(group);
    return std::vector<Entity>(groupEntities.begin(), groupEntities.end());
}

void Coordinator::removeEntityGroup(Entity entity, const std::string &group) {
    // Check if entity is registered in any group
    if (groupsPerEntityId.find(entity.getId()) != groupsPerEntityId.end()) {
        // Get all the groups the entity is in
        auto &entityGroups = groupsPerEntityId.at(entity.getId());
        // Get all the entities the group contains
        auto &groupEntities = entitiesPerGroup.at(group);
        // Check if the entity is registered in the provided group
        if (entityGroups.find(group) != entityGroups.end()) {
            // Unregister the entity from the group
            entityGroups.erase(group);
            groupEntities.erase(entity);

            // Remove the entity from the group system if no groups are applied
            if (entityGroups.empty()) {
                groupsPerEntityId.erase(entity.getId());
            }
            // Remove the group from the group system if no entites are contained
            if (groupEntities.empty()) {
                entitiesPerGroup.erase(group);
            }
        }
    }
}

void Coordinator::removeEntityGroups(Entity entity) {
    if (groupsPerEntityId.find(entity.getId()) != groupsPerEntityId.end()) {
        auto entityGroups = groupsPerEntityId.at(entity.getId());
        for (auto group : entityGroups) {
            entitiesPerGroup.at(group).erase(entity);
            if (entitiesPerGroup.at(group).empty()) {
                entitiesPerGroup.erase(group);
            }
        }
        groupsPerEntityId.erase(entity.getId());
    }
}

void Coordinator::removeGroup(const std::string &group) {
    if (entitiesPerGroup.find(group) != entitiesPerGroup.end()) {
        auto groupEntities = entitiesPerGroup.at(group);
        for (auto entity : groupEntities) {
            groupsPerEntityId.at(entity.getId()).erase(group);
            if (groupsPerEntityId.at(entity.getId()).empty()) {
                groupsPerEntityId.erase(entity.getId());
            }
        }
        entitiesPerGroup.erase(group);
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

        // Remove all traces of entity in tags and groups
        removeEntityTag(entity);
        removeEntityGroups(entity);
    }
    entitiesToBeDestroyed.clear();
}