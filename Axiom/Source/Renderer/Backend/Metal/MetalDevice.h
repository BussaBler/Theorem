#pragma once
#include "MetalBuffer.h"
#include "MetalCommandBuffer.h"
#include "MetalPipeline.h"
#include "MetalResourceLayout.h"
#include "MetalResourceSet.h"
#include "MetalSampler.h"
#include "MetalShader.h"
#include "MetalSwapChain.h"
#include "MetalTexture.h"
#include "MetalUtils.h"
#include "Renderer/Device.h"

#include <memory>

namespace Axiom {
    class MetalDevice : public Device {
      public:
        MetalDevice(const CreateInfo& createInfo);
        ~MetalDevice() override;

        std::unique_ptr<SwapChain> createSwapchain(uint32_t width, uint32_t height) override;
        std::unique_ptr<Shader> createShader(const std::string& vertexSource, const std::string& fragmentSource) override;
        std::unique_ptr<Pipeline> createPipeline(const Pipeline::CreateInfo& pipelineCreateInfo) override;
        std::unique_ptr<CommandBuffer> createCommandBuffer() override;
        std::unique_ptr<Buffer> createBuffer(const Buffer::CreateInfo& bufferCreateInfo) override;
        std::unique_ptr<Texture> createTexture(const Texture::CreateInfo& textureCreateInfo) override;
        std::unique_ptr<Sampler> createSampler(const Sampler::CreateInfo& samplerCreateInfo) override;
        std::unique_ptr<ResourceLayout> createResourceLayout(const std::vector<ResourceLayout::BindingCreateInfo>& bindings) override;
        std::unique_ptr<ResourceSet> createResourceSet(const ResourceLayout* resourceLayout) override;
        bool beginFrame(SwapChain* swapChain) override;
        CommandBuffer* getCurrentCommandBuffer() override;
        std::unique_ptr<CommandBuffer> beginSingleTimeCommands() override;
        void endSingleTimeCommands(CommandBuffer* commandBuffer) override;
        void submitCommandBuffers(const std::vector<CommandBuffer*> commandBuffers, SwapChain* swapChain) override;
        void waitIdle() override;

      private:
        MTL::Device* metalDevice = nullptr;
        CA::MetalLayer* metalLayer = nullptr;
        MTL::CommandQueue* commandQueue = nullptr;
        uint32_t maxFramesInFlight = 0;
        uint32_t currentFrameIndex = 0;
        std::vector<std::unique_ptr<MetalCommandBuffer>> commandBuffers;
    };
} // namespace Axiom
