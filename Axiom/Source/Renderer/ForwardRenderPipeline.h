#pragma once
#include "Asset/MaterialAsset.h"
#include "Asset/MeshAsset.h"
#include "Math/Color.h"
#include "Math/Mat.h"
#include "Math/Vec.h"
#include "Renderer/Buffer.h"
#include "Renderer/Pipeline.h"
#include "Renderer/RenderGraph.h"
#include "Renderer/RenderPass.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/ResourceLayout.h"
#include "Renderer/ResourceSet.h"

namespace Axiom {
    struct GlobalResourceData {
        RGTextureHandle colorBuffer;
        RGTextureHandle depthBuffer;
        uint32_t viewportIndex = 0;
    };

    class ForwardRenderPipeline : public RenderPipeline {
      public:
        ForwardRenderPipeline();
        ~ForwardRenderPipeline();

        void beginFrame() override;
        void render(RenderGraph& renderGraph, const RenderContext& renderView) override;

      private:
        void createGlobalData();
        void createOpaquePassData();
        void createSkyboxPassData();
        void createWorldGridPassData();
        void createGizmosPassData();

        void opaquePass(Scene* scene, const Texture* renderTarget, const Texture* depthTarget, CommandBuffer* commandBuffer, uint32_t viewportIndex);
        void skyboxPass(Scene* scene, const Texture* renderTarget, const Texture* depthTarget, CommandBuffer* commandBuffer);
        void transparentPass(Scene* scene, const Texture* renderTarget, const Texture* depthTarget, CommandBuffer* commandBuffer);
        void shadowPass(Scene* scene, const Texture* renderTarget, const Texture* depthTarget, CommandBuffer* commandBuffer);
        void worldGridPass(Scene* scene, const Texture* renderTarget, const Texture* depthTarget, CommandBuffer* commandBuffer);
        void gizmosPass(Scene* scene, const Texture* renderTarget, const Texture* depthTarget, CommandBuffer* commandBuffer, const Math::Vec3& gizmoPosition);

      private:
        struct GlobalData {
            Math::Mat4 viewMatrix = Math::Mat4::identity();
            Math::Mat4 projectionMatrix = Math::Mat4::identity();
            alignas(16) Math::Vec3 cameraPosition = Math::Vec3::zero();

            Color ambientColor = Color::white();
            alignas(16) Math::Vec3 mainLightDirection = Math::Vec3(0.0f, 1.0f, 0.0f);
            Color mainLightColor = Color::white();
        };

        struct DrawCommand {
            Math::Mat4 model = Math::Mat4::identity();
            std::shared_ptr<MeshAsset> meshAsset = nullptr;
            std::shared_ptr<MaterialAsset> materialAsset = nullptr;

            bool operator<(const DrawCommand& other) const {
                if (materialAsset->getShader() != other.materialAsset->getShader()) {
                    return materialAsset->getShader() < other.materialAsset->getShader();
                }
                return materialAsset->getHandle() < other.materialAsset->getHandle();
            }
        };

      private:
        static constexpr uint32_t GLOBAL_DATA_ALIGNMENT = 256;
        static constexpr uint32_t INSTANCE_DATA_ALIGNMENT = 256;
        static constexpr uint8_t MAX_VIEWPORTS = 10;
        static constexpr uint32_t MAX_INSTANCES_PER_FRAME = 10000;
        uint32_t globalDataAllocations = 0;
        uint32_t totalInstanceCount = 0;

        GlobalData globalData;
        std::unique_ptr<ResourceLayout> globalDataResourceLayout;
        std::unique_ptr<ResourceSet> globalDataResourceSet;
        std::unique_ptr<Buffer> globalDataBuffer;

        RenderPass opaqueRenderPass;
        std::unique_ptr<ResourceLayout> instanceResourceLayout;
        std::unique_ptr<ResourceSet> instanceResourceSet;
        std::unique_ptr<Buffer> instanceSBBO;

        RenderPass skyboxRenderPass;
        std::unique_ptr<Buffer> skyboxVertexBuffer;
        std::unique_ptr<Buffer> skyboxIndexBuffer;
        std::unique_ptr<Pipeline> skyboxPipeline;

        RenderPass worldGridRenderPass;
        std::unique_ptr<Pipeline> worldGridPipeline;

        RenderPass gizmosRenderPass;
        std::shared_ptr<MeshAsset> gizmosDefaultMesh;
        std::unique_ptr<Pipeline> gizmosPipeline;
    };
} // namespace Axiom
