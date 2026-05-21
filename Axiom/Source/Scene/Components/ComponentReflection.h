#pragma once
#include "Asset/Asset.h"
#include "Core/Log.h"
#include "ECS/Entity.h"

#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Axiom {
    enum class FieldType {
        None,
        Float,
        Int,
        Bool,
        String,
        Vec2,
        Vec3,
        Vec4,
        Color,
        AssetHandle,
        Enum,
    };

    struct FieldInfo {
        std::string name;
        FieldType type;
        size_t offset;
        std::vector<std::string> enumOptions = {};
        AssetType assetType = AssetType::None;
    };

    struct ComponentInfo {
        std::string name;
        size_t size;
        std::vector<FieldInfo> fields;
    };

    class ComponentReflection {
      public:
        static void init();
        inline static const ComponentInfo* getComponentInfo(const std::type_index& componentType) {
            auto it = componentRegistry.find(componentType);
            if (it != componentRegistry.end()) {
                return &it->second;
            } else {
                AX_LOG_ERROR("Component '{}' not found in reflection registry", componentType.name());
                return nullptr;
            }
        }
        inline static const ComponentInfo* getComponentInfo(const std::string& componentName) {
            for (const auto& [typeIndex, info] : componentRegistry) {
                if (info.name == componentName) {
                    return &info;
                }
            }
            return nullptr;
        }
        static void addComponent(Entity entity, const std::string& componentName, void* componentData);

        inline static const std::unordered_map<std::type_index, ComponentInfo>& getRegistry() { return componentRegistry; }

      private:
        static std::unordered_map<std::type_index, ComponentInfo> componentRegistry;
    };
} // namespace Axiom