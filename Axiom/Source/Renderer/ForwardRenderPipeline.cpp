#include "axpch.h"

#include "ForwardRenderPipeline.h"

#include "Asset/Asset.h"
#include "Asset/AssetManager.h"
#include "Asset/MaterialAsset.h"
#include "Asset/MeshAsset.h"
#include "Asset/ShaderAsset.h"
#include "Asset/UUID.h"
#include "Core/Assert.h"
#include "Core/Locator.h"
#include "Math/Color.h"
#include "Math/Mat.h"
#include "Math/Vec.h"
#include "Renderer/Buffer.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/Pipeline.h"
#include "Renderer/RenderGraph.h"
#include "Renderer/RenderPass.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/Renderer.h"
#include "Renderer/ResourceLayout.h"
#include "Renderer/ResourceSet.h"
#include "Renderer/Texture.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Scene.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Axiom {
    ForwardRenderPipeline::ForwardRenderPipeline() {
        // TODO: create renderer objects
        createGlobalData();
        createOpaquePassData();
        createSkyboxPassData();
        createWorldGridPassData();
        createGizmosPassData();
    }

    ForwardRenderPipeline::~ForwardRenderPipeline() {
    }

    void ForwardRenderPipeline::beginFrame() {
        globalDataAllocations = 0;
        totalInstanceCount = 0;
    }

    void ForwardRenderPipeline::render(RenderGraph& renderGraph, const RenderContext& renderView) {
        AX_CORE_ASSERT(globalDataAllocations < MAX_VIEWPORTS, "Exceeded maximum viewports per frame");

        uint32_t currentViewportIndex = globalDataAllocations++;
        globalData = {.viewMatrix = renderView.viewMatrix, .projectionMatrix = renderView.projectionMatrix, .cameraPosition = renderView.cameraPosition};
        globalDataBuffer->setData(&globalData, sizeof(GlobalData), currentViewportIndex * sizeof(GlobalData));

        RGTextureHandle colorBufferHandle = renderGraph.importTexture("ColorBuffer", renderView.renderTarget);
        RGTextureHandle depthBufferHandle = renderGraph.importTexture("DepthBuffer", renderView.depthTarget);

        renderGraph.getContext().add<GlobalResourceData>(
            {.colorBuffer = colorBufferHandle, .depthBuffer = depthBufferHandle, .viewportIndex = currentViewportIndex});

        renderGraph.addPass<DefaultPassData>("Opaque Pass", [this, &renderGraph, &renderView](PassBuilder& builder, DefaultPassData& passData) {
            const auto& resourceData = renderGraph.getContext().get<GlobalResourceData>();
            builder.writeTexture(resourceData.colorBuffer, TextureState::RenderTarget);
            builder.writeTexture(resourceData.depthBuffer, TextureState::DepthStencilTarget);
            passData.renderTarget = resourceData.colorBuffer;
            passData.depthTarget = resourceData.depthBuffer;
            passData.scene = renderView.targetScene;

            return [this, &renderGraph](const DefaultPassData& passData, const PassResources& resources, CommandBuffer* cmd) {
                const auto& resourceData = renderGraph.getContext().get<GlobalResourceData>();
                opaquePass(passData.scene, resources.getTexture(passData.renderTarget), resources.getTexture(passData.depthTarget), cmd,
                           resourceData.viewportIndex);
            };
        });

        if (renderView.shouldDrawSkybox) {
            // skyboxPass(sceneRenderPassData);
        }
        if (renderView.shouldDrawWorldGrid) {
            // worldGridPass(sceneRenderPassData);
        }
        if (renderView.shouldDrawGizmos) {
            // gizmosPass(sceneRenderPassData, renderView.gizmosPosition);
        }

        renderGraph.addPass<DefaultPassData>("Prepare Present Pass", [&renderGraph](PassBuilder& builder, DefaultPassData& passData) {
            const auto& resourceData = renderGraph.getContext().get<GlobalResourceData>();
            builder.readTexture(resourceData.colorBuffer, TextureState::Present);
            return nullptr;
        });
    }

    void ForwardRenderPipeline::createGlobalData() {
        globalData = {};

        std::vector<ResourceLayout::BindingCreateInfo> bindingsCreateInfo(1);
        bindingsCreateInfo[0].binding = 0;
        bindingsCreateInfo[0].type = ResourceType::UniformBuffer;
        bindingsCreateInfo[0].stages = ShaderStage::Vertex | ShaderStage::Fragment;
        bindingsCreateInfo[0].count = 1;
        globalDataResourceLayout = Locator::getRenderer()->createResourceLayout(bindingsCreateInfo);
        globalDataResourceSet = Locator::getRenderer()->createResourceSet(globalDataResourceLayout.get());

        Buffer::CreateInfo globalDataBufferCreateInfo = {
            .size = sizeof(GlobalData) * MAX_VIEWPORTS, .usage = BufferUsage::Uniform, .memoryUsage = MemoryUsage::GPUandCPU};
        globalDataBuffer = Locator::getRenderer()->createBuffer(globalDataBufferCreateInfo);

        std::vector<ResourceSet::Binding> bindingUpdateInfo(1);
        bindingUpdateInfo[0].binding = 0;
        bindingUpdateInfo[0].type = ResourceType::UniformBuffer;
        bindingUpdateInfo[0].buffers = {globalDataBuffer.get()};
        bindingUpdateInfo[0].maxNumberOfResources = 1;
        globalDataResourceSet->update(bindingUpdateInfo);
    }

    void ForwardRenderPipeline::createOpaquePassData() {
        RenderAttachment colorAttachment = {.loadOp = LoadOp::Clear, .storeOp = StoreOp::Store, .clearColor = Color::transparent()};
        RenderAttachment depthAttachment = {.loadOp = LoadOp::Clear, .storeOp = StoreOp::Store, .clearDepth = 1.0f};
        opaqueRenderPass = {};
        opaqueRenderPass.colorAttachments[0] = colorAttachment;
        opaqueRenderPass.colorAttachmentCount = 1;
        opaqueRenderPass.depthAttachment = depthAttachment;
        opaqueRenderPass.hasDepthAttachment = true;

        std::vector<ResourceLayout::BindingCreateInfo> bindingsCreateInfo(1);
        bindingsCreateInfo[0].binding = 1;
        bindingsCreateInfo[0].type = ResourceType::StorageBuffer;
        bindingsCreateInfo[0].stages = ShaderStage::Vertex;
        bindingsCreateInfo[0].count = 1;
        instanceResourceLayout = Locator::getRenderer()->createResourceLayout(bindingsCreateInfo);
        instanceResourceSet = Locator::getRenderer()->createResourceSet(instanceResourceLayout.get());

        Buffer::CreateInfo instanceSBBOCreateInfo = {
            .size = sizeof(Math::Mat4) * MAX_INSTANCES_PER_FRAME, .usage = BufferUsage::Storage, .memoryUsage = MemoryUsage::GPUandCPU};
        instanceSBBO = Locator::getRenderer()->createBuffer(instanceSBBOCreateInfo);

        std::vector<ResourceSet::Binding> bindingsUpdateInfo(1);
        bindingsUpdateInfo[0].binding = 1;
        bindingsUpdateInfo[0].type = ResourceType::StorageBuffer;
        bindingsUpdateInfo[0].buffers = {instanceSBBO.get()};
        bindingsUpdateInfo[0].maxNumberOfResources = 1;
        instanceResourceSet->update(bindingsUpdateInfo);
    }

    void ForwardRenderPipeline::createSkyboxPassData() {
        RenderAttachment colorAttachment = {.loadOp = LoadOp::Load, .storeOp = StoreOp::Store};
        RenderAttachment depthAttachment = {.loadOp = LoadOp::Load, .storeOp = StoreOp::Store};
        skyboxRenderPass = {};
        skyboxRenderPass.colorAttachments[0] = colorAttachment;
        skyboxRenderPass.colorAttachmentCount = 1;
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

        UUID skyboxShaderHandle = AssetManager::importAsset("BuiltIn.Skybox", "Assets/Shaders/BuiltIn.Skybox.axs", AssetType::Shader);
        std::shared_ptr<ShaderAsset> skyboxShader = AssetManager::getAsset<ShaderAsset>(skyboxShaderHandle);

        std::vector<VertexBindingDescription> skyboxVertexBindings = {{.binding = 0, .stride = sizeof(Math::Vec3), .inputRate = VertexInputRate::Vertex}};
        std::vector<VertexAttributeDescription> skyboxVertexAttributes = {
            {.location = 0, .binding = 0, .format = Format::R32G32B32Sfloat, .offset = 0},
        };

        Pipeline::CreateInfo pipelineCreateInfo = {.shader = skyboxShader->getShader(),
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
        skyboxPipeline = Locator::getRenderer()->createPipeline(pipelineCreateInfo);
    }

    void ForwardRenderPipeline::createWorldGridPassData() {
        RenderAttachment colorAttachment = {.loadOp = LoadOp::Load, .storeOp = StoreOp::Store};
        RenderAttachment depthAttachment = {.loadOp = LoadOp::Load, .storeOp = StoreOp::Store};
        worldGridRenderPass = {};
        worldGridRenderPass.colorAttachments[0] = colorAttachment;
        worldGridRenderPass.colorAttachmentCount = 1;
        worldGridRenderPass.depthAttachment = depthAttachment;
        worldGridRenderPass.hasDepthAttachment = true;

        UUID worldGridShaderHandle = AssetManager::importAsset("BuiltIn.WorldGrid", "Assets/Shaders/BuiltIn.WorldGrid.axs", AssetType::Shader);
        std::shared_ptr<ShaderAsset> worldGridShader = AssetManager::getAsset<ShaderAsset>(worldGridShaderHandle);

        Pipeline::CreateInfo pipelineCreateInfo = {.shader = worldGridShader->getShader(),
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
        worldGridPipeline = Locator::getRenderer()->createPipeline(pipelineCreateInfo);
    }

    void ForwardRenderPipeline::createGizmosPassData() {
        RenderAttachment colorAttachment = {.loadOp = LoadOp::Load, .storeOp = StoreOp::Store};
        gizmosRenderPass = {};
        gizmosRenderPass.colorAttachments[0] = colorAttachment;
        gizmosRenderPass.colorAttachmentCount = 1;

        UUID gizmosShaderHandle = AssetManager::importAsset("BuiltIn.Gizmos", "Assets/Shaders/BuiltIn.Gizmos.axs", AssetType::Shader);
        UUID gizmosDefaultMeshHandle = AssetManager::importAsset("BuiltIn.Gizmos.Arrow", "Assets/Models/Arrow.obj", AssetType::Mesh);
        std::shared_ptr<ShaderAsset> gizmosShader = AssetManager::getAsset<ShaderAsset>(gizmosShaderHandle);
        gizmosDefaultMesh = AssetManager::getAsset<MeshAsset>(gizmosDefaultMeshHandle);

        std::vector<VertexBindingDescription> vertexBindings = {{.binding = 0, .stride = sizeof(MeshVertex), .inputRate = VertexInputRate::Vertex}};
        std::vector<VertexAttributeDescription> vertexAttributes = {
            {.location = 0, .binding = 0, .format = Format::R32G32B32Sfloat, .offset = offsetof(MeshVertex, position)},
            {.location = 1, .binding = 0, .format = Format::R32G32B32Sfloat, .offset = offsetof(MeshVertex, normal)},
            {.location = 2, .binding = 0, .format = Format::R32G32Sfloat, .offset = offsetof(MeshVertex, uv)},
        };

        Pipeline::CreateInfo pipelineCreateInfo = {.shader = gizmosShader->getShader(),
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
        gizmosPipeline = Locator::getRenderer()->createPipeline(pipelineCreateInfo);
    }

    void ForwardRenderPipeline::opaquePass(Scene* scene, const Texture* renderTarget, const Texture* depthTarget, CommandBuffer* commandBuffer,
                                           uint32_t viewportIndex) {
        std::vector<DrawCommand> drawList;
        drawList.reserve(1000);

        auto view = scene->view<TransformComponent, MeshComponent>();
        for (const uint32_t entityId : view) {
            auto& transform = scene->getEntity(entityId).getComponent<TransformComponent>();
            auto& meshRenderer = scene->getEntity(entityId).getComponent<MeshComponent>();

            std::shared_ptr<MaterialAsset> material = AssetManager::getAsset<MaterialAsset>(meshRenderer.meterialId);
            std::shared_ptr<MeshAsset> mesh = AssetManager::getAsset<MeshAsset>(meshRenderer.meshId);

            if (material && mesh) {
                drawList.push_back({.model = transform.modelMatrix(), .meshAsset = mesh, .materialAsset = material});
            }
        }

        if (drawList.empty()) {
            return;
        }

        // TODO: add sprite component rendering

        std::sort(drawList.begin(), drawList.end());

        std::vector<Math::Mat4> allTransforms;
        allTransforms.reserve(drawList.size());
        for (const auto& cmd : drawList) {
            allTransforms.push_back(cmd.model);
        }

        uint32_t passInstanceOffset = totalInstanceCount;
        totalInstanceCount += allTransforms.size();
        AX_CORE_ASSERT(totalInstanceCount < MAX_INSTANCES_PER_FRAME, "Instance buffer overflow!");

        instanceSBBO->setData(allTransforms.data(), allTransforms.size() * sizeof(Math::Mat4), passInstanceOffset * sizeof(Math::Mat4));

        Math::iVec2 renderTargetSize = renderTarget->getSize();
        opaqueRenderPass.colorAttachments[0].texture = renderTarget;
        opaqueRenderPass.width = renderTargetSize.x();
        opaqueRenderPass.height = renderTargetSize.y();
        opaqueRenderPass.depthAttachment.texture = depthTarget;

        commandBuffer->beginRendering(opaqueRenderPass);
        commandBuffer->setViewport(0.0f, 0.0f, renderTargetSize.x(), renderTargetSize.y());
        commandBuffer->setScissor(0, 0, renderTargetSize.x(), renderTargetSize.y());
        commandBuffer->bindVertexBuffers({AssetManager::getGlobalVertexBuffer()});
        commandBuffer->bindIndexBuffer(AssetManager::getGlobalIndexBuffer());

        UUID currentShader = UUID();
        UUID currentMaterial = UUID();
        UUID currentMesh = UUID();

        uint32_t batchStartOffset = passInstanceOffset;
        uint32_t batchInstanceCount = 0;

        struct PushConstants {
            uint32_t viewportIndex;
            uint32_t instanceIndex;
        };

        auto flushBatch = [&]() {
            if (batchInstanceCount == 0) {
                return;
            }

            std::shared_ptr<MeshAsset> activeMesh = AssetManager::getAsset<MeshAsset>(currentMesh);

            PushConstants pc = {.viewportIndex = viewportIndex, .instanceIndex = batchStartOffset};
            commandBuffer->bindPushConstants(&pc, sizeof(PushConstants));

            commandBuffer->drawIndexed(activeMesh->getIndexCount(), batchInstanceCount, activeMesh->getIndexOffset(), activeMesh->getVertexOffset());

            batchStartOffset += batchInstanceCount;
            batchInstanceCount = 0;
        };

        for (const auto& cmd : drawList) {
            bool stateChanged = false;
            if (cmd.meshAsset->getHandle() != currentMesh || cmd.materialAsset->getHandle() != currentMaterial) {
                stateChanged = true;
            }

            if (stateChanged) {
                flushBatch();
            }

            if (cmd.materialAsset->getShader() != currentShader) {
                currentShader = cmd.materialAsset->getShader();
                std::shared_ptr<ShaderAsset> currentShaderAsset = AssetManager::getAsset<ShaderAsset>(currentShader);

                std::vector<VertexBindingDescription> meshVertexBindings = {{.binding = 0, .stride = sizeof(MeshVertex), .inputRate = VertexInputRate::Vertex}};
                std::vector<VertexAttributeDescription> meshVertexAttributes = {
                    {.location = 0, .binding = 0, .format = Format::R32G32B32Sfloat, .offset = offsetof(MeshVertex, position)},
                    {.location = 1, .binding = 0, .format = Format::R32G32B32Sfloat, .offset = offsetof(MeshVertex, normal)},
                    {.location = 2, .binding = 0, .format = Format::R32G32Sfloat, .offset = offsetof(MeshVertex, uv)},
                };

                Pipeline::CreateInfo pipelineCreateInfo = {
                    .shader = currentShaderAsset->getShader(), .vertexBindings = meshVertexBindings, .vertexAttributes = meshVertexAttributes};
                pipelineCreateInfo.colorAttachmentFormats = {renderTarget->getFormat()};
                if (depthTarget) {
                    pipelineCreateInfo.depthAttachmentFormat = depthTarget->getFormat();
                }
                pipelineCreateInfo.resourceLayouts = {globalDataResourceLayout.get(), instanceResourceLayout.get()};

                commandBuffer->bindPipeline(Locator::getRenderer()->getOrCreatePipeline(pipelineCreateInfo));
                commandBuffer->bindResources({globalDataResourceSet.get(), instanceResourceSet.get()});
            }

            if (cmd.materialAsset->getHandle() != currentMaterial) {
                // Eventually bind material-specific textures here
                currentMaterial = cmd.materialAsset->getHandle();
            }
            if (cmd.meshAsset->getHandle() != currentMesh) {
                currentMesh = cmd.meshAsset->getHandle();
            }
            batchInstanceCount++;
        }

        flushBatch();
        commandBuffer->endRendering();
    }

    void ForwardRenderPipeline::skyboxPass(Scene* scene, const Texture* renderTarget, const Texture* depthTarget, CommandBuffer* commandBuffer) {
        Math::iVec2 renderTargetSize = renderTarget->getSize();
        skyboxRenderPass.width = renderTargetSize.x();
        skyboxRenderPass.height = renderTargetSize.y();
        skyboxRenderPass.colorAttachments[0].texture = renderTarget;
        skyboxRenderPass.depthAttachment.texture = depthTarget;

        commandBuffer->beginRendering(skyboxRenderPass);
        commandBuffer->setViewport(0.0f, 0.0f, renderTargetSize.x(), renderTargetSize.y());
        commandBuffer->setScissor(0, 0, renderTargetSize.x(), renderTargetSize.y());
        commandBuffer->bindPipeline(skyboxPipeline.get());
        commandBuffer->bindVertexBuffers({skyboxVertexBuffer.get()});
        commandBuffer->bindIndexBuffer(skyboxIndexBuffer.get());
        commandBuffer->bindResources({globalDataResourceSet.get()});
        commandBuffer->drawIndexed(36, 1);
        commandBuffer->endRendering();
    }

    void ForwardRenderPipeline::worldGridPass(Scene* scene, const Texture* renderTarget, const Texture* depthTarget, CommandBuffer* commandBuffer) {
        Math::iVec2 renderTargetSize = renderTarget->getSize();
        worldGridRenderPass.width = renderTargetSize.x();
        worldGridRenderPass.height = renderTargetSize.y();
        worldGridRenderPass.colorAttachments[0].texture = renderTarget;
        worldGridRenderPass.depthAttachment.texture = depthTarget;

        commandBuffer->beginRendering(worldGridRenderPass);
        commandBuffer->setViewport(0.0f, 0.0f, renderTargetSize.x(), renderTargetSize.y());
        commandBuffer->setScissor(0, 0, renderTargetSize.x(), renderTargetSize.y());
        commandBuffer->bindPipeline(worldGridPipeline.get());
        commandBuffer->bindResources({globalDataResourceSet.get()});
        commandBuffer->draw(6, 1);
        commandBuffer->endRendering();
    }

    void ForwardRenderPipeline::gizmosPass(Scene* scene, const Texture* renderTarget, const Texture* depthTarget, CommandBuffer* commandBuffer,
                                           const Math::Vec3& gizmoPosition) {
        struct PushConstants {
            Math::Mat4 model;
            Math::Vec4 color;
        };

        PushConstants pushConstants = {.model = Math::Mat4::identity(), .color = Color::white()};
        float distanceToTheCamera = Math::length(globalData.cameraPosition - gizmoPosition);
        float scale = distanceToTheCamera * 0.05f;

        Math::iVec2 renderTargetSize = renderTarget->getSize();
        gizmosRenderPass.colorAttachments[0].texture = renderTarget;
        gizmosRenderPass.width = renderTargetSize.x();
        gizmosRenderPass.height = renderTargetSize.y();

        commandBuffer->beginRendering(gizmosRenderPass);
        commandBuffer->bindPipeline(gizmosPipeline.get());
        commandBuffer->bindVertexBuffers({AssetManager::getGlobalVertexBuffer()});
        commandBuffer->bindIndexBuffer(AssetManager::getGlobalIndexBuffer());
        commandBuffer->setViewport(0.0f, 0.0f, renderTargetSize.x(), renderTargetSize.y());
        commandBuffer->setScissor(0, 0, renderTargetSize.x(), renderTargetSize.y());
        commandBuffer->bindResources({globalDataResourceSet.get()});

        // Y axis
        pushConstants.model = Math::Mat4::model(gizmoPosition, Math::Vec3::zero(), Math::Vec3(scale));
        pushConstants.color = Color::green();
        commandBuffer->bindPushConstants(&pushConstants, sizeof(PushConstants));
        commandBuffer->drawIndexed(gizmosDefaultMesh->getIndexCount(), 1, gizmosDefaultMesh->getIndexOffset(), gizmosDefaultMesh->getVertexOffset(), 0);
        // X axis
        pushConstants.model = Math::Mat4::model(gizmoPosition, Math::Vec3(0.0f, 0.0f, Math::PI * -0.5f), Math::Vec3(scale));
        pushConstants.color = Color::red();
        commandBuffer->bindPushConstants(&pushConstants, sizeof(PushConstants));
        commandBuffer->drawIndexed(gizmosDefaultMesh->getIndexCount(), 1, gizmosDefaultMesh->getIndexOffset(), gizmosDefaultMesh->getVertexOffset(), 0);
        // Z axis
        pushConstants.model = Math::Mat4::model(gizmoPosition, Math::Vec3(Math::PI * 0.5f, 0.0f, 0.0f), Math::Vec3(scale));
        pushConstants.color = Color::blue();
        commandBuffer->bindPushConstants(&pushConstants, sizeof(PushConstants));
        commandBuffer->drawIndexed(gizmosDefaultMesh->getIndexCount(), 1, gizmosDefaultMesh->getIndexOffset(), gizmosDefaultMesh->getVertexOffset(), 0);

        commandBuffer->endRendering();
    }
} // namespace Axiom
