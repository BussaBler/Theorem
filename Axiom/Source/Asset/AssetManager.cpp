#include "axpch.h"

#include "AssetManager.h"

#include "Asset/Asset.h"
#include "Asset/MaterialAsset.h"
#include "Asset/UUID.h"
#include "AxImageLoader.h"
#include "AxModelLoader.h"
#include "Core/Locator.h"
#include "Core/Log.h"
#include "Math/Color.h"
#include "MeshAsset.h"
#include "Renderer/Renderer.h"
#include "ShaderAsset.h"
#include "TextureAsset.h"
#include "Utils/FileSystem.h"
#include "Utils/JSONSerializer.h"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

namespace Axiom {
    std::unordered_map<UUID, AssetMetadata> AssetManager::registry;
    std::unordered_map<UUID, std::shared_ptr<Asset>> AssetManager::loadedAssets;
    std::unordered_map<std::string, UUID> AssetManager::assetHandles;

    std::unique_ptr<Buffer> AssetManager::globalVertexBuffer;
    std::unique_ptr<Buffer> AssetManager::globalIndexBuffer;
    uint32_t AssetManager::currentVertexCount = 0;
    uint32_t AssetManager::currentIndexCount = 0;

    const UUID AssetManager::defaultTextureHandle = 1;
    const UUID AssetManager::defaultMaterialHandle = 2;

    UUID AssetManager::importAsset(const std::string& name, const std::filesystem::path& path, AssetType type) {
        std::string cacheString = path.lexically_normal().generic_string();
        if (assetHandles.find(cacheString) != assetHandles.end()) {
            return assetHandles[cacheString];
        }

        if (!FileSystem::exists(path)) {
            AX_CORE_LOG_ERROR("Tried to import an asset that does not exist: {}", path.generic_string());
            return UUID();
        }

        UUID newID = UUID::generate();
        AssetMetadata meta = {.name = name, .type = type, .filePath = path};

        registry[newID] = meta;
        assetHandles[cacheString] = newID;

        return newID;
    }

    void AssetManager::init() {
        uint64_t globalBufferSize = Math::megabytes(512);

        Buffer::CreateInfo vertexBufferCreateInfo = {
            .size = globalBufferSize, .usage = BufferUsage::Vertex | BufferUsage::TransferDst, .memoryUsage = MemoryUsage::GPUOnly};
        globalVertexBuffer = Locator::getRenderer()->createBuffer(vertexBufferCreateInfo);
        Buffer::CreateInfo indexBufferCreateInfo = {
            .size = globalBufferSize, .usage = BufferUsage::Index | BufferUsage::TransferDst, .memoryUsage = MemoryUsage::GPUOnly};
        globalIndexBuffer = Locator::getRenderer()->createBuffer(indexBufferCreateInfo);

        std::string manifestStr = FileSystem::readFileStr("Assets/AssetManifest.json");

        if (!manifestStr.empty()) {
            JSONValue serializerValue = JSONSerializer::deserialize(manifestStr);

            if (serializerValue.getType() == JSONValueType::Object && serializerValue.hasChild("Assets")) {
                const JSONValue& assetNode = serializerValue.getChild("Assets");
                const auto& children = assetNode.getChildren();

                for (const auto& [uuidStr, dataNode] : children) {
                    uint64_t uuidValue = std::stoull(uuidStr);

                    const auto& assetData = dataNode.getChildren();
                    std::string name = assetData.at("Name").getString();
                    std::string rawPath = assetData.at("FilePath").getString();
                    AssetType type = static_cast<AssetType>(assetData.at("Type").getInt());

                    std::string cacheString = std::filesystem::path(rawPath).lexically_normal().generic_string();

                    AssetMetadata meta = {.name = name, .type = type, .filePath = std::filesystem::path(cacheString)};

                    registry[UUID(uuidValue)] = meta;
                    assetHandles[cacheString] = UUID(uuidValue);
                }
            }
        }

        initDefaultAssets();
    }

