#pragma once
#include "Asset/AssetManager.h"
#include "Asset/ShaderAsset.h"
#include "Components/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/MeshComponent.h"
#include "Components/Sprite2DComponent.h"
#include "Components/TagComponent.h"
#include "Components/TransformComponent.h"
#include "Core/Locator.h"
#include "Renderer/Renderer.h"
#include "Scene.h"

namespace Axiom {
    struct SceneRenderPassData {
        Scene* scene;
        CommandBuffer* commandBuffer;
        Texture* renderTarget;
        Texture* depthTarget;
    };

    class SceneRenderer {
      public:
        SceneRenderer();
        ~SceneRenderer() = default;

        // update the global data, with mattices and light info
        void beginScene(Scene* scene, const Math::Mat4& projection, const Math::Mat4& view, const Math::Vec3& cameraPosition);

        void opaquePass(const SceneRenderPassData& data);
        void skyboxPass(const SceneRenderPassData& data);
        void transparentPass(const SceneRenderPassData& data);
        void lightingPass(const SceneRenderPassData& data);
        void worldGridPass(const SceneRenderPassData& data);
        void postProcessPass(const SceneRenderPassData& data);
        void gizmoPass(const SceneRenderPassData& data, const Math::Vec3& gizmoPosition);

      private:
        void createOpaquePassResources();
        void createSkyboxPassResources();

        void createWorldGridPassResources();
        void createGizmoPassResources();

      private:
        std::unique_ptr<ResourceLayout> globalDataResourceLayout;
        std::unique_ptr<ResourceSet> globalDataResourceSet;
        std::unique_ptr<Buffer> globalDataBuffer;
        struct GlobalData {
            Math::Mat4 projection;
            Math::Mat4 view;
            Math::Vec4 cameraPosition;

            Math::Vec4 ambientColor;
            Math::Vec4 directionalLightDirection;
            Math::Vec4 directionalLightColor;
        };
        GlobalData globalData;

        // geometry pass data
        RenderPass opaqueRenderPass;
        // 2d objects
        static const uint32_t MAX_SPRITE_INSTANCES = 1000;
        static const uint8_t MAX_TEXTURE_SLOTS = 16;
        std::shared_ptr<ShaderAsset> spriteShader = nullptr;
        std::unique_ptr<Pipeline> spritePipeline = nullptr;
        std::unique_ptr<Buffer> spriteVertexBuffer = nullptr;
        std::unique_ptr<Buffer> spriteIndexBuffer = nullptr;
        std::unique_ptr<Buffer> spriteInstanceBuffer = nullptr;
        std::vector<std::unique_ptr<ResourceLayout>> spriteResourceLayouts;
        std::vector<std::unique_ptr<ResourceSet>> spriteResourceSets;
        struct SpriteInstance {
            Math::Mat4 model;
            Math::Vec4 color;
            Math::Vec4 data;
        };
        struct SpriteVertex {
            Math::Vec2 position;
            Math::Vec2 uv;
        };
        // 3d objects
        static const uint32_t MAX_MESH_INSTANCES = 1000;
        std::shared_ptr<ShaderAsset> meshShader = nullptr;
        std::unique_ptr<Pipeline> meshPipeline = nullptr;
        std::unique_ptr<Buffer> meshInstanceBuffer = nullptr;
        std::vector<std::unique_ptr<ResourceLayout>> meshResourceLayouts;
        std::vector<std::unique_ptr<ResourceSet>> meshResourceSets;
        struct MeshInstance {
            Math::Mat4 model;
        };

        // skybox pass data
        RenderPass skyboxRenderPass;
        std::shared_ptr<ShaderAsset> skyboxShader = nullptr;
        std::unique_ptr<Pipeline> skyboxPipeline = nullptr;
        std::unique_ptr<Buffer> skyboxVertexBuffer = nullptr;
        std::unique_ptr<Buffer> skyboxIndexBuffer = nullptr;

        // world grid pass data
        RenderPass worldGridRenderPass;
        std::shared_ptr<ShaderAsset> gridShader = nullptr;
        std::unique_ptr<Pipeline> gridPipeline = nullptr;

        // gizmo pass data
        static const uint32_t MAX_GIZMO_LINES = 1000;
        std::shared_ptr<ShaderAsset> gizmoShader = nullptr;
        std::shared_ptr<MeshAsset> gizmoMesh = nullptr;
        std::unique_ptr<Pipeline> gizmoPipeline = nullptr;
        RenderPass gizmoRenderPass;
    };
} // namespace Axiom