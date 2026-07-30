#include "axpch.h"

#include "MetalCommandBuffer.h"

#include "MetalBuffer.h"
#include "MetalPipeline.h"
#include "MetalResourceSet.h"
#include "MetalTexture.h"

#include <cstdint>

namespace Axiom {
    MetalCommandBuffer::MetalCommandBuffer(MTL::CommandQueue* commandQueue) : commandQueue(commandQueue) {
    }

    MetalCommandBuffer::~MetalCommandBuffer() {
    }

    void MetalCommandBuffer::begin() {
        commandBuffer = commandQueue->commandBuffer();
        currentIndexBuffer = nullptr;
    }

    void MetalCommandBuffer::end() {
        // nothing to fo here
    }

    void MetalCommandBuffer::beginRendering(const RenderPass& renderPass) {
        MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
        for (uint32_t i = 0; i < renderPass.colorAttachmentCount; i++) {
            const RenderAttachment& colorAttachment = renderPass.colorAttachments[i];
            MTL::RenderPassColorAttachmentDescriptor* colorAttachmentDescriptor = renderPassDescriptor->colorAttachments()->object(i);
            colorAttachmentDescriptor->setTexture(static_cast<const MetalTexture*>(colorAttachment.texture)->getHandle());
            colorAttachmentDescriptor->setLoadAction(axToMetalLoadAction(colorAttachment.loadOp));
            colorAttachmentDescriptor->setStoreAction(axToMetalStoreAction(colorAttachment.storeOp));
            colorAttachmentDescriptor->setClearColor(MTL::ClearColor{colorAttachment.clearColor.x(), colorAttachment.clearColor.y(),
                                                                     colorAttachment.clearColor.z(), colorAttachment.clearColor.w()});
        }

        if (renderPass.hasDepthAttachment) {
            const RenderAttachment& depthAttachment = renderPass.depthAttachment;
            MTL::RenderPassDepthAttachmentDescriptor* depthAttachmentDescriptor = renderPassDescriptor->depthAttachment();
            depthAttachmentDescriptor->setTexture(static_cast<const MetalTexture*>(depthAttachment.texture)->getHandle());
            depthAttachmentDescriptor->setLoadAction(axToMetalLoadAction(depthAttachment.loadOp));
            depthAttachmentDescriptor->setStoreAction(axToMetalStoreAction(depthAttachment.storeOp));
            depthAttachmentDescriptor->setClearDepth(depthAttachment.clearDepth);
        }

        renderEncoder = commandBuffer->renderCommandEncoder(renderPassDescriptor);
        renderPassDescriptor->release();
    }

    void MetalCommandBuffer::endRendering() {
        renderEncoder->endEncoding();
        renderEncoder = nullptr;
    }

    void MetalCommandBuffer::bindPipeline(Pipeline* pipeline) {
        MetalPipeline* metalPipeline = static_cast<MetalPipeline*>(pipeline);
        renderEncoder->setRenderPipelineState(metalPipeline->getHandle());
        renderEncoder->setFrontFacingWinding(metalPipeline->getFaceWinding());
        renderEncoder->setCullMode(metalPipeline->getCullMode());
        renderEncoder->setDepthStencilState(metalPipeline->getDepthStencilState());
        renderEncoder->setTriangleFillMode(metalPipeline->getFillMode());
        currentPipelinePrimitiveType = metalPipeline->getPrimitiveType();
    }

