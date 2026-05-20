#include "axpch.h"

#include "SceneRenderer.h"

#include "Core/Profiler.h"

namespace Axiom {
    SceneRenderer::SceneRenderer() {
        std::vector<ResourceLayout::BindingCreateInfo> globalDataLayoutBindings(1);
        globalDataLayoutBindings[0].binding = 0;
        globalDataLayoutBindings[0].type = ResourceType::UniformBuffer;
        globalDataLayoutBindings[0].stages = ShaderStage::Vertex | ShaderStage::Fragment;
        globalDataLayoutBindings[0].count = 1;
        globalDataResourceLayout = Locator::getRenderer()->createResourceLayout(globalDataLayoutBindings);

        Buffer::CreateInfo globalDataBufferCreateInfo = {
            .size = sizeof(GlobalData),
            .usage = BufferUsage::Uniform,
            .memoryUsage = MemoryUsage::GPUandCPU,
        };
        globalDataBuffer = Locator::getRenderer()->createBuffer(globalDataBufferCreateInfo);

        createOpaquePassResources();
        createSkyboxPassResources();
        createWorldGridPassResources();
        createGizmoPassResources();
    }

    void SceneRenderer::beginScene(Scene* scene, const Math::Mat4& projection, const Math::Mat4& view, const Math::Vec3& cameraPosition) {
        globalData.projection = projection;
        globalData.view = view;
        globalData.cameraPosition = Math::Vec4(cameraPosition.x(), cameraPosition.y(), cameraPosition.z(), 1.0f);

        globalData.ambientColor = Color::gray();

        auto directionalLightEntities = scene->view<TransformComponent, DirectionalLightComponent>();

        for (uint32_t entityId : directionalLightEntities) {
            auto& transform = scene->getEntity(entityId).getComponent<TransformComponent>();
            auto& directionalLight = scene->getEntity(entityId).getComponent<DirectionalLightComponent>();
            Math::Mat4 rotation = Math::Mat4::model(Math::Vec3::zero(), transform.rotation * Math::DEG_TO_RAD, Math::Vec3::one());
            Math::Vec3 lightDir = rotation.getForward();

            globalData.directionalLightColor = directionalLight.color;
            globalData.directionalLightDirection = Math::Vec4(lightDir.x(), lightDir.y(), lightDir.z(), 0.0f);
            break; // Only support one directional light for now
        }

        globalDataBuffer->setData(&globalData, sizeof(GlobalData));
    }