    void AssetManager::shutdown() {
        loadedAssets.clear();
        assetHandles.clear();

        globalVertexBuffer.reset();
        globalIndexBuffer.reset();

        JSONValue root;
        JSONValue assetsNode;
        for (const auto& [uuid, meta] : registry) {
            if (!uuid.isValid() || meta.filePath.empty()) {
                continue;
            }

            JSONValue assetNode;

            JSONValue nameValue;
            nameValue.setString(meta.name);
            assetNode.setChild("Name", nameValue);

            JSONValue filePathValue;
            filePathValue.setString(meta.filePath.generic_string());
            assetNode.setChild("FilePath", filePathValue);

            JSONValue typeValue;
            typeValue.setInt(static_cast<int>(meta.type));
            assetNode.setChild("Type", typeValue);

            assetsNode.setChild(std::to_string(uuid), assetNode);
        }

        root.setChild("Assets", assetsNode);
        FileSystem::writeFile("Assets/AssetManifest.json", JSONSerializer::serialize(root));

        registry.clear();
    }

    void AssetManager::initDefaultAssets() {
        // Default texture
        const uint32_t defaultTextureSize = 4;

        Texture::CreateInfo textureCreateInfo = {.width = defaultTextureSize,
                                                 .height = defaultTextureSize,
                                                 .mipLevels = 1,
                                                 .arrayLayers = 1,
                                                 .format = Format::R8G8B8A8Unorm,
                                                 .usage = TextureUsage::Sampled | TextureUsage::TransferDst,
                                                 .aspect = TextureAspect::Color,
                                                 .initialState = TextureState::Undefined,
                                                 .memoryUsage = MemoryUsage::GPUOnly};
        std::unique_ptr<Texture> defaultTexture = Locator::getRenderer()->createTexture(textureCreateInfo);

        Buffer::CreateInfo stagingBufferCreateInfo = {
            .size = defaultTextureSize * defaultTextureSize * sizeof(uint32_t), .usage = BufferUsage::TransferSrc, .memoryUsage = MemoryUsage::GPUandCPU};
        std::unique_ptr<Buffer> stagingBuffer = Locator::getRenderer()->createBuffer(stagingBufferCreateInfo);

        // Magenta: R=255, G=0, B=255, A=255
        // Black:   R=0,   G=0, B=0,   A=255
        const uint32_t magenta = 0xFF00FFFF; // AABBGGRR (check your API's expected byte order)
        const uint32_t black = 0xFF000000;

        std::array<uint32_t, defaultTextureSize * defaultTextureSize> defaultTextureData;
        for (uint32_t y = 0; y < defaultTextureSize; y++) {
            for (uint32_t x = 0; x < defaultTextureSize; x++) {
                if ((x + y) % 2 == 0) {
                    defaultTextureData[y * defaultTextureSize + x] = magenta;
                } else {
                    defaultTextureData[y * defaultTextureSize + x] = black;
                }
            }
        }

        std::unique_ptr<CommandBuffer> commandBuffer = Locator::getRenderer()->beginSingleTimeCommands();
        stagingBuffer->setData<uint32_t>(defaultTextureData);
        commandBuffer->copyBufferToTexture(stagingBuffer.get(), defaultTexture.get(), defaultTextureSize, defaultTextureSize);
        Locator::getRenderer()->endSingleTimeCommands(commandBuffer.get());

        AssetMetadata defaultTextureMeta = {.name = "Default Texture", .type = AssetType::Texture, .filePath = ""};
        registry[defaultTextureHandle] = defaultTextureMeta;
        loadedAssets[defaultTextureHandle] = std::make_shared<TextureAsset>(defaultMaterialHandle, "Default Texture", std::move(defaultTexture));

        // Default material
        std::filesystem::path defaultShaderPath = "Assets/Shaders/BuiltIn.DefaultPBR.axs";
        UUID defaultShaderHandle = importAsset("Default PBR Shader", defaultShaderPath, AssetType::Shader);

        auto material = std::make_shared<MaterialAsset>(defaultMaterialHandle, "Default PBR", defaultShaderHandle, nullptr);
        material->setAlbedoColor(Color::lightGray());

        AssetMetadata defaultMaterialMeta = {.name = "Default Material", .type = AssetType::Material, .filePath = ""};
        registry[defaultMaterialHandle] = defaultMaterialMeta;
        loadedAssets[defaultMaterialHandle] = material;
    }

