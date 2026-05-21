#pragma once
#include "Registry.h"

#include <typeindex>

namespace Axiom {
    class Entity {
      public:
        Entity() : id(0), registry(nullptr) {}
        Entity(uint32_t id, Registry* registry) : id(id), registry(registry) {}
        ~Entity() = default;

        template <typename T> bool hasComponent() { return registry->getComponentSignature(id).test(registry->getComponentType<T>()); }
        bool hasComponent(std::type_index type) { return registry->getComponentSignature(id).test(registry->getComponentType(type)); }
        template <typename T> void addComponent(T component) { registry->addComponent<T>(id, component); }
        template <typename T> T& getComponent() { return registry->getComponent<T>(id); }
        template <typename T> const T& getComponent() const { return registry->getComponent<T>(id); }
        std::vector<std::pair<std::type_index, void*>> getComponents() { return registry->getComponents(id); }
        template <typename T> void removeComponent() { registry->removeComponent<T>(id); }

        inline uint32_t getId() const { return id; }

        operator bool() const { return registry != nullptr; }
        bool operator==(const Entity& other) const { return id == other.id; }

      private:
        uint32_t id;
        Registry* registry;
    };
} // namespace Axiom