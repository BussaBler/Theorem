#pragma once
#include "EntityManager.h"

#include <typeindex>
#include <utility>
#include <vector>

namespace Axiom {
    class IComponentArray {
      public:
        virtual ~IComponentArray() = default;
        virtual void entityDestroyed(uint32_t entityId) = 0;
        virtual bool hasEntity(uint32_t entityId) = 0;
        virtual void* getComponentPtr(uint32_t entityId) = 0;
    };

    template <typename T> class ComponentArray : public IComponentArray {
      public:
        ComponentArray() = default;
        ~ComponentArray() = default;

        void insertData(uint32_t entityId, T component) {
            if (entityToIndexMap.find(entityId) != entityToIndexMap.end()) {
                componentArray[entityToIndexMap[entityId]] = component;
                return;
            }
            size_t newIndex = size;
            entityToIndexMap[entityId] = newIndex;
            indexToEntityMap[newIndex] = entityId;
            componentArray[newIndex] = component;
            size++;
        }
        void removeData(uint32_t entityId) {
            AX_CORE_ASSERT(entityToIndexMap.find(entityId) != entityToIndexMap.end(), "Trying to remove non-existent component.");

            size_t indexOfRemovedEntity = entityToIndexMap[entityId];
            size_t indexOfLastElement = size - 1;
            componentArray[indexOfRemovedEntity] = componentArray[indexOfLastElement];

            uint32_t entityOfLastElement = indexToEntityMap[indexOfLastElement];
            entityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
            indexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

            entityToIndexMap.erase(entityId);
            indexToEntityMap.erase(indexOfLastElement);

            size--;
        }
        T& getData(uint32_t entityId) {
            AX_CORE_ASSERT(entityToIndexMap.find(entityId) != entityToIndexMap.end(), "Trying to access non-existent component.");

            return componentArray[entityToIndexMap[entityId]];
        }
        void entityDestroyed(uint32_t entityId) override {
            if (entityToIndexMap.find(entityId) != entityToIndexMap.end()) {
                removeData(entityId);
            }
        }
        bool hasEntity(uint32_t entityId) override { return entityToIndexMap.find(entityId) != entityToIndexMap.end(); }
        void* getComponentPtr(uint32_t entityId) override {
            if (entityToIndexMap.find(entityId) != entityToIndexMap.end()) {
                return &componentArray[entityToIndexMap[entityId]];
            }
            return nullptr;
        }

      private:
        std::array<T, MAX_ENTITIES> componentArray;
        std::unordered_map<uint32_t, size_t> entityToIndexMap;
        std::unordered_map<size_t, uint32_t> indexToEntityMap;
        size_t size = 0;
    };

    class ComponentManager {
      public:
        ComponentManager() = default;
        ~ComponentManager() = default;

        template <typename T> void registerComponent() {
            std::type_index typeName = typeid(T);
            AX_CORE_ASSERT(componentTypes.find(typeName) == componentTypes.end(), "Registering component type more than once.");

            componentTypes.insert({typeName, nextComponentType++});
            componentArrays.insert({typeName, std::make_shared<ComponentArray<T>>()});
        }

        template <typename T> uint8_t getComponentType() {
            std::type_index typeName = typeid(T);
            AX_CORE_ASSERT(componentTypes.find(typeName) != componentTypes.end(), "Component not registered before use.");

            return componentTypes[typeName];
        }
        uint8_t getComponentType(std::type_index type) {
            AX_CORE_ASSERT(componentTypes.find(type) != componentTypes.end(), "Component not registered before use.");

            return componentTypes[type];
        }

        template <typename T> void addComponent(uint32_t entityId, T component) { getComponentArray<T>()->insertData(entityId, component); }
        template <typename T> void removeComponent(uint32_t entityId) { getComponentArray<T>()->removeData(entityId); }
        template <typename T> T& getComponent(uint32_t entityId) { return getComponentArray<T>()->getData(entityId); }
        std::vector<std::pair<std::type_index, void*>> getComponents(uint32_t entityId) {
            std::vector<std::pair<std::type_index, void*>> components;

            for (const auto& pair : componentArrays) {
                const auto& type = pair.first;
                const auto& componentArray = pair.second;

                if (componentArray->hasEntity(entityId)) {
                    components.emplace_back(type, componentArray->getComponentPtr(entityId));
                }
            }

            return components;
        }
        void entityDestroyed(uint32_t entityId) {
            for (auto const& pair : componentArrays) {
                auto const& component = pair.second;
                component->entityDestroyed(entityId);
            }
        }

      private:
        template <typename T> std::shared_ptr<ComponentArray<T>> getComponentArray() {
            std::type_index typeName = typeid(T);
            AX_CORE_ASSERT(componentTypes.find(typeName) != componentTypes.end(), "Component not registered before use.");

            return std::static_pointer_cast<ComponentArray<T>>(componentArrays[typeName]);
        }

      private:
        std::unordered_map<std::type_index, uint8_t> componentTypes;
        std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> componentArrays;
        uint8_t nextComponentType = 0;
    };
} // namespace Axiom