    void MetalCommandBuffer::setViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
        renderEncoder->setViewport(MTL::Viewport(x, y, width, height, minDepth, maxDepth));
    }

    void MetalCommandBuffer::setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) {
        renderEncoder->setScissorRect(MTL::ScissorRect(x, y, width, height));
    }

    void MetalCommandBuffer::bindResources(const std::vector<ResourceSet*>& resourceSets, uint32_t firstSet) {
        for (size_t i = 0; i < resourceSets.size(); i++) {
            MetalResourceSet* metalResourceSet = static_cast<MetalResourceSet*>(resourceSets[i]);
            uint32_t pushConstantOffset = 8;

            MTL::Buffer* argBuffer = metalResourceSet->getArgumentBuffer();
            renderEncoder->setVertexBuffer(argBuffer, 0, firstSet + i + pushConstantOffset);
            renderEncoder->setFragmentBuffer(argBuffer, 0, firstSet + i + pushConstantOffset);

            const auto& textures = metalResourceSet->getResidentTextures();
            if (!textures.empty()) {
                renderEncoder->useResources(reinterpret_cast<const MTL::Resource* const*>(textures.data()), textures.size(), MTL::ResourceUsageRead,
                                            MTL::RenderStageVertex | MTL::RenderStageFragment);
            }

            const auto& buffers = metalResourceSet->getResidentBuffers();
            if (!buffers.empty()) {
                renderEncoder->useResources(reinterpret_cast<const MTL::Resource* const*>(buffers.data()), buffers.size(), MTL::ResourceUsageRead,
                                            MTL::RenderStageVertex | MTL::RenderStageFragment);
            }
        }
    }

    void MetalCommandBuffer::bindPushConstants(const void* data, uint32_t size, uint32_t offset) {
        // push constanst should be in the buffer 4 position
        uint32_t pushConstantIndex = 4;
        const uint8_t* pushConstantData = static_cast<const uint8_t*>(data) + offset;

        renderEncoder->setVertexBytes(pushConstantData, size, pushConstantIndex);
        renderEncoder->setFragmentBytes(pushConstantData, size, pushConstantIndex);
    }

    void MetalCommandBuffer::bindVertexBuffers(const std::vector<Buffer*>& vertexBuffers) {
        uint32_t vertexBufferOffset = 0; // index 0 to 3 are for vertex buffers
        for (size_t i = 0; i < vertexBuffers.size(); i++) {
            Buffer* vertexBuffer = vertexBuffers[i];
            renderEncoder->setVertexBuffer(static_cast<MetalBuffer*>(vertexBuffer)->getHandle(), 0, i + vertexBufferOffset);
        }
    }

    void MetalCommandBuffer::bindIndexBuffer(Buffer* indexBuffer) {
        currentIndexBuffer = indexBuffer;
    }

    void MetalCommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
        renderEncoder->drawPrimitives(currentPipelinePrimitiveType, firstVertex, vertexCount, instanceCount, firstInstance);
    }

    void MetalCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
        renderEncoder->drawIndexedPrimitives(currentPipelinePrimitiveType, indexCount, MTL::IndexTypeUInt32,
                                             static_cast<MetalBuffer*>(currentIndexBuffer)->getHandle(), firstIndex * sizeof(uint32_t), instanceCount,
                                             vertexOffset, firstInstance);
    }

    void MetalCommandBuffer::pipelineBarrier(const std::vector<Texture::Barrier>& textureBarries) {
        // this can be left empty for Metal
    }

    void MetalCommandBuffer::copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, uint64_t size, uint64_t srcOffset, uint64_t dstOffset) {
        MTL::BlitCommandEncoder* blitEncoder = commandBuffer->blitCommandEncoder();
        blitEncoder->copyFromBuffer(static_cast<MetalBuffer*>(srcBuffer)->getHandle(), srcOffset, static_cast<MetalBuffer*>(dstBuffer)->getHandle(), dstOffset,
                                    size);
        blitEncoder->endEncoding();
    }

    void MetalCommandBuffer::copyBufferToTexture(Buffer* srcBuffer, Texture* dstTexture, uint32_t width, uint32_t height, uint32_t mipLevel,
                                                 uint32_t arrayLayer) {
        MetalBuffer* source = static_cast<MetalBuffer*>(srcBuffer);
        MetalTexture* dst = static_cast<MetalTexture*>(dstTexture);

        MTL::BlitCommandEncoder* blitEncoder = commandBuffer->blitCommandEncoder();

        MTL::Origin origin = {0, 0, 0};
        MTL::Size sourceSize = {width, height, 1};

        uint32_t bytesPerPixel = getStride(dst->getFormat());
        NS::UInteger bytesPerRow = width * bytesPerPixel;
        NS::UInteger bytesPerImage = bytesPerRow * height;

        blitEncoder->copyFromBuffer(source->getHandle(), 0, bytesPerRow, bytesPerImage, sourceSize, dst->getHandle(), arrayLayer, mipLevel, origin);

        blitEncoder->endEncoding();
    }
} // namespace Axiom
