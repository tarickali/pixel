#ifndef ECS_H
#define ECS_H

#include <spdlog/spdlog.h>

#include <bitset>
#include <set>
#include <vector>
#include <unordered_map>
#include <deque>
#include <typeindex>
#include <optional>

////////////////////////////////////////////////////////////////////////////////
// Component Signature
////////////////////////////////////////////////////////////////////////////////
// We will use a bitset to keep track of which components an entity has.
// This will also help keep track of which entities a system is interested in.
////////////////////////////////////////////////////////////////////////////////
const size_t MAX_COMPONENTS = 32;
using ComponentSignature = std::bitset<MAX_COMPONENTS>;

////////////////////////////////////////////////////////////////////////////////
// Entity
////////////////////////////////////////////////////////////////////////////////
// An Entity is just an ID that represents a game object.
////////////////////////////////////////////////////////////////////////////////
using EntityId = size_t;

class Entity {
    private:
        EntityId id;

    public:
        ////////////////////////////////////////////////////////////////////////
        // Constructors
        ////////////////////////////////////////////////////////////////////////
        Entity() = default;
        Entity(EntityId id) { this->id = id; }
        Entity(const Entity &entity) = default;

        EntityId getId() const { return id; }

        ////////////////////////////////////////////////////////////////////////
        // Operator overloading
        ////////////////////////////////////////////////////////////////////////
        // NOTE: We need to implement these to use an Entity like an ID.
        //////////////////////////////////////////////////////////////////////// 
        Entity &operator =(const Entity &other) = default;
        bool operator ==(const Entity &other) const { return id == other.id; }
        bool operator !=(const Entity &other) const { return id != other.id; }
        bool operator <(const Entity &other) const { return id < other.id; }
        bool operator <=(const Entity &other) const { return id <= other.id; }
        bool operator >(const Entity &other) const { return id > other.id; }
        bool operator >=(const Entity &other) const { return id >= other.id; }
};

////////////////////////////////////////////////////////////////////////////////
// Entity Manager
////////////////////////////////////////////////////////////////////////////////
// An Entity Manager manages the creation, destruction, tagging, and grouping
// of Entity objects.
////////////////////////////////////////////////////////////////////////////////
// class EntityManager {
//     private:
//         size_t numEntites = 0;

//         std::set<Entity> entitiesToBeCreated;
//         std::set<Entity> entitiesToBeDestroyed;

//         std::deque<EntityId> freeIds;
    
//     public:
//         EntityManager() = default;
//         ~EntityManager() = default;
        
//         Entity create();
//         void destroy(Entity entity);
// };

////////////////////////////////////////////////////////////////////////////////
// Component
////////////////////////////////////////////////////////////////////////////////
// A Component is pure data.
////////////////////////////////////////////////////////////////////////////////
using ComponentId = size_t;

struct IComponent {
    protected:
        static ComponentId nextId;
};

template <typename T>
struct Component : public IComponent {
    static ComponentId getId() {
        static auto id = nextId++;
        return id;
    }
};

////////////////////////////////////////////////////////////////////////////////
// Pool
////////////////////////////////////////////////////////////////////////////////
// A Pool is a vector of objects of type T
////////////////////////////////////////////////////////////////////////////////
class IPool {
    public:
        virtual ~IPool() = default;
        virtual void remove(EntityId entityId) = 0;
};

template <typename T>
class Pool : public IPool {
    private:
        std::vector<T> data;
        int size;

        std::unordered_map<int, int> entityIdToIndex;
        std::unordered_map<int, int> indexToEntityId;

    public:
        Pool(int capacity = 100) {
            size = 0;
            data.resize(capacity);
        }

        virtual ~Pool() = default;

        bool isEmpty() const {
            return size == 0;
        }

        int getSize() const {
            return size;
        }

        void resize(int n) {
            data.resize(n);
        }

        void clear() {
            data.clear();
            size = 0;
        }

        void set(int entityId, T object) {
            if (entityIdToIndex.find(entityId) != entityIdToIndex.end()) {
                // If the element already exists, simply replace the object
                int index = entityIdToIndex.at(entityId);
                data[index] = object;
            } else {
                int index = size;
                entityIdToIndex.emplace(entityId, index);
                indexToEntityId.emplace(index, entityId);

                // If necessary, resize the current capacity of the data vector
                if (index >= static_cast<int>(data.capacity())) {
                    data.resize(size * 2);
                }

                data[index] = object;
                size++;
            }
        }

