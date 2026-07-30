#pragma once
#include "Asset.h"
#include "Asset/MaterialAsset.h"
#include "Asset/UUID.h"
#include "Core/Locator.h"
#include "Core/Log.h"
#include "Renderer/Buffer.h"
#include "TextureAsset.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace Axiom {
    struct AssetMetadata {
        std::string name = "";
        AssetType type = AssetType::None;
        std::filesystem::path filePath = "";
    };

    class AssetManager {
        friend class Application;

      public:
        template <typename T> static std::shared_ptr<T> getAsset(UUID handle) {
            if (!handle.isValid()) {
                return nullptr;
            }

            if (loadedAssets.find(handle) != loadedAssets.end()) {
                return std::static_pointer_cast<T>(loadedAssets[handle]);
            }

            if (registry.find(handle) == registry.end()) {
                AX_CORE_LOG_ERROR("Tried to load an asset that is not registered: {}", handle);
                return nullptr;
            }

            const AssetMetadata& meta = registry[handle];
            std::shared_ptr<Asset> newAsset = nullptr;

            switch (meta.type) {
            case AssetType::Texture:
                newAsset = loadTexture(meta.filePath, handle);
                break;
            case AssetType::Shader:
                newAsset = loadShader(meta.filePath, handle);
                break;
            case AssetType::Mesh:
                newAsset = loadMesh(meta.filePath, handle);
                break;
            case AssetType::Material:
                newAsset = loadMaterial(meta.filePath, handle);
            default:
                break;
            }

            if (newAsset) {
                loadedAssets[handle] = newAsset;
                return std::static_pointer_cast<T>(newAsset);
            }

            return nullptr;
        }

        static UUID importAsset(const std::string& name, const std::filesystem::path& path, AssetType type);

        static Buffer* getGlobalVertexBuffer() { return globalVertexBuffer.get(); }
        static Buffer* getGlobalIndexBuffer() { return globalIndexBuffer.get(); }

        static const AssetMetadata& getMetadata(UUID handle) {
            static AssetMetadata nullMeta;
            if (!handle.isValid() || registry.find(handle) == registry.end()) {
                return nullMeta;
            }
            return registry.at(handle);
        }
        static bool isAssetRegistered(UUID handle) {
            if (!handle.isValid()) {
                return false;
            }

            return registry.find(handle) != registry.end();
        }

        static std::vector<UUID> getAssetsByType(AssetType type) {
            std::vector<UUID> handles;
            for (const auto& [handle, meta] : registry) {
                if (meta.type == type) {
                    handles.push_back(handle);
                }
            }
            return handles;
        }

        static UUID getDefaultTextureHandle() { return defaultTextureHandle; }
        static std::shared_ptr<TextureAsset> getDefaultTexture() { return std::static_pointer_cast<TextureAsset>(loadedAssets[defaultTextureHandle]); }
        static std::shared_ptr<MaterialAsset> getDefaultMaterial() { return std::static_pointer_cast<MaterialAsset>(loadedAssets[defaultMaterialHandle]); }

      private:
        static void init();
        static void shutdown();

        static void initDefaultAssets();

        static std::shared_ptr<Asset> loadTexture(const std::filesystem::path& path, UUID uuid);
        static std::shared_ptr<Asset> loadShader(const std::filesystem::path& path, UUID uuid);
        static std::shared_ptr<Asset> loadMesh(const std::filesystem::path& path, UUID uuid);
        static std::shared_ptr<Asset> loadMaterial(const std::filesystem::path& path, UUID uuid);

      private:
        static std::unordered_map<UUID, AssetMetadata> registry;
        static std::unordered_map<UUID, std::shared_ptr<Asset>> loadedAssets;
        static std::unordered_map<std::string, UUID> assetHandles;

        static std::unique_ptr<Buffer> globalVertexBuffer;
        static std::unique_ptr<Buffer> globalIndexBuffer;
        static uint32_t currentVertexCount;
        static uint32_t currentIndexCount;

        static const UUID defaultTextureHandle;
        static const UUID defaultMaterialHandle;
    };
} // namespace Axiom
