#pragma once
#include "Buffer.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "ResourceSet.h"

#include <cstdint>
#include <vector>

namespace Axiom {
    class CommandBuffer {
      public:
        CommandBuffer() = default;
        virtual ~CommandBuffer() = default;

        virtual void begin() = 0;
        virtual void end() = 0;
        virtual void beginRendering(const RenderPass& renderPass) = 0;
        virtual void endRendering() = 0;
        virtual void bindPipeline(Pipeline* pipeline) = 0;
        virtual void setViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
        virtual void setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;
        virtual void bindResources(const std::vector<ResourceSet*>& resourceSets, uint32_t firstSet = 0) = 0;
        virtual void bindPushConstants(const void* data, uint32_t size, uint32_t offset = 0) = 0;
        virtual void bindVertexBuffers(const std::vector<Buffer*>& vertexBuffers) = 0;
        virtual void bindIndexBuffer(Buffer* indexBuffer) = 0;
        virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
        virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                                 uint32_t firstInstance = 0) = 0;
        virtual void pipelineBarrier(const std::vector<Texture::Barrier>& textureBarries) = 0;
        virtual void copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, uint64_t size, uint64_t srcOffset = 0, uint64_t dstOffset = 0) = 0;
        virtual void copyBufferToTexture(Buffer* srcBuffer, Texture* dstTexture, uint32_t width, uint32_t height, uint32_t mipLevel = 0,
                                         uint32_t arrayLayer = 0) = 0;
    };
} // namespace Axiom
