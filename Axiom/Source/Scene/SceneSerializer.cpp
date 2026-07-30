#include "axpch.h"

#include "SceneSerializer.h"

namespace Axiom {
    void SceneSerializer::serialize(const std::string& filePath) {
        JSONValue root = JSONValue();
        JSONValue entitiesArray = JSONValue();

        View entities = scene->registry->view();
        for (auto entityId : entities) {
            Entity entity = Entity(entityId, scene->registry.get());
            if (!entity.hasComponent<TagComponent>()) {
                continue;
            }

            JSONValue entityNode;
            JSONValue idValue;
            idValue.setInt(static_cast<int>(entityId));
            entityNode.setChild("EntityID", idValue);

            JSONValue componentsNode;

            auto components = scene->registry->getComponents(entityId);

            for (const auto& [typeIndex, dataPtr] : components) {
                const ComponentInfo* componentInfo = ComponentReflection::getComponentInfo(typeIndex);
                if (!componentInfo) {
                    AX_CORE_LOG_ERROR("No reflection info found for component with type index {}", typeIndex.name());
                    continue;
                }
                JSONValue componentNode;

                const auto& fields = componentInfo->fields;

                for (const FieldInfo& field : fields) {
                    void* fieldAddress = static_cast<char*>(dataPtr) + field.offset;
                    JSONValue fieldNode;

                    switch (field.type) {
                    case FieldType::Float:
                        fieldNode.setFloat(*static_cast<float*>(fieldAddress));
                        break;
                    case FieldType::Int:
                        fieldNode.setInt(*static_cast<int*>(fieldAddress));
                        break;
                    case FieldType::Bool:
                        fieldNode.setBool(*static_cast<bool*>(fieldAddress));
                        break;
                    case FieldType::String:
                        fieldNode.setString(*static_cast<std::string*>(fieldAddress));
                        break;
                    case FieldType::Vec3: {
                        Math::Vec3* vec = static_cast<Math::Vec3*>(fieldAddress);
                        JSONValue xNode, yNode, zNode;
                        xNode.setFloat(vec->x());
                        yNode.setFloat(vec->y());
                        zNode.setFloat(vec->z());

                        fieldNode.setChild("x", xNode);
                        fieldNode.setChild("y", yNode);
                        fieldNode.setChild("z", zNode);
                        break;
                    }
                    case FieldType::Color: {
                        Color* col = static_cast<Color*>(fieldAddress);
                        JSONValue rNode, gNode, bNode, aNode;
                        rNode.setFloat(col->r());
                        gNode.setFloat(col->g());
                        bNode.setFloat(col->b());
                        aNode.setFloat(col->a());

                        fieldNode.setChild("r", rNode);
                        fieldNode.setChild("g", gNode);
                        fieldNode.setChild("b", bNode);
                        fieldNode.setChild("a", aNode);
                        break;
                    }
                    case FieldType::AssetHandle:
                        fieldNode.setString(std::to_string(*static_cast<uint64_t*>(fieldAddress)));
                        break;
                    case FieldType::Enum:
                        fieldNode.setInt(*static_cast<int*>(fieldAddress));
                        break;
                    default:
                        break;
                    }
                    componentNode.setChild(field.name, fieldNode);
                }
                componentsNode.setChild(componentInfo->name, componentNode);
            }
            entityNode.setChild("Components", componentsNode);
            entitiesArray.addArrayElement(entityNode);
        }

        root.setChild("Entities", entitiesArray);

        std::string fileContent = JSONSerializer::serialize(root);
        FileSystem::writeFile(filePath, fileContent);
    }

    bool SceneSerializer::deserialize(const std::string& filePath) {
        std::string fileContent = FileSystem::readFileStr(filePath);
        if (fileContent.empty()) {
            AX_CORE_LOG_ERROR("Failed to read scene file: {0}", filePath);
            return false;
        }

        JSONValue deserializedData = JSONSerializer::deserialize(fileContent);
        if (deserializedData.getType() != JSONValueType::Object || !deserializedData.hasChild("Entities")) {
            AX_CORE_LOG_ERROR("Invalid scene format: {0}", filePath);
            return false;
        }

        const JSONValue& entitiesArray = deserializedData.getChild("Entities");
        for (const JSONValue& entityNode : entitiesArray.getArrayElements()) {

            Entity entity = scene->createEntity("Unnamed");

            if (!entityNode.hasChild("Components")) {
                continue;
            }

            const JSONValue& componentsNode = entityNode.getChild("Components");

            for (const auto& [componentName, componentNode] : componentsNode.getChildren()) {

                const ComponentInfo* componentInfo = ComponentReflection::getComponentInfo(componentName);
                if (!componentInfo) {
                    AX_CORE_LOG_ERROR("Component '{}' not found in reflection registry", componentName);
                    continue;
                }
                void* dataPtr = operator new(componentInfo->size);

                const auto& fields = componentInfo->fields;

                for (const FieldInfo& field : fields) {
                    if (!componentNode.hasChild(field.name)) {
                        continue;
                    }

                    const JSONValue& fieldNode = componentNode.getChild(field.name);
                    void* fieldAddress = static_cast<char*>(dataPtr) + field.offset;

                    switch (field.type) {
                    case FieldType::Float:
                        *static_cast<float*>(fieldAddress) = fieldNode.getFloat();
                        break;
                    case FieldType::Int:
                        *static_cast<int*>(fieldAddress) = fieldNode.getInt();
                        break;
                    case FieldType::Bool:
                        *static_cast<bool*>(fieldAddress) = fieldNode.getBool();
                        break;
                    case FieldType::String:
                        *static_cast<std::string*>(fieldAddress) = fieldNode.getString();
                        break;
                    case FieldType::Vec3: {
                        Math::Vec3* vec = static_cast<Math::Vec3*>(fieldAddress);
                        float x = fieldNode.hasChild("x") ? fieldNode.getChild("x").getFloat() : 0.0f;
                        float y = fieldNode.hasChild("y") ? fieldNode.getChild("y").getFloat() : 0.0f;
                        float z = fieldNode.hasChild("z") ? fieldNode.getChild("z").getFloat() : 0.0f;
                        *vec = Math::Vec3(x, y, z);
                        break;
                    }
                    case FieldType::Color: {
                        Color* col = static_cast<Color*>(fieldAddress);
                        float r = fieldNode.hasChild("r") ? fieldNode.getChild("r").getFloat() : 1.0f;
                        float g = fieldNode.hasChild("g") ? fieldNode.getChild("g").getFloat() : 1.0f;
                        float b = fieldNode.hasChild("b") ? fieldNode.getChild("b").getFloat() : 1.0f;
                        float a = fieldNode.hasChild("a") ? fieldNode.getChild("a").getFloat() : 1.0f;
                        col->r() = r;
                        col->g() = g;
                        col->b() = b;
                        col->a() = a;
                        break;
                    }
                    case FieldType::AssetHandle: {
                        *static_cast<uint64_t*>(fieldAddress) = std::stoull(fieldNode.getString());
                        break;
                    }
                    case FieldType::Enum:
                        *static_cast<int*>(fieldAddress) = fieldNode.getInt();
                        break;
                    default:
                        break;
                    }
                }

                ComponentReflection::addComponent(entity, componentName, dataPtr);
                operator delete(dataPtr);
            }
        }

        return true;
    }
} // namespace Axiom
