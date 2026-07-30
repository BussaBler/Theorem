#pragma once
#include "MetalUtils.h"
#include "Renderer/CommandBuffer.h"

namespace Axiom {
    class MetalCommandBuffer : public CommandBuffer {
      public:
        MetalCommandBuffer(MTL::CommandQueue* commandQueue);
        ~MetalCommandBuffer() override;

        void begin() override;
        void end() override;
        void beginRendering(const RenderPass& renderPass) override;
        void endRendering() override;
        void bindPipeline(Pipeline* pipeline) override;
        void setViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) override;
        void setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) override;
        void bindResources(const std::vector<ResourceSet*>& resourceSets, uint32_t firstSet = 0) override;
        void bindPushConstants(const void* data, uint32_t size, uint32_t offset = 0) override;
        void bindVertexBuffers(const std::vector<Buffer*>& vertexBuffers) override;
        void bindIndexBuffer(Buffer* indexBuffer) override;
        void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0, uint32_t firstInstance = 0) override;
        void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) override;
        void pipelineBarrier(const std::vector<Texture::Barrier>& textureBarries) override;
        void copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, uint64_t size, uint64_t srcOffset = 0, uint64_t dstOffset = 0) override;
        void copyBufferToTexture(Buffer* srcBuffer, Texture* dstTexture, uint32_t width, uint32_t height, uint32_t mipLevel = 0,
                                 uint32_t arrayLayer = 0) override;

        MTL::CommandBuffer* getHandle() const { return commandBuffer; }

      private:
        MTL::CommandQueue* commandQueue = nullptr;
        MTL::CommandBuffer* commandBuffer = nullptr;
        MTL::RenderCommandEncoder* renderEncoder = nullptr;
        Buffer* currentIndexBuffer = nullptr;

        MTL::PrimitiveType currentPipelinePrimitiveType = MTL::PrimitiveTypeTriangle;
    };
} // namespace Axiom
