#pragma once
#include "ComponentManager.h"
#include "EntityManager.h"

namespace Axiom {
    class View {
      public:
        View(EntityManager* entityManager, std::bitset<32> signature) : entityManager(entityManager), signature(signature) {}

        ~View() = default;

        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = uint32_t;
            using pointer = const uint32_t*;
            using reference = const uint32_t&;

            Iterator(uint32_t entityId, EntityManager* entityManager, std::bitset<32> signature)
                : entityId(entityId), entityManager(entityManager), signature(signature) {
                advanceToValid();
            }

            Iterator& operator++() {
                entityId++;
                advanceToValid();
                return *this;
            }

            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator!=(const Iterator& other) const { return entityId != other.entityId; }
            bool operator==(const Iterator& other) const { return entityId == other.entityId; }

            uint32_t operator*() const { return entityId; }

          private:
            uint32_t entityId;
            EntityManager* entityManager;
            std::bitset<32> signature;

            void advanceToValid() {
                while (entityId < MAX_ENTITIES &&
                       (!entityManager->isEntityAlive(entityId) || (entityManager->getComponentSignature(entityId) & signature) != signature)) {
                    entityId++;
                }
            }
        };

        Iterator begin() const { return Iterator(0, entityManager, signature); }
        Iterator end() const { return Iterator(MAX_ENTITIES, entityManager, signature); }

      private:
        EntityManager* entityManager;
        std::bitset<32> signature;
    };

    class Registry {
      public:
        Registry() {
            entityManager = std::make_unique<EntityManager>();
            componentManager = std::make_unique<ComponentManager>();
        }
        ~Registry() = default;

        [[nodiscard]] uint32_t createEntityId();
        void destroyEntityId(uint32_t entityId);

        template <typename T> void addComponent(uint32_t entityId, T component) {
            componentManager->addComponent<T>(entityId, component);
            entityManager->getComponentSignature(entityId).set(componentManager->getComponentType<T>(), true);
        }
        template <typename T> void removeComponent(uint32_t entityId) {
            componentManager->removeComponent<T>(entityId);
            entityManager->getComponentSignature(entityId).set(componentManager->getComponentType<T>(), false);
        }
        template <typename T> T& getComponent(uint32_t entityId) { return componentManager->getComponent<T>(entityId); }
        std::vector<std::pair<std::type_index, void*>> getComponents(uint32_t entityId) { return componentManager->getComponents(entityId); }
        std::bitset<32>& getComponentSignature(uint32_t entityId) { return entityManager->getComponentSignature(entityId); }

        template <typename T> void registerComponent() { componentManager->registerComponent<T>(); }
        template <typename T> uint8_t getComponentType() { return componentManager->getComponentType<T>(); }
        uint8_t getComponentType(std::type_index type) { return componentManager->getComponentType(type); }

        template <typename... Components> View view() {
            std::bitset<32> signature;
            ((signature.set(componentManager->getComponentType<Components>())), ...);
            return View(entityManager.get(), signature);
        }

        View view() {
            std::bitset<32> signature;
            return View(entityManager.get(), signature);
        }

      private:
        std::unique_ptr<EntityManager> entityManager;
        std::unique_ptr<ComponentManager> componentManager;
    };
} // namespace Axiom