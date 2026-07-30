#include "axpch.h"

#include "VulkanCommandBuffer.h"

namespace Axiom {
    VulkanCommandBuffer::VulkanCommandBuffer(Vk::Device logicDevice, Vk::CommandPool commandPool) : device(logicDevice), commandPool(commandPool) {
        Vk::CommandBufferAllocateInfo allocInfo(commandPool, Vk::CommandBufferLevel::ePrimary, 1);
        Vk::ResultValue<std::vector<Vk::CommandBuffer>> commandBufferResult = device.allocateCommandBuffers(allocInfo);

        AX_CORE_ASSERT(commandBufferResult.result == Vk::Result::eSuccess, "Failed to allocate Vulkan command buffer: {}",
                       Vk::to_string(commandBufferResult.result));
        commandBuffer = commandBufferResult.value[0];
    }

    VulkanCommandBuffer::~VulkanCommandBuffer() {
        if (commandBuffer) {
            device.freeCommandBuffers(commandPool, commandBuffer);
        }
    }

    void VulkanCommandBuffer::begin() {
        Vk::CommandBufferBeginInfo beginInfo(Vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        AX_CORE_ASSERT(commandBuffer.begin(beginInfo) == Vk::Result::eSuccess, "Failed to begin recording Vulkan command buffer");
    }

    void VulkanCommandBuffer::end() {
        AX_CORE_ASSERT(commandBuffer.end() == Vk::Result::eSuccess, "Failed to end recording Vulkan command buffer");
    }

    void VulkanCommandBuffer::beginRendering(const RenderPass& renderPass) {
        std::vector<Vk::RenderingAttachmentInfo> colorAttachments;
        colorAttachments.reserve(renderPass.colorAttachmentCount);

        for (size_t i = 0; i < renderPass.colorAttachmentCount; i++) {
            const auto& attachment = renderPass.colorAttachments[i];
            VulkanTexture* vulkanTexture = static_cast<VulkanTexture*>(attachment.texture);
            Vk::ClearValue clearValue{};
            clearValue.setColor(Vk::ClearColorValue(
                std::array<float, 4>{attachment.clearColor.x(), attachment.clearColor.y(), attachment.clearColor.z(), attachment.clearColor.w()}));
            Vk::RenderingAttachmentInfo attachmentInfo{};
            attachmentInfo.setImageView(vulkanTexture->getImageView());
            attachmentInfo.setImageLayout(Vk::ImageLayout::eColorAttachmentOptimal);
            attachmentInfo.setLoadOp(axToVkLoadOp(attachment.loadOp));
            attachmentInfo.setStoreOp(axToVkStoreOp(attachment.storeOp));
            attachmentInfo.setClearValue(clearValue);

            colorAttachments.push_back(attachmentInfo);
        }

        Vk::RenderingAttachmentInfo depthAttachmentInfo{};
        if (renderPass.hasDepthAttachment) {
            VulkanTexture* vulkanTexture = static_cast<VulkanTexture*>(renderPass.depthAttachment.texture);
            Vk::ClearValue clearValue{};
            clearValue.setDepthStencil(Vk::ClearDepthStencilValue{renderPass.depthAttachment.clearDepth, renderPass.depthAttachment.clearStencil});
            depthAttachmentInfo.setImageView(vulkanTexture->getImageView());
            depthAttachmentInfo.setImageLayout(Vk::ImageLayout::eDepthStencilAttachmentOptimal);
            depthAttachmentInfo.setLoadOp(axToVkLoadOp(renderPass.depthAttachment.loadOp));
            depthAttachmentInfo.setStoreOp(axToVkStoreOp(renderPass.depthAttachment.storeOp));
            depthAttachmentInfo.setClearValue(clearValue);
        }

        Vk::Rect2D renderArea{{0, 0}, {renderPass.width, renderPass.height}};
        Vk::RenderingInfo renderingInfo{};
        renderingInfo.setRenderArea(renderArea);
        renderingInfo.setLayerCount(1);
        renderingInfo.setColorAttachments(colorAttachments);
        if (renderPass.hasDepthAttachment) {
            renderingInfo.setPDepthAttachment(&depthAttachmentInfo);
        }

        commandBuffer.beginRendering(renderingInfo);
    }

    void VulkanCommandBuffer::endRendering() {
        commandBuffer.endRendering();
    }

    void VulkanCommandBuffer::bindPipeline(Pipeline* pipeline) {
        Vk::Pipeline vkPipeline = static_cast<VulkanPipeline*>(pipeline)->getHandle();
        currentPipelineLayout = static_cast<VulkanPipeline*>(pipeline)->getPipelineLayout();

        commandBuffer.bindPipeline(Vk::PipelineBindPoint::eGraphics, vkPipeline);
    }

    void VulkanCommandBuffer::setViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
        Vk::Viewport viewport(x, y + height, width, -height, minDepth, maxDepth);
        commandBuffer.setViewport(0, viewport);
    }