        void remove(EntityId entityId) override {
            if (entityIdToIndex.find(entityId) == entityIdToIndex.end()) {
                return;
            }

            int indexOfRemoved = entityIdToIndex.at(entityId);
            int indexOfLast = size - 1;
            data[indexOfRemoved] = data[indexOfLast];

            int entityIdOfLast = indexToEntityId[indexOfLast];
            entityIdToIndex[entityIdOfLast] = indexOfRemoved;
            indexToEntityId[indexOfRemoved] = entityIdOfLast;
            
            entityIdToIndex.erase(entityId);
            indexToEntityId.erase(indexOfLast);

            size--;
        }

        T &get(int entityId) {
            return static_cast<T&>(data[entityIdToIndex[entityId]]);

            // FIXME: What happens if entityId is not found?
            // NOTE: Can use pointer instead of reference as return type and
            // return nullptr.
            // auto index = entityIdToIndex.find(entityId);
            // if (index != entityIdToIndex.end()) {
            //     return static_cast<T&>(data[index->second]);
            // } else {
            //     return nullptr;
            // }
        }

        T &operator [](int index) {
            return data[index];
        }
};

////////////////////////////////////////////////////////////////////////////////
// System
////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
class System {
    private:
        ComponentSignature componentSignature;
        std::vector<Entity> entities;
    
    public:
        System() = default;
        ~System() = default;

        void addEntityToSystem(Entity entity);
        void removeEntityToSystem(Entity entity);
        std::vector<Entity> getSystemEntities() const;
        const ComponentSignature getComponentSignature() const;

        template <typename TComponent> void requireComponent();
};

////////////////////////////////////////////////////////////////////////////////
// Coordinator
////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
class Coordinator {
    private:
        ////////////////////////////////////////////////////////////////////////
        // Entity management
        ////////////////////////////////////////////////////////////////////////
        size_t numEntites = 0;
        std::set<Entity> entitiesToBeCreated;
        std::set<Entity> entitiesToBeDestroyed;
        std::deque<EntityId> freeIds;

        ////////////////////////////////////////////////////////////////////////
        // Component management 
        ////////////////////////////////////////////////////////////////////////
        // A vector of component pools, each pool contains all the data for a
        // certain component type.
        // [ Vector index = component type id ]
        // [ Pool index = entity id ]
        std::vector<std::shared_ptr<IPool>> componentPools;

        ////////////////////////////////////////////////////////////////////////
        // System management 
        ////////////////////////////////////////////////////////////////////////
        std::unordered_map<std::type_index, std::shared_ptr<System>> systems;

        ////////////////////////////////////////////////////////////////////////
        // Entity-Component-System management
        ////////////////////////////////////////////////////////////////////////
        // A vector of component signatures for each entity, indicating which
        // component is turned "on" for each entity.
        // [ Vector index = entity id ]
        std::vector<ComponentSignature> entityComponentSignatures;
        
        ////////////////////////////////////////////////////////////////////////
        // Tag and Group management
        ////////////////////////////////////////////////////////////////////////
        std::unordered_map<std::string, Entity> entityPerTag;
        std::unordered_map<EntityId, std::string> tagPerEntityId;
        std::unordered_map<std::string, std::set<Entity>> entitiesPerGroup;
        std::unordered_map<EntityId, std::set<std::string>> groupsPerEntityId;
    
    public:
        Coordinator();
        ~Coordinator();

        ////////////////////////////////////////////////////////////////////////
        // Entity management
        ////////////////////////////////////////////////////////////////////////
        Entity create();
        void destroy(Entity entity);

        ////////////////////////////////////////////////////////////////////////
        // Component management
        ////////////////////////////////////////////////////////////////////////
        template <typename TComponent, typename ...TArgs> void addComponent(Entity entity, TArgs &&...args);
        template <typename TComponent> void removeComponent(Entity entity);
        template <typename TComponent> bool hasComponent(Entity entity) const;
        template <typename TComponent> TComponent &getComponent(Entity entity) const;

        ////////////////////////////////////////////////////////////////////////
        // System management
        ////////////////////////////////////////////////////////////////////////
        template <typename TSystem, typename ...TArgs> void addSystem(TArgs &&...args);
        template <typename TSystem> void removeSystem();
        template <typename TSystem> bool hasSystem() const;
        template <typename TSystem> TSystem &getSystem() const;

