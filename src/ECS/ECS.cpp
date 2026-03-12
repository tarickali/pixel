#include "ECS.h"
#include "../Logger/Logger.h"

#include <algorithm>

unsigned int IComponent::nextId = 0;

unsigned int Entity::GetId() const {
    return id;
}

void Entity::Destroy() {
    world->DestroyEntity(*this);
}

void Entity::Tag(const std::string &tag) {
    world->TagEntity(*this, tag);
}

bool Entity::HasTag(const std::string &tag) const {
    return world->EntityHasTag(*this, tag);
}

void Entity::Group(const std::string &group) {
    world->GroupEntity(*this, group);
}

bool Entity::BelongsToGroup(const std::string &group) const {
    return world->EntityBelongsToGroup(*this, group);
}

void System::AddEntityToSystem(Entity entity) {
    entities.push_back(entity);
}

void System::RemoveEntityFromSystem(Entity entity) {
    entities.erase(
        std::remove_if(
            entities.begin(),
            entities.end(),
            [&entity](Entity other) {
                return entity == other;
            }
        ),
        entities.end()
    );
}

std::vector<Entity> System::GetSystemEntities() const {
    return entities;
}

const Signature &System::GetComponentSignature() const {
    return componentSignature;
}

Entity World::CreateEntity() {
    unsigned int entityId;

    if (freeIds.empty()) {
        entityId = numEntities++;
        if (entityId >= entityComponentSignatures.size()) {
            entityComponentSignatures.resize(entityId + 1);
        }
    } else {
        entityId = freeIds.front();
        freeIds.pop_front();
    }

    Entity entity(entityId);
    entity.world = this;
    entitiesToBeCreated.insert(entity);

    Logger::Log("Entity created with id = " + std::to_string(entityId));

    return entity;
}

void World::DestroyEntity(Entity entity) {
    entitiesToBeDestroyed.insert(entity);
    Logger::Log("Entity destroyed with id = " + std::to_string(entity.GetId()));
}

void World::AddEntityToSystems(Entity entity) {
    const auto entityId = entity.GetId();

    if (entityId >= entityComponentSignatures.size()) {
        return;
    }

    const auto &entityComponentSignature = entityComponentSignatures[entityId];

    for (auto &system : systems) {
        const auto &systemComponentSignature = system.second->GetComponentSignature();
        bool isInterested = (entityComponentSignature & systemComponentSignature) == systemComponentSignature;
        if (isInterested) {
            system.second->AddEntityToSystem(entity);
        }
    }
}

void World::RemoveEntityFromSystems(Entity entity) {
    for (auto &system : systems) {
        system.second->RemoveEntityFromSystem(entity);
    }
}

void World::TagEntity(Entity entity, const std::string &tag) {
    entityPerTag[tag] = entity;
    tagPerEntity[entity.GetId()] = tag;
}

bool World::EntityHasTag(Entity entity, const std::string &tag) const {
    auto it = tagPerEntity.find(entity.GetId());
    return it != tagPerEntity.end() && it->second == tag;
}

Entity World::GetEntityByTag(const std::string &tag) const {
    return entityPerTag.at(tag);
}

void World::RemoveEntityTag(Entity entity) {
    auto taggedEntity = tagPerEntity.find(entity.GetId());
    if (taggedEntity != tagPerEntity.end()) {
        std::string tag = taggedEntity->second;
        entityPerTag.erase(tag);
        tagPerEntity.erase(taggedEntity);
    }
}

void World::GroupEntity(Entity entity, const std::string &group) {
    if (entitiesPerGroup.find(group) == entitiesPerGroup.end()) {
        entitiesPerGroup[group] = std::set<Entity>();
    }
    entitiesPerGroup[group].insert(entity);
    groupPerEntity[entity.GetId()] = group;
}

bool World::EntityBelongsToGroup(Entity entity, const std::string &group) const {
    if (entitiesPerGroup.find(group) == entitiesPerGroup.end()) {
        return false;
    }
    const auto &groupEntities = entitiesPerGroup.at(group);
    return groupEntities.find(entity) != groupEntities.end();
}

std::vector<Entity> World::GetEntitiesByGroup(const std::string &group) const {
    if (entitiesPerGroup.find(group) == entitiesPerGroup.end()) {
        return std::vector<Entity>();
    }
    const auto &setOfEntities = entitiesPerGroup.at(group);
    return std::vector<Entity>(setOfEntities.begin(), setOfEntities.end());
}

void World::RemoveEntityGroup(Entity entity) {
    auto groupedEntity = groupPerEntity.find(entity.GetId());
    if (groupedEntity != groupPerEntity.end()) {
        std::string group = groupedEntity->second;
        auto groupIt = entitiesPerGroup.find(group);
        if (groupIt != entitiesPerGroup.end()) {
            groupIt->second.erase(entity);
        }
        groupPerEntity.erase(groupedEntity);
    }
}

void World::Update() {
    for (auto entity : entitiesToBeCreated) {
        AddEntityToSystems(entity);
    }
    entitiesToBeCreated.clear();

    for (auto entity : entitiesToBeDestroyed) {
        RemoveEntityFromSystems(entity);
        if (entity.GetId() < entityComponentSignatures.size()) {
            entityComponentSignatures[entity.GetId()].reset();
        }

        for (auto &pool : componentPools) {
            if (pool) {
                pool->RemoveEntityFromPool(entity.GetId());
            }
        }

        freeIds.push_back(entity.GetId());

        RemoveEntityTag(entity);
        RemoveEntityGroup(entity);
    }
    entitiesToBeDestroyed.clear();
}
