#pragma once
#include "CommandBuffer.h"
#include "Math/Mat.h"
#include "Math/Vec.h"
#include "Renderer/RenderGraph.h"
#include "Scene/Scene.h"
#include "Texture.h"

namespace Axiom {
    struct RenderContext {

        Scene* targetScene = nullptr;
        Math::Mat4 viewMatrix = Math::Mat4::identity();
        Math::Mat4 projectionMatrix = Math::Mat4::identity();
        Math::Vec3 cameraPosition = Math::Vec3::zero();

        Texture* renderTarget = nullptr;
        Texture* depthTarget = nullptr;

        bool shouldDrawSkybox = true;
        bool shouldDrawGizmos = true;
        bool shouldDrawWorldGrid = true;

        Math::Vec3 gizmosPosition = Math::Vec3::zero();
    };

    struct DefaultPassData {
        Scene* scene;
        RGTextureHandle renderTarget;
        RGTextureHandle depthTarget;
    };

    class RenderPipeline {
      public:
        virtual ~RenderPipeline() = default;

        virtual void beginFrame() = 0;
        virtual void render(RenderGraph& renderGraph, const RenderContext& renderView) = 0;
    };
} // namespace Axiom
