#ifndef ECS_H
#define ECS_H

#include "../Logger/Logger.h"

#include <iostream>
#include <bitset>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <set>
#include <deque>
#include <memory>

const unsigned int MAX_COMPONENTS = 32;
typedef std::bitset<MAX_COMPONENTS> Signature;

class Entity {
    private:
        unsigned int id;

    public:
        Entity() = default;
        Entity(unsigned int id) : id(id) {}
        Entity(const Entity &entity) = default;
        void Destroy();
        unsigned int GetId() const;

        void Tag(const std::string &tag);
        bool HasTag(const std::string &tag) const;
        void Group(const std::string &group);
        bool BelongsToGroup(const std::string &group) const;

        Entity &operator=(const Entity &other) = default;
        bool operator==(const Entity &other) const { return id == other.id; }
        bool operator!=(const Entity &other) const { return id != other.id; }
        bool operator<(const Entity &other) const { return id < other.id; }
        bool operator<=(const Entity &other) const { return id <= other.id; }
        bool operator>(const Entity &other) const { return id > other.id; }
        bool operator>=(const Entity &other) const { return id >= other.id; }

        template <typename TComponent, typename ...TArgs> void AddComponent(TArgs &&...args);
        template <typename TComponent> void RemoveComponent();
        template <typename TComponent> bool HasComponent() const;
        template <typename TComponent> TComponent &GetComponent() const;

        class World *world;
};

struct IComponent {
    protected:
        static unsigned int nextId;
};

template <typename T>
class Component : public IComponent {
    public:
        static unsigned int GetId() {
            static auto id = nextId++;
            return id;
        }
};

class System {
    private:
        Signature componentSignature;
        std::vector<Entity> entities;

    public:
        System() = default;
        ~System() = default;

        void AddEntityToSystem(Entity entity);
        void RemoveEntityFromSystem(Entity entity);
        std::vector<Entity> GetSystemEntities() const;
        const Signature &GetComponentSignature() const;

        template <typename TComponent> void RequireComponent();
};

class IPool {
    public:
        virtual ~IPool() = default;
        virtual void RemoveEntityFromPool(unsigned int entityId) = 0;
};

template <typename T>
class Pool : public IPool {
    private:
        std::vector<T> data;
        int size;

        std::unordered_map<unsigned int, int> entityIdToIndex;
        std::unordered_map<int, unsigned int> indexToEntityId;

    public:
        Pool(int capacity = 100) {
            size = 0;
            data.resize(capacity);
        }

        ~Pool() override = default;

        bool IsEmpty() const { return size == 0; }
        int GetSize() const { return size; }
        void Resize(int n) { data.resize(n); }
        void Clear() { data.clear(); size = 0; }

        void Set(unsigned int entityId, T object) {
            if (entityIdToIndex.find(entityId) != entityIdToIndex.end()) {
                int index = entityIdToIndex.at(entityId);
                data[index] = object;
            } else {
                int index = size;
                entityIdToIndex.emplace(entityId, index);
                indexToEntityId.emplace(index, entityId);
                if (index >= static_cast<int>(data.capacity())) {
                    data.resize(size * 2 + 1);
                }
                data[index] = object;
                size++;
            }
        }

        void Remove(unsigned int entityId) {
            if (entityIdToIndex.find(entityId) == entityIdToIndex.end()) {
                return;
            }

            int indexOfRemoved = entityIdToIndex[entityId];
            int indexOfLast = size - 1;
            data[indexOfRemoved] = data[indexOfLast];

            unsigned int entityIdOfLastElement = indexToEntityId[indexOfLast];
            entityIdToIndex[entityIdOfLastElement] = indexOfRemoved;
            indexToEntityId[indexOfRemoved] = entityIdOfLastElement;

            entityIdToIndex.erase(entityId);
            indexToEntityId.erase(indexOfLast);

            size--;
        }

        void RemoveEntityFromPool(unsigned int entityId) override {
            if (entityIdToIndex.find(entityId) != entityIdToIndex.end()) {
                Remove(entityId);
            }
        }

        T &Get(unsigned int entityId) {
            int index = entityIdToIndex[entityId];
            return static_cast<T &>(data[index]);
        }

        T &operator[](unsigned int index) {
            return data[index];
        }
};

class World {
    private:
        unsigned int numEntities = 0;

        std::set<Entity> entitiesToBeCreated;
        std::set<Entity> entitiesToBeDestroyed;

        std::unordered_map<std::string, Entity> entityPerTag;
        std::unordered_map<unsigned int, std::string> tagPerEntity;

