#include "axpch.h"

#include "Renderer.h"

#include "Renderer/ForwardRenderPipeline.h"
#include "Renderer/Pipeline.h"
#include "Renderer/ResourceLayout.h"
#include "Renderer/ResourceSet.h"

#include <memory>
#include <utility>

namespace Axiom {
    Renderer::Renderer(Window* windowPtr) : window(windowPtr) {
        Device::CreateInfo deviceCreateInfo{};
        deviceCreateInfo.api = GraphicsApi::Vulkan;
        deviceCreateInfo.windowObjPtr = window;

        device = Device::create(deviceCreateInfo);
        swapChain = device->createSwapchain(window->getFramebufferWidth(), window->getFramebufferHeight());

        renderTargetBarrier = {.oldState = TextureState::Undefined, .newState = TextureState::RenderTarget, .aspect = TextureAspect::Color};
        presentBarrier = {.oldState = TextureState::RenderTarget, .newState = TextureState::Present, .aspect = TextureAspect::Color};
        depthBarrier = {.oldState = TextureState::Undefined, .newState = TextureState::DepthStencilTarget, .aspect = TextureAspect::Depth};

        createDefaultSamplers();
    }

    Renderer::~Renderer() {
        device->waitIdle();
    }

    void Renderer::initPipelines() {
        forwardRP = std::make_unique<ForwardRenderPipeline>();
    }

    void Renderer::waitIdle() {
        device->waitIdle();
    }

    CommandBuffer* Renderer::beginFrame() {
        if (!device->beginFrame(swapChain.get())) {
            recreateSwapChain();
            return nullptr;
        }

        CommandBuffer* commandBuffer = device->getCurrentCommandBuffer();
        commandBuffer->begin();

        renderTargetBarrier.texture = swapChain->getCurrentTexture();
        // commandBuffer->pipelineBarrier({renderTargetBarrier});
        depthBarrier.texture = swapChain->getCurrentDepthTexture();
        // commandBuffer->pipelineBarrier({depthBarrier});
        return commandBuffer;
    }

    void Renderer::endFrame() {
        CommandBuffer* commandBuffer = device->getCurrentCommandBuffer();
        presentBarrier.texture = renderTargetBarrier.texture;
        // commandBuffer->pipelineBarrier({presentBarrier});
        commandBuffer->end();

        device->submitCommandBuffers({commandBuffer}, swapChain.get());

        if (!swapChain->present()) {
            recreateSwapChain();
        }
    }

    std::unique_ptr<Shader> Renderer::createShader(const std::string& vertexSource, const std::string& fragmentSource) {
        return device->createShader(vertexSource, fragmentSource);
    }

    std::unique_ptr<Pipeline> Renderer::createPipeline(const Pipeline::CreateInfo& pipelineCreateInfo) {
        return device->createPipeline(pipelineCreateInfo);
    }

    std::unique_ptr<Buffer> Renderer::createBuffer(const Buffer::CreateInfo& bufferCreateInfo) {
        return device->createBuffer(bufferCreateInfo);
    }

    std::unique_ptr<Texture> Renderer::createTexture(const Texture::CreateInfo& textureCreateInfo) {
        return device->createTexture(textureCreateInfo);
    }

    std::unique_ptr<Sampler> Renderer::createSampler(const Sampler::CreateInfo& samplerCreateInfo) {
        return device->createSampler(samplerCreateInfo);
    }

    std::unique_ptr<ResourceLayout> Renderer::createResourceLayout(const std::vector<ResourceLayout::BindingCreateInfo>& bindings) {
        return device->createResourceLayout(bindings);
    }

    std::unique_ptr<ResourceSet> Renderer::createResourceSet(const ResourceLayout* resourceLayout) {
        return device->createResourceSet(resourceLayout);
    }

    std::unique_ptr<CommandBuffer> Renderer::beginSingleTimeCommands() {
        return device->beginSingleTimeCommands();
    }

    void Renderer::endSingleTimeCommands(CommandBuffer* commandBuffer) {
        device->endSingleTimeCommands(commandBuffer);
    }

    Pipeline* Renderer::getOrCreatePipeline(const Pipeline::CreateInfo& pipelineCreateInfo) {
        if (pipelineCache.find(pipelineCreateInfo) != pipelineCache.end()) {
            return pipelineCache[pipelineCreateInfo].get();
        }

        std::unique_ptr<Pipeline> newPipeline = createPipeline(pipelineCreateInfo);
        Pipeline* pNewPipeline = newPipeline.get();
        pipelineCache[pipelineCreateInfo] = std::move(newPipeline);

        return pNewPipeline;
    }

    void Renderer::recreateSwapChain() {
        uint32_t width = window->getFramebufferWidth();
        uint32_t height = window->getFramebufferHeight();

        device->waitIdle();
        swapChain.reset();
        swapChain = device->createSwapchain(width, height);
    }

    void Renderer::createDefaultSamplers() {
        Sampler::CreateInfo linearSamplerCreateInfo = {
            .adressMode = SamplerAddressMode::Repeat, .filterMode = SamplerFilterMode::Linear, .mipmapFilterMode = SamplerFilterMode::Linear};
        linearSampler = device->createSampler(linearSamplerCreateInfo);

        Sampler::CreateInfo nearestSamplerCreateInfo = {
            .adressMode = SamplerAddressMode::Repeat, .filterMode = SamplerFilterMode::Nearest, .mipmapFilterMode = SamplerFilterMode::Nearest};
        nearestSampler = device->createSampler(nearestSamplerCreateInfo);
    }
} // namespace Axiom