        ////////////////////////////////////////////////////////////////////////
        // Entity-System management
        ////////////////////////////////////////////////////////////////////////
        void addEntityToSystems(Entity entity);
        void removeEntityFromSystems(Entity entity);

        ////////////////////////////////////////////////////////////////////////
        // Tag and Group management
        ////////////////////////////////////////////////////////////////////////
        void tagEntity(Entity entity, const std::string &tag);
        bool entityHasTag(Entity entity, const std::string &tag) const;
        std::optional<Entity> getEntityByTag(const std::string &tag) const;
        void removeEntityTag(Entity entity);
        void removeTag(const std::string &tag);

        void groupEntity(Entity entity, const std::string &group);
        bool entityBelongsToGroup(Entity entity, const std::string &group);
        std::vector<Entity> getEntitiesByGroup(const std::string &group);
        void removeEntityGroup(Entity entity, const std::string &group);
        void removeEntityGroups(Entity entity);
        void removeGroup(const std::string &group);
        
        ////////////////////////////////////////////////////////////////////////
        // General
        ////////////////////////////////////////////////////////////////////////
        void update();
};

////////////////////////////////////////////////////////////////////////////////
// Template Implementations
////////////////////////////////////////////////////////////////////////////////

template <typename TComponent, typename ...TArgs>
void Coordinator::addComponent(Entity entity, TArgs &&...args) {
    const auto componentId = Component<TComponent>::getId();

    const auto entityId = entity.getId();

    // Resize the component pools vector if necessary to accomodate component 
    if (componentId >= componentPools.size()) {
        componentPools.resize(componentId + 1, nullptr);
    }
 
    // Add a new component pool to component pools vector if necessary
    if (!componentPools[componentId]) {
        std::shared_ptr<Pool<TComponent>> componentPool = std::make_shared<Pool<TComponent>>();
        componentPools[componentId] = componentPool;
    }

    spdlog::info("add new component pool");

    // Get the component pool
    std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);

    // Create a new component
    TComponent newComponent(std::forward<TArgs>(args)...);


    // Add entity-component relationship into component pool
    componentPool->set(entityId, newComponent);

    // Set this component bit in entity's component signature
    entityComponentSignatures[entityId].set(componentId, true);

    spdlog::info("set component siganture");
}

template <typename TComponent>
void Coordinator::removeComponent(Entity entity) {
    const auto componentId = Component<TComponent>::getId();
    const auto entityId = entity.getId();

    // Do nothing if the component is not valid (not in component pools or is a nullptr)
    if (componentId >= componentPools.size() || !componentPools[componentId]) {
        return;
    }

    // Remove the entity from the component pool
    std::shared_ptr<TComponent> componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);
    componentPool->remove(entityId);

    // Unset this component bit in entity's component signature
    entityComponentSignatures[entityId].set(componentId, false);
}

template <typename TComponent>
bool Coordinator::hasComponent(Entity entity) const {
    const auto componentId = Component<TComponent>::getId();
    const auto entityId = entity.getId();
    return entityComponentSignatures[entityId].test(componentId);
}

template <typename TComponent>
TComponent &Coordinator::getComponent(Entity entity) const {
    // FIXME: We are assuming that an entity will have the component here!

    const auto componentId = Component<TComponent>::getId();
    const auto entityId = entity.getId();
    std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);
    return componentPool->get(entityId);
}

template <typename TSystem, typename ...TArgs>
void Coordinator::addSystem(TArgs &&...args) {
    // NOTE: A system can be added multiple times, but will replace the old one
    std::shared_ptr<TSystem> newSystem = std::make_shared<TSystem>(std::forward<TArgs>(args)...);
    systems.insert(std::make_pair(std::type_index(typeid(TSystem)), newSystem));
}

template <typename TSystem>
void Coordinator::removeSystem() {
    auto system = systems.find(std::type_index(typeid(TSystem)));
    if (system != systems.end()) {
        systems.erase(system);
    }
}

template <typename TSystem>
bool Coordinator::hasSystem() const {
    return systems.find(std::type_index(typeid(TSystem))) != systems.end();
}

template <typename TSystem>
TSystem &Coordinator::getSystem() const {
    // FIXME: We are assuming that an entity will have the component here!
    auto system = systems.find(std::type_index(typeid(TSystem)));
    return *(std::static_pointer_cast<TSystem>(system->second));
}

template <typename TComponent>
void System::requireComponent() {
    componentSignature.set(Component<TComponent>::getId());
}

#endif