    void VulkanCommandBuffer::setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) {
        Vk::Rect2D scissor({x, y}, {width, height});
        commandBuffer.setScissor(0, scissor);
    }

    void VulkanCommandBuffer::bindResources(const std::vector<ResourceSet*>& resourceSets, uint32_t firstSet) {
        std::vector<Vk::DescriptorSet> vkDescriptorSets(resourceSets.size());

        for (size_t i = 0; i < resourceSets.size(); i++) {
            vkDescriptorSets[i] = static_cast<VulkanResourceSet*>(resourceSets[i])->getHandle();
        }
        commandBuffer.bindDescriptorSets(Vk::PipelineBindPoint::eGraphics, currentPipelineLayout, firstSet, vkDescriptorSets, {});
    }

    void VulkanCommandBuffer::bindPushConstants(const void* data, uint32_t size, uint32_t offset) {
        commandBuffer.pushConstants(currentPipelineLayout, Vk::ShaderStageFlagBits::eVertex | Vk::ShaderStageFlagBits::eFragment, offset, size, data);
    }

    void VulkanCommandBuffer::bindVertexBuffers(const std::vector<Buffer*>& vertexBuffers) {
        std::vector<Vk::Buffer> vkBuffers(vertexBuffers.size());
        std::vector<Vk::DeviceSize> offsets(vertexBuffers.size(), 0);

        for (size_t i = 0; i < vertexBuffers.size(); i++) {
            vkBuffers[i] = static_cast<VulkanBuffer*>(vertexBuffers[i])->getHandle();
        }
        commandBuffer.bindVertexBuffers(0, vkBuffers, offsets);
    }

    void VulkanCommandBuffer::bindIndexBuffer(Buffer* indexBuffer) {
        Vk::Buffer vkBuffer = static_cast<VulkanBuffer*>(indexBuffer)->getHandle();
        commandBuffer.bindIndexBuffer(vkBuffer, 0, Vk::IndexType::eUint32); // TODO: support uint16 indices
    }

    void VulkanCommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
        commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
        commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanCommandBuffer::pipelineBarrier(const std::vector<Texture::Barrier>& textureBarries) {
        std::vector<Vk::ImageMemoryBarrier2> imageBarriers(textureBarries.size());

        for (size_t i = 0; i < textureBarries.size(); i++) {
            const auto& barrier = textureBarries[i];
            VulkanTexture* vulkanTexture = static_cast<VulkanTexture*>(barrier.texture);
            imageBarriers[i].setImage(vulkanTexture->getImage());
            imageBarriers[i].setSrcQueueFamilyIndex(Vk::QueueFamilyIgnored);
            imageBarriers[i].setDstQueueFamilyIndex(Vk::QueueFamilyIgnored);
            imageBarriers[i].setSubresourceRange({axToVkImageAspectFlags(barrier.aspect), 0, barrier.mipLevelCount, 0, barrier.arrayLayerCount});

            switch (barrier.oldState) {
            case TextureState::Undefined:
                imageBarriers[i].setOldLayout(Vk::ImageLayout::eUndefined);
                imageBarriers[i].setSrcAccessMask({});
                imageBarriers[i].setSrcStageMask(Vk::PipelineStageFlagBits2::eTopOfPipe);
                break;
            case TextureState::RenderTarget:
                imageBarriers[i].setOldLayout(Vk::ImageLayout::eColorAttachmentOptimal);
                imageBarriers[i].setSrcAccessMask(Vk::AccessFlagBits2::eColorAttachmentWrite);
                imageBarriers[i].setSrcStageMask(Vk::PipelineStageFlagBits2::eColorAttachmentOutput);
                break;
            case TextureState::DepthStencilTarget:
                imageBarriers[i].setOldLayout(Vk::ImageLayout::eDepthStencilAttachmentOptimal);
                imageBarriers[i].setSrcAccessMask(Vk::AccessFlagBits2::eDepthStencilAttachmentWrite);
                imageBarriers[i].setSrcStageMask(Vk::PipelineStageFlagBits2::eEarlyFragmentTests);
                break;
            case TextureState::ShaderResource:
                imageBarriers[i].setOldLayout(Vk::ImageLayout::eShaderReadOnlyOptimal);
                imageBarriers[i].setSrcAccessMask(Vk::AccessFlagBits2::eShaderRead);
                imageBarriers[i].setSrcStageMask(Vk::PipelineStageFlagBits2::eFragmentShader);
                break;
            case TextureState::TransferDst:
                imageBarriers[i].setOldLayout(Vk::ImageLayout::eTransferDstOptimal);
                imageBarriers[i].setSrcAccessMask(Vk::AccessFlagBits2::eTransferWrite);
                imageBarriers[i].setSrcStageMask(Vk::PipelineStageFlagBits2::eTransfer);
                break;
            case TextureState::TransferSrc:
                imageBarriers[i].setOldLayout(Vk::ImageLayout::eTransferSrcOptimal);
                imageBarriers[i].setSrcAccessMask(Vk::AccessFlagBits2::eTransferRead);
                imageBarriers[i].setSrcStageMask(Vk::PipelineStageFlagBits2::eTransfer);
                break;
            default:
                break;
            }

            switch (barrier.newState) {
            case TextureState::RenderTarget:
                imageBarriers[i].setNewLayout(Vk::ImageLayout::eColorAttachmentOptimal);
                imageBarriers[i].setDstAccessMask(Vk::AccessFlagBits2::eColorAttachmentWrite | Vk::AccessFlagBits2::eColorAttachmentRead);
                imageBarriers[i].setDstStageMask(Vk::PipelineStageFlagBits2::eColorAttachmentOutput);
                break;

            case TextureState::DepthStencilTarget:
                imageBarriers[i].setNewLayout(Vk::ImageLayout::eDepthStencilAttachmentOptimal);
                imageBarriers[i].setDstAccessMask(Vk::AccessFlagBits2::eDepthStencilAttachmentWrite | Vk::AccessFlagBits2::eDepthStencilAttachmentRead);
                imageBarriers[i].setDstStageMask(Vk::PipelineStageFlagBits2::eEarlyFragmentTests | Vk::PipelineStageFlagBits2::eLateFragmentTests);
                break;

            case TextureState::ShaderResource:
                imageBarriers[i].setNewLayout(Vk::ImageLayout::eShaderReadOnlyOptimal);
                imageBarriers[i].setDstAccessMask(Vk::AccessFlagBits2::eShaderRead);
                // If needed to read textures in the vertex shader might need to OR this with eVertexShader
                imageBarriers[i].setDstStageMask(Vk::PipelineStageFlagBits2::eFragmentShader);
                break;

            case TextureState::TransferDst:
                imageBarriers[i].setNewLayout(Vk::ImageLayout::eTransferDstOptimal);
                imageBarriers[i].setDstAccessMask(Vk::AccessFlagBits2::eTransferWrite);
                imageBarriers[i].setDstStageMask(Vk::PipelineStageFlagBits2::eTransfer);
                break;

            case TextureState::TransferSrc:
                imageBarriers[i].setNewLayout(Vk::ImageLayout::eTransferSrcOptimal);
                imageBarriers[i].setDstAccessMask(Vk::AccessFlagBits2::eTransferRead);
                imageBarriers[i].setDstStageMask(Vk::PipelineStageFlagBits2::eTransfer);
                break;

            case TextureState::Present:
                imageBarriers[i].setNewLayout(Vk::ImageLayout::ePresentSrcKHR);
                imageBarriers[i].setDstAccessMask({});
                imageBarriers[i].setDstStageMask(Vk::PipelineStageFlagBits2::eBottomOfPipe);
                break;

            default:
                break;
            }

            barrier.texture->setCurrentState(barrier.newState);
        }

        Vk::DependencyInfo dependencyInfo{};
        dependencyInfo.setImageMemoryBarriers(imageBarriers);

        commandBuffer.pipelineBarrier2(dependencyInfo);
    }

    void VulkanCommandBuffer::copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, uint64_t size, uint64_t srcOffset, uint64_t dstOffset) {
        AX_CORE_ASSERT(size + dstOffset <= dstBuffer->getSize(), "Copy will not fit in the destination buffer");

        Vk::Buffer vkSrcBuffer = static_cast<VulkanBuffer*>(srcBuffer)->getHandle();
        Vk::Buffer vkDstBuffer = static_cast<VulkanBuffer*>(dstBuffer)->getHandle();

        Vk::BufferCopy copyRegion(srcOffset, dstOffset, size);

        commandBuffer.copyBuffer(vkSrcBuffer, vkDstBuffer, copyRegion);
    }

    void VulkanCommandBuffer::copyBufferToTexture(Buffer* srcBuffer, Texture* dstTexture, uint32_t width, uint32_t height, uint32_t mipLevel,
                                                  uint32_t arrayLayer) {
        Texture::Barrier transferBarrier = {.texture = dstTexture,
                                            .oldState = TextureState::Undefined,
                                            .newState = TextureState::TransferDst,
                                            .baseMipLevel = mipLevel,
                                            .mipLevelCount = 1,
                                            .baseArrayLayer = arrayLayer,
                                            .arrayLayerCount = 1};
        pipelineBarrier({transferBarrier});

        Vk::BufferImageCopy copyRegion{};
        copyRegion.setBufferOffset(0);

        copyRegion.setBufferRowLength(0);
        copyRegion.setBufferImageHeight(0);

        copyRegion.setImageSubresource({axToVkImageAspectFlags(transferBarrier.aspect), mipLevel, arrayLayer, 1});

        copyRegion.setImageOffset({0, 0, 0});
        copyRegion.setImageExtent({width, height, 1});

        commandBuffer.copyBufferToImage(static_cast<VulkanBuffer*>(srcBuffer)->getHandle(), static_cast<VulkanTexture*>(dstTexture)->getImage(),
                                        Vk::ImageLayout::eTransferDstOptimal, copyRegion);

        Texture::Barrier shaderReadBarrier = transferBarrier;
        shaderReadBarrier.oldState = TextureState::TransferDst;
        shaderReadBarrier.newState = TextureState::ShaderResource;
        pipelineBarrier({shaderReadBarrier});
    }
} // namespace Axiom
