#pragma once
#include "Buffer.h"
#include "CommandBuffer.h"
#include "Core/Window.h"
#include "Pipeline.h"
#include "Renderer/ResourceLayout.h"
#include "ResourceSet.h"
#include "Sampler.h"
#include "SwapChain.h"
#include "Texture.h"

#include <memory>
#include <vector>

namespace Axiom {
    enum class GraphicsApi { Vulkan, DirectX12, Metal };

    class Device {
      public:
        struct CreateInfo {
            GraphicsApi api = GraphicsApi::Vulkan;

            std::string appName = "Axiom Application";
            std::string engineName = "Axiom Engine";

            uint32_t appVersionMajor = 1;
            uint32_t appVersionMinor = 0;
            uint32_t appVersionPatch = 0;
            uint32_t engineVersionMajor = 1;
            uint32_t engineVersionMinor = 0;
            uint32_t engineVersionPatch = 0;

            Window* windowObjPtr = nullptr;

            uint32_t maxFramesInFlight = 2;
        };

        Device() = default;
        virtual ~Device() = default;

        static std::unique_ptr<Device> create(const CreateInfo& createInfo);

        virtual std::unique_ptr<SwapChain> createSwapchain(uint32_t width, uint32_t height) = 0;
        virtual std::unique_ptr<Shader> createShader(const std::string& vertexSource, const std::string& fragmentSource) = 0;
        virtual std::unique_ptr<Pipeline> createPipeline(const Pipeline::CreateInfo& pipelineCreateInfo) = 0;
        virtual std::unique_ptr<CommandBuffer> createCommandBuffer() = 0;
        virtual std::unique_ptr<Buffer> createBuffer(const Buffer::CreateInfo& bufferCreateInfo) = 0;
        virtual std::unique_ptr<Texture> createTexture(const Texture::CreateInfo& textureCreateInfo) = 0;
        virtual std::unique_ptr<Sampler> createSampler(const Sampler::CreateInfo& samplerCreateInfo) = 0;
        virtual std::unique_ptr<ResourceLayout> createResourceLayout(const std::vector<ResourceLayout::BindingCreateInfo>& bindings) = 0;
        virtual std::unique_ptr<ResourceSet> createResourceSet(const ResourceLayout* resourceLayout) = 0;
        virtual bool beginFrame(SwapChain* swapChain) = 0;
        virtual CommandBuffer* getCurrentCommandBuffer() = 0;
        virtual std::unique_ptr<CommandBuffer> beginSingleTimeCommands() = 0;
        virtual void endSingleTimeCommands(CommandBuffer* commandBuffer) = 0;
        virtual void submitCommandBuffers(const std::vector<CommandBuffer*> commandBuffers, SwapChain* swapChain) = 0;
        virtual void waitIdle() = 0;
    };
} // namespace Axiom