    void SceneRenderer::opaquePass(const SceneRenderPassData& data) {
        AX_PROFILE_FUNCTION();
        std::vector<SpriteInstance> spriteInstances;
        spriteInstances.reserve(MAX_SPRITE_INSTANCES);
        std::vector<Texture*> spriteTextureSlots = {Locator::getRenderer()->getDefaultTexture()};
        auto spriteEntities = data.scene->view<TransformComponent, Sprite2DComponent>();

        for (uint32_t entityId : spriteEntities) {
            auto& transform = data.scene->getEntity(entityId).getComponent<TransformComponent>();
            auto& sprite = data.scene->getEntity(entityId).getComponent<Sprite2DComponent>();
            std::shared_ptr<TextureAsset> textureAsset = AssetManager::getAsset<TextureAsset>(sprite.textureId);

            int32_t textureSlotIndex = 0;
            int32_t samplerType = (sprite.filterMode == SamplerFilterMode::Linear) ? 0 : 1;

            if (textureAsset) {
                Texture* rawTexture = textureAsset->getTexture();
                bool found = false;
                for (uint32_t i = 0; i < spriteTextureSlots.size(); i++) {
                    if (spriteTextureSlots[i] == rawTexture) {
                        textureSlotIndex = (int32_t)i;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    if (spriteTextureSlots.size() >= MAX_TEXTURE_SLOTS) {
                        AX_CORE_LOG_WARN("Maximum texture slots reached! Some sprites may not render correctly.");
                        textureSlotIndex = 0; // Fallback to default texture
                    } else {
                        textureSlotIndex = (int32_t)spriteTextureSlots.size();
                        spriteTextureSlots.push_back(rawTexture);
                    }
                }
            }

            Math::Mat4 model = Math::Mat4::model(transform.position, transform.rotation * Math::DEG_TO_RAD, transform.scale);
            spriteInstances.push_back({model, sprite.color, Math::Vec4((float)textureSlotIndex, (float)samplerType, 0.0f, 0.0f)});
        }

        std::unordered_map<UUID, std::vector<MeshInstance>> meshBatches;
        auto meshEntities = data.scene->view<TransformComponent, MeshComponent>();

        for (uint32_t entityId : meshEntities) {
            auto& transform = data.scene->getEntity(entityId).getComponent<TransformComponent>();
            auto& mesh = data.scene->getEntity(entityId).getComponent<MeshComponent>();
            auto model = Math::Mat4::model(transform.position, transform.rotation * Math::DEG_TO_RAD, transform.scale);
            meshBatches[mesh.meshId].push_back({model});
        }

        Math::iVec2 renderTargetSize = data.renderTarget->getSize();
        opaqueRenderPass.colorAttachments[0].texture = data.renderTarget;
        opaqueRenderPass.width = renderTargetSize.x();
        opaqueRenderPass.height = renderTargetSize.y();
        opaqueRenderPass.depthAttachment.texture = data.depthTarget;
        data.commandBuffer->beginRendering(opaqueRenderPass);

        if (!spriteInstances.empty()) {
            spriteInstanceBuffer->setData<SpriteInstance>(spriteInstances);
            std::vector<ResourceSet::Binding> resourceSetBindings(2);
            resourceSetBindings[0].binding = 0;
            resourceSetBindings[0].type = ResourceType::Texture;
            resourceSetBindings[0].textures = spriteTextureSlots;
            resourceSetBindings[0].maxNumberOfResources = MAX_TEXTURE_SLOTS;
            resourceSetBindings[1].binding = 1;
            resourceSetBindings[1].type = ResourceType::Sampler;
            resourceSetBindings[1].samplers = {Locator::getRenderer()->getLinearSampler(), Locator::getRenderer()->getNearestSampler()};
            spriteResourceSets[1]->update(resourceSetBindings);

            data.commandBuffer->bindPipeline(spritePipeline.get());
            data.commandBuffer->bindVertexBuffers({spriteVertexBuffer.get()});
            data.commandBuffer->bindIndexBuffer(spriteIndexBuffer.get());
            data.commandBuffer->setViewport(0.0f, 0.0f, renderTargetSize.x(), renderTargetSize.y());
            data.commandBuffer->setScissor(0, 0, renderTargetSize.x(), renderTargetSize.y());
            data.commandBuffer->bindResources({globalDataResourceSet.get(), spriteResourceSets[0].get(), spriteResourceSets[1].get()});
            data.commandBuffer->drawIndexed(6, spriteInstances.size(), 0, 0, 0);
        }

        if (!meshBatches.empty()) {
            globalDataBuffer->setData(&globalData, sizeof(GlobalData));

            data.commandBuffer->bindPipeline(meshPipeline.get());
            data.commandBuffer->setViewport(0.0f, 0.0f, renderTargetSize.x(), renderTargetSize.y());
            data.commandBuffer->setScissor(0, 0, renderTargetSize.x(), renderTargetSize.y());
            data.commandBuffer->bindVertexBuffers({AssetManager::getGlobalVertexBuffer()});
            data.commandBuffer->bindIndexBuffer(AssetManager::getGlobalIndexBuffer());

            for (const auto& [meshId, meshInstances] : meshBatches) {
                auto meshAsset = AssetManager::getAsset<MeshAsset>(meshId);
                if (!meshAsset) {
                    AX_CORE_LOG_WARN("Mesh asset with ID {} not found! Skipping mesh.", meshId);
                    continue;
                }

                meshInstanceBuffer->setData<MeshInstance>(meshInstances);

                data.commandBuffer->bindResources({globalDataResourceSet.get(), meshResourceSets[0].get()});
                data.commandBuffer->drawIndexed(meshAsset->getIndexCount(), meshInstances.size(), meshAsset->getIndexOffset(), meshAsset->getVertexOffset(), 0);
            }
        }

        data.commandBuffer->endRendering();
    }

    void SceneRenderer::skyboxPass(const SceneRenderPassData& data) {
        AX_PROFILE_FUNCTION();
        Math::iVec2 renderTargetSize = data.renderTarget->getSize();
        skyboxRenderPass.colorAttachments[0].texture = data.renderTarget;
        skyboxRenderPass.width = renderTargetSize.x();
        skyboxRenderPass.height = renderTargetSize.y();
        skyboxRenderPass.depthAttachment.texture = data.depthTarget;

        data.commandBuffer->beginRendering(skyboxRenderPass);
        data.commandBuffer->bindPipeline(skyboxPipeline.get());
        data.commandBuffer->bindVertexBuffers({skyboxVertexBuffer.get()});
        data.commandBuffer->bindIndexBuffer(skyboxIndexBuffer.get());
        data.commandBuffer->setViewport(0.0f, 0.0f, renderTargetSize.x(), renderTargetSize.y());
        data.commandBuffer->setScissor(0, 0, renderTargetSize.x(), renderTargetSize.y());
        data.commandBuffer->bindResources({globalDataResourceSet.get()});
        data.commandBuffer->drawIndexed(36, 1, 0, 0, 0);
        data.commandBuffer->endRendering();
    }

    void SceneRenderer::worldGridPass(const SceneRenderPassData& data) {
        AX_PROFILE_FUNCTION();
        Math::iVec2 renderTargetSize = data.renderTarget->getSize();
        worldGridRenderPass.colorAttachments[0].texture = data.renderTarget;
        worldGridRenderPass.width = renderTargetSize.x();
        worldGridRenderPass.height = renderTargetSize.y();
        worldGridRenderPass.depthAttachment.texture = data.depthTarget;

        data.commandBuffer->beginRendering(worldGridRenderPass);
        data.commandBuffer->bindPipeline(gridPipeline.get());
        data.commandBuffer->setViewport(0.0f, 0.0f, renderTargetSize.x(), renderTargetSize.y());
        data.commandBuffer->setScissor(0, 0, renderTargetSize.x(), renderTargetSize.y());
        data.commandBuffer->bindResources({globalDataResourceSet.get()});
        data.commandBuffer->draw(6, 1, 0);
        data.commandBuffer->endRendering();
    }

    void SceneRenderer::gizmoPass(const SceneRenderPassData& data, const Math::Vec3& gizmoPosition) {
        AX_PROFILE_FUNCTION();
        struct PushConstants {
            Math::Mat4 model;
            Math::Vec4 color;
        };

        PushConstants pushConstants = {.model = Math::Mat4::identity(), .color = Color::white()};
        float distanceToTheCamera =
            Math::length(Math::Vec3(globalData.cameraPosition.x(), globalData.cameraPosition.y(), globalData.cameraPosition.z()) - gizmoPosition);
        float scale = distanceToTheCamera * 0.05f;

        Math::iVec2 renderTargetSize = data.renderTarget->getSize();
        gizmoRenderPass.colorAttachments[0].texture = data.renderTarget;
        gizmoRenderPass.width = renderTargetSize.x();
        gizmoRenderPass.height = renderTargetSize.y();

        data.commandBuffer->beginRendering(gizmoRenderPass);
        data.commandBuffer->bindPipeline(gizmoPipeline.get());
        data.commandBuffer->bindVertexBuffers({AssetManager::getGlobalVertexBuffer()});
        data.commandBuffer->bindIndexBuffer(AssetManager::getGlobalIndexBuffer());
        data.commandBuffer->setViewport(0.0f, 0.0f, renderTargetSize.x(), renderTargetSize.y());
        data.commandBuffer->setScissor(0, 0, renderTargetSize.x(), renderTargetSize.y());
        data.commandBuffer->bindResources({globalDataResourceSet.get()});
        // Y axis
        pushConstants.model = Math::Mat4::model(gizmoPosition, Math::Vec3::zero(), Math::Vec3(scale));
        pushConstants.color = Color::green();
        data.commandBuffer->bindPushConstants(&pushConstants, sizeof(PushConstants));
        data.commandBuffer->drawIndexed(gizmoMesh->getIndexCount(), 1, gizmoMesh->getIndexOffset(), gizmoMesh->getVertexOffset(), 0);
        // Z axis
        pushConstants.model = Math::Mat4::model(gizmoPosition, Math::Vec3(0.0f, 0.0f, Math::PI * -0.5f), Math::Vec3(scale));
        pushConstants.color = Color::blue();
        data.commandBuffer->bindPushConstants(&pushConstants, sizeof(PushConstants));
        data.commandBuffer->drawIndexed(gizmoMesh->getIndexCount(), 1, gizmoMesh->getIndexOffset(), gizmoMesh->getVertexOffset(), 0);
        // X axis
        pushConstants.model = Math::Mat4::model(gizmoPosition, Math::Vec3(Math::PI * 0.5f, 0.0f, 0.0f), Math::Vec3(scale));
        pushConstants.color = Color::red();
        data.commandBuffer->bindPushConstants(&pushConstants, sizeof(PushConstants));
        data.commandBuffer->drawIndexed(gizmoMesh->getIndexCount(), 1, gizmoMesh->getIndexOffset(), gizmoMesh->getVertexOffset(), 0);

        data.commandBuffer->endRendering();
    }

    void SceneRenderer::createOpaquePassResources() {
        RenderAttachment colorAttachment{};
        colorAttachment.loadOp = LoadOp::Clear;
        colorAttachment.storeOp = StoreOp::Store;
        colorAttachment.clearColor = Color::white();
        opaqueRenderPass.colorAttachments[0] = colorAttachment;
        opaqueRenderPass.colorAttachmentCount = 1;
        RenderAttachment depthAttachment{};
        depthAttachment.loadOp = LoadOp::Clear;
        depthAttachment.storeOp = StoreOp::Store;
        depthAttachment.clearDepth = 1.0f;
        opaqueRenderPass.depthAttachment = depthAttachment;
        opaqueRenderPass.hasDepthAttachment = true;

        // 2d objects
        UUID spriteShaderHandle = AssetManager::importAsset("Assets/Shaders/BuiltIn.Scene.Sprite2D.axs", AssetType::Shader);
        spriteShader = AssetManager::getAsset<ShaderAsset>(spriteShaderHandle);

        Buffer::CreateInfo spriteVertexBufferCreateInfo = {
            .size = sizeof(SpriteVertex) * 4, .usage = BufferUsage::Vertex | BufferUsage::TransferDst, .memoryUsage = MemoryUsage::GPUOnly};
        spriteVertexBuffer = Locator::getRenderer()->createBuffer(spriteVertexBufferCreateInfo);
        Buffer::CreateInfo spriteVertexStagingBufferCreateInfo = {
            .size = sizeof(SpriteVertex) * 4, .usage = BufferUsage::TransferSrc, .memoryUsage = MemoryUsage::GPUandCPU};
        std::unique_ptr<Buffer> spriteVertexStagingBuffer = Locator::getRenderer()->createBuffer(spriteVertexStagingBufferCreateInfo);
        std::vector<SpriteVertex> vertices = {
            {{-0.5f, -0.5f}, {0.0f, 0.0f}}, {{0.5f, -0.5f}, {1.0f, 0.0f}}, {{0.5f, 0.5f}, {1.0f, 1.0f}}, {{-0.5f, 0.5f}, {0.0f, 1.0f}}};
        spriteVertexStagingBuffer->setData<SpriteVertex>(vertices);
        auto commandBuffer = Locator::getRenderer()->beginSingleTimeCommands();
        commandBuffer->copyBuffer(spriteVertexStagingBuffer.get(), spriteVertexBuffer.get(), spriteVertexBufferCreateInfo.size);
        Locator::getRenderer()->endSingleTimeCommands(commandBuffer.get());

        Buffer::CreateInfo spriteIndexBufferCreateInfo = {
            .size = sizeof(uint32_t) * 6, .usage = BufferUsage::Index | BufferUsage::TransferDst, .memoryUsage = MemoryUsage::GPUOnly};
        spriteIndexBuffer = Locator::getRenderer()->createBuffer(spriteIndexBufferCreateInfo);
        Buffer::CreateInfo spriteStagingBufferCreateInfo = {
            .size = sizeof(uint32_t) * 6, .usage = BufferUsage::TransferSrc, .memoryUsage = MemoryUsage::GPUandCPU};
        std::unique_ptr<Buffer> spriteStagingBuffer = Locator::getRenderer()->createBuffer(spriteStagingBufferCreateInfo);
        std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
        commandBuffer = Locator::getRenderer()->beginSingleTimeCommands();
        spriteStagingBuffer->setData<uint32_t>(indices);
        commandBuffer->copyBuffer(spriteStagingBuffer.get(), spriteIndexBuffer.get(), spriteStagingBufferCreateInfo.size);
        Locator::getRenderer()->endSingleTimeCommands(commandBuffer.get());

        Buffer::CreateInfo spriteInstanceBufferCreateInfo = {
            .size = sizeof(SpriteInstance) * MAX_SPRITE_INSTANCES, .usage = BufferUsage::Storage, .memoryUsage = MemoryUsage::GPUandCPU};
        spriteInstanceBuffer = Locator::getRenderer()->createBuffer(spriteInstanceBufferCreateInfo);

        std::vector<VertexBindingDescription> spriteVertexBindings = {{.binding = 0, .stride = sizeof(SpriteVertex), .inputRate = VertexInputRate::Vertex}};
        std::vector<VertexAttributeDescription> spriteVertexAttributes = {
            {.location = 0, .binding = 0, .format = Format::R32G32Sfloat, .offset = offsetof(SpriteVertex, position)},
            {.location = 1, .binding = 0, .format = Format::R32G32Sfloat, .offset = offsetof(SpriteVertex, uv)},
        };

        std::vector<ResourceLayout::BindingCreateInfo> spriteResourceLayoutBindings(1);
        spriteResourceLayoutBindings[0].binding = 0;
        spriteResourceLayoutBindings[0].type = ResourceType::StorageBuffer;
        spriteResourceLayoutBindings[0].stages = ShaderStage::Vertex;
        spriteResourceLayoutBindings[0].count = 1;
        spriteResourceLayouts.push_back(Locator::getRenderer()->createResourceLayout(spriteResourceLayoutBindings));
        std::vector<ResourceLayout::BindingCreateInfo> spriteResourceLayoutBindings2(2);
        spriteResourceLayoutBindings2[0].binding = 0;
        spriteResourceLayoutBindings2[0].type = ResourceType::Texture;
        spriteResourceLayoutBindings2[0].stages = ShaderStage::Fragment;
        spriteResourceLayoutBindings2[0].count = MAX_TEXTURE_SLOTS;
        spriteResourceLayoutBindings2[1].binding = 1;
        spriteResourceLayoutBindings2[1].type = ResourceType::Sampler;
        spriteResourceLayoutBindings2[1].stages = ShaderStage::Fragment;
        spriteResourceLayoutBindings2[1].count = 2; // linear and nearest sampler
        spriteResourceLayouts.push_back(Locator::getRenderer()->createResourceLayout(spriteResourceLayoutBindings2));

        std::vector<ResourceSet::Binding> spriteResourceSetBindings(1);
        spriteResourceSetBindings[0].binding = 0;
        spriteResourceSetBindings[0].type = ResourceType::StorageBuffer;
        spriteResourceSetBindings[0].buffers = {spriteInstanceBuffer.get()};

        Pipeline::CreateInfo spritePipelineCreateInfo = {
            .shader = spriteShader->getShader(),
            .vertexBindings = spriteVertexBindings,
            .vertexAttributes = spriteVertexAttributes,
            .topology = PrimitiveTopology::TriangleList,
            .polygonMode = PolygonMode::Fill,
            .cullMode = CullMode::None,
            .frontFaceClockwise = true,
            .enableBlending = false,
            .enableDepthTest = true,
            .enableDepthWrite = true,
            .colorAttachmentFormats = {Locator::getRenderer()->getRenderTargetFormat()},
            .depthAttachmentFormat = Locator::getRenderer()->getDepthTextureFormat(),
            .resourceLayouts = {globalDataResourceLayout.get(), spriteResourceLayouts[0].get(), spriteResourceLayouts[1].get()}};
        spritePipeline = Locator::getRenderer()->createPipeline(spritePipelineCreateInfo);
        globalDataResourceSet = spritePipeline->createResourceSet(globalDataResourceLayout.get());
        std::vector<ResourceSet::Binding> globalDataSetBindings(1);
        globalDataSetBindings[0].binding = 0;
        globalDataSetBindings[0].type = ResourceType::UniformBuffer;
        globalDataSetBindings[0].buffers = {globalDataBuffer.get()};
        globalDataSetBindings[0].maxNumberOfResources = 1;
        globalDataResourceSet->update(globalDataSetBindings);
        spriteResourceSets.push_back(spritePipeline->createResourceSet(spriteResourceLayouts[0].get()));
        spriteResourceSets.push_back(spritePipeline->createResourceSet(spriteResourceLayouts[1].get()));
        spriteResourceSets[0]->update(spriteResourceSetBindings);

        // 3d objects
        UUID meshShaderHandle = AssetManager::importAsset("Assets/Shaders/BuiltIn.Scene.Mesh.axs", AssetType::Shader);
        meshShader = AssetManager::getAsset<ShaderAsset>(meshShaderHandle);

        Buffer::CreateInfo meshInstanceBufferCreateInfo = {
            .size = sizeof(MeshInstance) * MAX_MESH_INSTANCES, .usage = BufferUsage::Storage, .memoryUsage = MemoryUsage::GPUandCPU};
        meshInstanceBuffer = Locator::getRenderer()->createBuffer(meshInstanceBufferCreateInfo);

        std::vector<VertexBindingDescription> meshVertexBindings = {{.binding = 0, .stride = sizeof(MeshVertex), .inputRate = VertexInputRate::Vertex}};
        std::vector<VertexAttributeDescription> meshVertexAttributes = {
            {.location = 0, .binding = 0, .format = Format::R32G32B32Sfloat, .offset = offsetof(MeshVertex, position)},
            {.location = 1, .binding = 0, .format = Format::R32G32B32Sfloat, .offset = offsetof(MeshVertex, normal)},
            {.location = 2, .binding = 0, .format = Format::R32G32Sfloat, .offset = offsetof(MeshVertex, uv)},
        };

        std::vector<ResourceLayout::BindingCreateInfo> meshResourceLayoutBindings(1);
        meshResourceLayoutBindings[0].binding = 0;
        meshResourceLayoutBindings[0].type = ResourceType::StorageBuffer;
        meshResourceLayoutBindings[0].stages = ShaderStage::Vertex;
        meshResourceLayoutBindings[0].count = 1;
        meshResourceLayouts.push_back(Locator::getRenderer()->createResourceLayout(meshResourceLayoutBindings));

        std::vector<ResourceSet::Binding> meshResourceSetBindings(1);
        meshResourceSetBindings[0].binding = 0;
        meshResourceSetBindings[0].type = ResourceType::StorageBuffer;
        meshResourceSetBindings[0].buffers = {meshInstanceBuffer.get()};

        Pipeline::CreateInfo meshPipelineCreateInfo = {.shader = meshShader->getShader(),
                                                       .vertexBindings = meshVertexBindings,
                                                       .vertexAttributes = meshVertexAttributes,
                                                       .topology = PrimitiveTopology::TriangleList,
                                                       .polygonMode = PolygonMode::Fill,
                                                       .cullMode = CullMode::Back,
                                                       .frontFaceClockwise = true,
                                                       .enableBlending = false,
                                                       .enableDepthTest = true,
                                                       .enableDepthWrite = true,
                                                       .colorAttachmentFormats = {Locator::getRenderer()->getRenderTargetFormat()},
                                                       .depthAttachmentFormat = Locator::getRenderer()->getDepthTextureFormat(),
                                                       .resourceLayouts = {globalDataResourceLayout.get(), meshResourceLayouts.back().get()}};
        meshPipeline = Locator::getRenderer()->createPipeline(meshPipelineCreateInfo);
        meshResourceSets.push_back(meshPipeline->createResourceSet(meshResourceLayouts.back().get()));
        meshResourceSets.back()->update(meshResourceSetBindings);
        globalDataResourceSet = meshPipeline->createResourceSet(globalDataResourceLayout.get());
        globalDataResourceSet->update(globalDataSetBindings);
    }

    void SceneRenderer::createSkyboxPassResources() {
        UUID skyboxShaderHandle = AssetManager::importAsset("Assets/Shaders/BuiltIn.Scene.Skybox.axs", AssetType::Shader);
        skyboxShader = AssetManager::getAsset<ShaderAsset>(skyboxShaderHandle);

        RenderAttachment colorAttachment{};
        colorAttachment.loadOp = LoadOp::Load;
        colorAttachment.storeOp = StoreOp::Store;
        colorAttachment.clearColor = Color::transparent();
        skyboxRenderPass.colorAttachments[0] = colorAttachment;
        skyboxRenderPass.colorAttachmentCount = 1;
        RenderAttachment depthAttachment{};
        depthAttachment.loadOp = LoadOp::Load;
        depthAttachment.storeOp = StoreOp::Store;
        depthAttachment.clearDepth = 1.0f;
        skyboxRenderPass.depthAttachment = depthAttachment;
        skyboxRenderPass.hasDepthAttachment = true;

        std::vector<Math::Vec3> skyboxVertices = {{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
                                                  {-0.5f, -0.5f, 0.5f},  {0.5f, -0.5f, 0.5f},  {0.5f, 0.5f, 0.5f},  {-0.5f, 0.5f, 0.5f}};
        std::vector<uint32_t> skyboxIndices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4, 0, 4, 7, 7, 3, 0, 1, 5, 6, 6, 2, 1, 3, 2, 6, 6, 7, 3, 0, 1, 5, 5, 4, 0};

        Buffer::CreateInfo skyboxVertexBufferCreateInfo = {
            .size = sizeof(Math::Vec3) * skyboxVertices.size(), .usage = BufferUsage::Vertex | BufferUsage::TransferDst, .memoryUsage = MemoryUsage::GPUOnly};
        skyboxVertexBuffer = Locator::getRenderer()->createBuffer(skyboxVertexBufferCreateInfo);
        Buffer::CreateInfo skyboxVertexStagingBufferCreateInfo = {
            .size = sizeof(Math::Vec3) * skyboxVertices.size(), .usage = BufferUsage::TransferSrc, .memoryUsage = MemoryUsage::GPUandCPU};
        std::unique_ptr<Buffer> skyboxVertexStagingBuffer = Locator::getRenderer()->createBuffer(skyboxVertexStagingBufferCreateInfo);
        skyboxVertexStagingBuffer->setData<Math::Vec3>(skyboxVertices);

        Buffer::CreateInfo skyboxIndexBufferCreateInfo = {
            .size = sizeof(uint32_t) * skyboxIndices.size(), .usage = BufferUsage::Index | BufferUsage::TransferDst, .memoryUsage = MemoryUsage::GPUOnly};
        skyboxIndexBuffer = Locator::getRenderer()->createBuffer(skyboxIndexBufferCreateInfo);
        Buffer::CreateInfo skyboxIndexStagingBufferCreateInfo = {
            .size = sizeof(uint32_t) * skyboxIndices.size(), .usage = BufferUsage::TransferSrc, .memoryUsage = MemoryUsage::GPUandCPU};
        std::unique_ptr<Buffer> skyboxIndexStagingBuffer = Locator::getRenderer()->createBuffer(skyboxIndexStagingBufferCreateInfo);
        skyboxIndexStagingBuffer->setData<uint32_t>(skyboxIndices);

        auto commandBuffer = Locator::getRenderer()->beginSingleTimeCommands();
        commandBuffer->copyBuffer(skyboxVertexStagingBuffer.get(), skyboxVertexBuffer.get(), skyboxVertexBufferCreateInfo.size);
        commandBuffer->copyBuffer(skyboxIndexStagingBuffer.get(), skyboxIndexBuffer.get(), skyboxIndexBufferCreateInfo.size);
        Locator::getRenderer()->endSingleTimeCommands(commandBuffer.get());

        std::vector<VertexBindingDescription> skyboxVertexBindings = {{.binding = 0, .stride = sizeof(Math::Vec3), .inputRate = VertexInputRate::Vertex}};
        std::vector<VertexAttributeDescription> skyboxVertexAttributes = {
            {.location = 0, .binding = 0, .format = Format::R32G32B32Sfloat, .offset = 0},
        };

        Pipeline::CreateInfo skyboxPipelineCreateInfo = {.shader = skyboxShader->getShader(),
                                                         .vertexBindings = skyboxVertexBindings,
                                                         .vertexAttributes = skyboxVertexAttributes,
                                                         .topology = PrimitiveTopology::TriangleList,
                                                         .polygonMode = PolygonMode::Fill,
                                                         .cullMode = CullMode::Front,
                                                         .frontFaceClockwise = true,
                                                         .enableBlending = false,
                                                         .enableDepthTest = true,
                                                         .enableDepthWrite = false,
                                                         .colorAttachmentFormats = {Locator::getRenderer()->getRenderTargetFormat()},
                                                         .depthAttachmentFormat = Locator::getRenderer()->getDepthTextureFormat(),
                                                         .resourceLayouts = {globalDataResourceLayout.get()}};
        skyboxPipeline = Locator::getRenderer()->createPipeline(skyboxPipelineCreateInfo);
        globalDataResourceSet = skyboxPipeline->createResourceSet(globalDataResourceLayout.get());
        std::vector<ResourceSet::Binding> globalDataSetBindings(1);
        globalDataSetBindings[0].binding = 0;
        globalDataSetBindings[0].type = ResourceType::UniformBuffer;
        globalDataSetBindings[0].buffers = {globalDataBuffer.get()};
        globalDataSetBindings[0].maxNumberOfResources = 1;
        globalDataResourceSet->update(globalDataSetBindings);
    }

    void SceneRenderer::createWorldGridPassResources() {
        UUID gridShaderHandle = AssetManager::importAsset("Assets/Shaders/BuiltIn.Scene.Grid.axs", AssetType::Shader);
        gridShader = AssetManager::getAsset<ShaderAsset>(gridShaderHandle);

        RenderAttachment colorAttachment{};
        colorAttachment.loadOp = LoadOp::Load;
        colorAttachment.storeOp = StoreOp::Store;
        colorAttachment.clearColor = Color::transparent();
        RenderAttachment depthAttachment{};
        depthAttachment.loadOp = LoadOp::Load;
        depthAttachment.storeOp = StoreOp::Store;
        depthAttachment.clearDepth = 1.0f;
        worldGridRenderPass.colorAttachments[0] = colorAttachment;
        worldGridRenderPass.colorAttachmentCount = 1;
        worldGridRenderPass.depthAttachment = depthAttachment;
        worldGridRenderPass.hasDepthAttachment = true;

        Pipeline::CreateInfo pipelineCreateInfo = {.shader = gridShader->getShader(),
                                                   .vertexBindings = {},
                                                   .vertexAttributes = {},
                                                   .topology = PrimitiveTopology::TriangleList,
                                                   .polygonMode = PolygonMode::Fill,
                                                   .cullMode = CullMode::None,
                                                   .frontFaceClockwise = true,
                                                   .enableBlending = true,
                                                   .enableDepthTest = true,
                                                   .enableDepthWrite = false,
                                                   .colorAttachmentFormats = {Locator::getRenderer()->getRenderTargetFormat()},
                                                   .depthAttachmentFormat = Locator::getRenderer()->getDepthTextureFormat(),
                                                   .resourceLayouts = {globalDataResourceLayout.get()}};
        gridPipeline = Locator::getRenderer()->createPipeline(pipelineCreateInfo);
        globalDataResourceSet = gridPipeline->createResourceSet(globalDataResourceLayout.get());
        std::vector<ResourceSet::Binding> globalDataSetBindings(1);
        globalDataSetBindings[0].binding = 0;
        globalDataSetBindings[0].type = ResourceType::UniformBuffer;
        globalDataSetBindings[0].buffers = {globalDataBuffer.get()};
        globalDataSetBindings[0].maxNumberOfResources = 1;
        globalDataResourceSet->update(globalDataSetBindings);
    }

    void SceneRenderer::createGizmoPassResources() {
        UUID gizmoShaderHandle = AssetManager::importAsset("Assets/Shaders/BuiltIn.Scene.Gizmo.axs", AssetType::Shader);
        gizmoShader = AssetManager::getAsset<ShaderAsset>(gizmoShaderHandle);
        UUID gizmoMeshHandle = AssetManager::importAsset("Assets/Models/Arrow.obj", AssetType::Mesh);
        gizmoMesh = AssetManager::getAsset<MeshAsset>(gizmoMeshHandle);

        RenderAttachment colorAttachment{};
        colorAttachment.loadOp = LoadOp::Load;
        colorAttachment.storeOp = StoreOp::Store;
        colorAttachment.clearColor = Color::transparent();
        gizmoRenderPass.colorAttachments[0] = colorAttachment;
        gizmoRenderPass.colorAttachmentCount = 1;

        std::vector<VertexBindingDescription> vertexBindings = {{.binding = 0, .stride = sizeof(MeshVertex), .inputRate = VertexInputRate::Vertex}};
        std::vector<VertexAttributeDescription> vertexAttributes = {
            {.location = 0, .binding = 0, .format = Format::R32G32B32Sfloat, .offset = offsetof(MeshVertex, position)},
            {.location = 1, .binding = 0, .format = Format::R32G32B32Sfloat, .offset = offsetof(MeshVertex, normal)},
            {.location = 2, .binding = 0, .format = Format::R32G32Sfloat, .offset = offsetof(MeshVertex, uv)},
        };

        Pipeline::CreateInfo pipelineCreateInfo = {.shader = gizmoShader->getShader(),
                                                   .vertexBindings = vertexBindings,
                                                   .vertexAttributes = vertexAttributes,
                                                   .topology = PrimitiveTopology::TriangleList,
                                                   .polygonMode = PolygonMode::Fill,
                                                   .cullMode = CullMode::Back,
                                                   .frontFaceClockwise = true,
                                                   .enableBlending = true,
                                                   .enableDepthTest = false,
                                                   .enableDepthWrite = false,
                                                   .colorAttachmentFormats = {Locator::getRenderer()->getRenderTargetFormat()},
                                                   .depthAttachmentFormat = Format::Undefined,
                                                   .resourceLayouts = {globalDataResourceLayout.get()}};
        gizmoPipeline = Locator::getRenderer()->createPipeline(pipelineCreateInfo);
        globalDataResourceSet = gizmoPipeline->createResourceSet(globalDataResourceLayout.get());
        std::vector<ResourceSet::Binding> globalDataSetBindings(1);
        globalDataSetBindings[0].binding = 0;
        globalDataSetBindings[0].type = ResourceType::UniformBuffer;
        globalDataSetBindings[0].buffers = {globalDataBuffer.get()};
        globalDataSetBindings[0].maxNumberOfResources = 1;
        globalDataResourceSet->update(globalDataSetBindings);
    }
} // namespace Axiom