    std::shared_ptr<Asset> AssetManager::loadTexture(const std::filesystem::path& path, UUID uuid) {
        auto imageResult = AxImageLoader::loadImage(path, 4);

        if (imageResult.has_value()) {
            Texture::CreateInfo createInfo = {.width = imageResult->width,
                                              .height = imageResult->height,
                                              .mipLevels = 1,
                                              .arrayLayers = 1,
                                              .format = Format::R8G8B8A8Unorm,
                                              .usage = TextureUsage::Sampled | TextureUsage::TransferDst,
                                              .aspect = TextureAspect::Color,
                                              .initialState = TextureState::Undefined,
                                              .memoryUsage = MemoryUsage::GPUOnly};

            std::unique_ptr<Texture> texture = Locator::getRenderer()->createTexture(createInfo);

            Buffer::CreateInfo stagingBufferCreateInfo = {
                .size = imageResult->data.size(), .usage = BufferUsage::TransferSrc, .memoryUsage = MemoryUsage::GPUandCPU};
            std::unique_ptr<Buffer> stagingBuffer = Locator::getRenderer()->createBuffer(stagingBufferCreateInfo);
            stagingBuffer->setData(imageResult->data.data(), imageResult->data.size());

            std::unique_ptr<CommandBuffer> commandBuffer = Locator::getRenderer()->beginSingleTimeCommands();
            commandBuffer->copyBufferToTexture(stagingBuffer.get(), texture.get(), imageResult->width, imageResult->height);
            Locator::getRenderer()->endSingleTimeCommands(commandBuffer.get());

            return std::make_shared<TextureAsset>(uuid, path.filename().string(), std::move(texture));
        }

        AX_CORE_LOG_ERROR("Failed to load texture({}): {}", uint64_t(uuid), imageResult.error());
        return nullptr;
    }

    std::shared_ptr<Asset> AssetManager::loadShader(const std::filesystem::path& path, UUID uuid) {
        auto source = FileSystem::readFileStr(path);

        size_t vertexPos = source.find("#type vertex");
        size_t fragmentPos = source.find("#type fragment");

        std::string vertexSource = source.substr(vertexPos + 13, fragmentPos - (vertexPos + 13));
        std::string fragmentSource = source.substr(fragmentPos + 15, std::string::npos);

        std::unique_ptr<Shader> shader = Locator::getRenderer()->createShader(vertexSource, fragmentSource);
        return std::make_shared<ShaderAsset>(uuid, path.filename().string(), std::move(shader));
    }

    std::shared_ptr<Asset> AssetManager::loadMesh(const std::filesystem::path& path, UUID uuid) {
        auto modelResult = AxModelLoader::loadModel(path);

        if (modelResult.has_value()) {
            std::vector<MeshVertex> vertices;
            vertices.reserve(modelResult->vertices.size() / 3);
            for (size_t i = 0; i < modelResult->vertices.size() / 3; i++) {
                MeshVertex vertex;
                vertex.position = {modelResult->vertices[i * 3], modelResult->vertices[i * 3 + 1], modelResult->vertices[i * 3 + 2]};
                vertex.normal = {modelResult->normals[i * 3], modelResult->normals[i * 3 + 1], modelResult->normals[i * 3 + 2]};
                vertex.uv = {modelResult->texCoords[i * 2], modelResult->texCoords[i * 2 + 1]};
                vertices.push_back(vertex);
            }

            uint32_t vertexCount = vertices.size();
            uint32_t indexCount = modelResult->indices.size();
            uint32_t vertexBytes = vertexCount * sizeof(MeshVertex);
            uint32_t indexBytes = indexCount * sizeof(uint32_t);

            Buffer::CreateInfo vertexStagingInfo = {.size = vertexBytes, .usage = BufferUsage::TransferSrc, .memoryUsage = MemoryUsage::GPUandCPU};
            std::unique_ptr<Buffer> vertexStaging = Locator::getRenderer()->createBuffer(vertexStagingInfo);
            vertexStaging->setData(vertices.data(), vertexBytes);

            Buffer::CreateInfo indexStagingInfo = {.size = indexBytes, .usage = BufferUsage::TransferSrc, .memoryUsage = MemoryUsage::GPUandCPU};
            std::unique_ptr<Buffer> indexStaging = Locator::getRenderer()->createBuffer(indexStagingInfo);
            indexStaging->setData(modelResult->indices.data(), indexBytes);

            auto commandBuffer = Locator::getRenderer()->beginSingleTimeCommands();
            uint32_t vertexByteDstOffset = currentVertexCount * sizeof(MeshVertex);
            uint32_t indexByteDstOffset = currentIndexCount * sizeof(uint32_t);
            commandBuffer->copyBuffer(vertexStaging.get(), globalVertexBuffer.get(), vertexBytes, vertexByteDstOffset);
            commandBuffer->copyBuffer(indexStaging.get(), globalIndexBuffer.get(), indexBytes, indexByteDstOffset);
            Locator::getRenderer()->endSingleTimeCommands(commandBuffer.get());

            currentVertexCount += vertexCount;
            currentIndexCount += indexCount;

            return std::make_shared<MeshAsset>(uuid, path.filename().string(), currentVertexCount - vertexCount, currentIndexCount - indexCount,
                                               modelResult->indices.size());
        }

        AX_CORE_LOG_ERROR("Failed to load mesh: {}", modelResult.error());
        return nullptr;
    }