        std::unordered_map<std::string, std::set<Entity>> entitiesPerGroup;
        std::unordered_map<unsigned int, std::string> groupPerEntity;

        std::deque<unsigned int> freeIds;

        std::vector<std::shared_ptr<IPool>> componentPools;
        std::vector<Signature> entityComponentSignatures;

        std::unordered_map<std::type_index, std::shared_ptr<System>> systems;

    public:
        World() {
            Logger::Log("World created");
        }

        ~World() {
            Logger::Log("World destroyed");
        }

        Entity CreateEntity();
        void DestroyEntity(Entity entity);

        void TagEntity(Entity entity, const std::string &tag);
        bool EntityHasTag(Entity entity, const std::string &tag) const;
        Entity GetEntityByTag(const std::string &tag) const;
        void RemoveEntityTag(Entity entity);

        void GroupEntity(Entity entity, const std::string &group);
        bool EntityBelongsToGroup(Entity entity, const std::string &group) const;
        std::vector<Entity> GetEntitiesByGroup(const std::string &group) const;
        void RemoveEntityGroup(Entity entity);

        template <typename TComponent, typename ...TArgs> void AddComponent(Entity entity, TArgs &&...args);
        template <typename TComponent> void RemoveComponent(Entity entity);
        template <typename TComponent> bool HasComponent(Entity entity) const;
        template <typename TComponent> TComponent &GetComponent(Entity entity) const;

        template <typename TSystem, typename ...TArgs> void AddSystem(TArgs &&...args);
        template <typename TSystem> void RemoveSystem();
        template <typename TSystem> bool HasSystem() const;
        template <typename TSystem> TSystem &GetSystem() const;

        void AddEntityToSystems(Entity entity);
        void RemoveEntityFromSystems(Entity entity);

        void Update();
};

// Template implementations
template <typename TComponent, typename ...TArgs>
void Entity::AddComponent(TArgs &&...args) {
    world->AddComponent<TComponent>(*this, std::forward<TArgs>(args)...);
}

template <typename TComponent>
void Entity::RemoveComponent() {
    world->RemoveComponent<TComponent>(*this);
}

template <typename TComponent>
bool Entity::HasComponent() const {
    return world->HasComponent<TComponent>(*this);
}

template <typename TComponent>
TComponent &Entity::GetComponent() const {
    return world->GetComponent<TComponent>(*this);
}

template <typename TComponent>
void System::RequireComponent() {
    componentSignature.set(Component<TComponent>::GetId());
}

template <typename TComponent, typename ...TArgs>
void World::AddComponent(Entity entity, TArgs &&...args) {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();

    if (componentId >= componentPools.size()) {
        componentPools.resize(componentId + 1, nullptr);
    }

    if (!componentPools[componentId]) {
        componentPools[componentId] = std::make_shared<Pool<TComponent>>();
    }

    std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);
    TComponent newComponent(std::forward<TArgs>(args)...);
    componentPool->Set(entityId, newComponent);
    entityComponentSignatures[entityId].set(componentId);

    Logger::Log("Component id = " + std::to_string(componentId) + " was added to entity id " + std::to_string(entityId));
}

template <typename TComponent>
void World::RemoveComponent(Entity entity) {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();

    std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);
    componentPool->Remove(entityId);
    entityComponentSignatures[entityId].set(componentId, false);

    Logger::Log("Component id = " + std::to_string(componentId) + " was removed from entity id " + std::to_string(entityId));
}

template <typename TComponent>
bool World::HasComponent(Entity entity) const {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    return entityId < entityComponentSignatures.size() && entityComponentSignatures[entityId].test(componentId);
}

template <typename TComponent>
TComponent &World::GetComponent(Entity entity) const {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    auto componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);
    return componentPool->Get(entityId);
}

template <typename TSystem, typename ...TArgs>
void World::AddSystem(TArgs &&...args) {
    std::shared_ptr<TSystem> newSystem = std::make_shared<TSystem>(std::forward<TArgs>(args)...);
    systems[std::type_index(typeid(TSystem))] = newSystem;
}

template <typename TSystem>
void World::RemoveSystem() {
    auto it = systems.find(std::type_index(typeid(TSystem)));
    if (it != systems.end()) {
        systems.erase(it);
    }
}

template <typename TSystem>
bool World::HasSystem() const {
    return systems.find(std::type_index(typeid(TSystem))) != systems.end();
}

template <typename TSystem>
TSystem &World::GetSystem() const {
    auto it = systems.find(std::type_index(typeid(TSystem)));
    return *(std::static_pointer_cast<TSystem>(it->second));
}

#endif
