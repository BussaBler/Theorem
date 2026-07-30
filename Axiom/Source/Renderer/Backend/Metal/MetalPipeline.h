#pragma once
#include "MetalUtils.h"
#include "Renderer/Pipeline.h"

namespace Axiom {
    class MetalPipeline : public Pipeline {
      public:
        MetalPipeline(const CreateInfo& createInfo, MTL::Device* device);
        ~MetalPipeline() override;

        inline MTL::RenderPipelineState* getHandle() const { return pipelineState; }
        inline MTL::Winding getFaceWinding() const { return faceWinding; }
        inline MTL::CullMode getCullMode() const { return cullMode; }
        inline MTL::TriangleFillMode getFillMode() const { return fillMode; }
        inline MTL::DepthStencilState* getDepthStencilState() const { return depthStencilState; }
        inline MTL::PrimitiveType getPrimitiveType() const { return primitiveType; }

      private:
        MTL::Device* device;
        MTL::RenderPipelineState* pipelineState;
        MTL::Winding faceWinding;
        MTL::CullMode cullMode;
        MTL::TriangleFillMode fillMode;
        MTL::PrimitiveType primitiveType;
        MTL::DepthStencilState* depthStencilState;
    };
} // namespace Axiom