    std::shared_ptr<Asset> AssetManager::loadMaterial(const std::filesystem::path& path, UUID uuid) {
        std::string fileContent = FileSystem::readFileStr(path);
        if (fileContent.empty()) {
            AX_CORE_LOG_ERROR("Failed to read material file: {}", path.generic_string());
            return nullptr;
        }

        JSONValue root = JSONSerializer::deserialize(fileContent);
        if (root.getType() != JSONValueType::Object) {
            AX_CORE_LOG_ERROR("Invalid material file format: {}", path.generic_string());
            return nullptr;
        }

        std::string name = path.filename().string();
        if (root.hasChild("Name")) {
            name = root.getChild("Name").getString();
        }

        UUID shaderHandle = UUID();
        if (root.hasChild("Shader")) {
            shaderHandle = UUID(std::stoull(root.getChild("Shader").getString()));
        }

        if (!shaderHandle.isValid()) {
            AX_CORE_LOG_ERROR("Material {} is missing a valid Shader UUID!", path.generic_string());
            // return nullptr;
        }

        auto material = std::make_shared<MaterialAsset>(uuid, name, shaderHandle, nullptr);

        if (root.hasChild("AlbedoColor")) {
            const auto& colorArray = root.getChild("AlbedoColor").getArrayElements();
            if (colorArray.size() >= 4) {
                material->setAlbedoColor(Color(colorArray[0].getFloat(), colorArray[1].getFloat(), colorArray[2].getFloat(), colorArray[3].getFloat()));
            }
        }

        if (root.hasChild("Metallic")) {
            material->setMetallic(root.getChild("Metallic").getFloat());
        }

        if (root.hasChild("Roughness")) {
            material->setRoughness(root.getChild("Roughness").getFloat());
        }

        if (root.hasChild("Emission")) {
            material->setEmission(root.getChild("Emission").getFloat());
        }

        if (root.hasChild("UVTiling")) {
            const auto& uvArray = root.getChild("UVTiling").getArrayElements();
            if (uvArray.size() >= 2) {
                material->setUvTiling(Math::Vec2(uvArray[0].getFloat(), uvArray[1].getFloat()));
            }
        }

        if (root.hasChild("AlbedoMap")) {
            material->setAlbedoMap(UUID(std::stoull(root.getChild("AlbedoMap").getString())));
        }
        if (root.hasChild("NormalMap")) {
            material->setNormalMap(UUID(std::stoull(root.getChild("NormalMap").getString())));
        }
        if (root.hasChild("MetallicRoughnessMap")) {
            material->setMetallicRoughnessMap(UUID(std::stoull(root.getChild("MetallicRoughnessMap").getString())));
        }

        return material;
    }
} // namespace Axiom
