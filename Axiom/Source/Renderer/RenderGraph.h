#pragma once

#include "Core/Log.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/Texture.h"

#include <any>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Axiom {
    class RenderGraph;

    struct RGTextureHandle {
        uint32_t id = 0xFFFFFFFF;
        inline bool isValid() const { return id != 0xFFFFFFFF; }
    };

    struct TextureAccess {
        uint32_t textureId;
        TextureState state;
    };

    class PassBuilder {
      public:
        PassBuilder() = default;
        ~PassBuilder() = default;

        RGTextureHandle createTexture(const std::string& name, const Texture::CreateInfo& createInfo);

        void readTexture(RGTextureHandle handle, TextureState textureState);
        void writeTexture(RGTextureHandle handle, TextureState textureState);

      private:
        friend class RenderGraph;
        uint32_t currentPassIndex = 0;
        RenderGraph* renderGraph = nullptr;

        std::vector<TextureAccess> currentReads;
        std::vector<TextureAccess> currentWrites;
    };

    class PassResources {
      public:
        PassResources() = default;
        ~PassResources() = default;

        const Texture* getTexture(RGTextureHandle handle) const;

      private:
        friend class RenderGraph;
        std::unordered_map<uint32_t, Texture*> textures;
    };

    struct RenderPassNode {
        std::string name;
        std::function<void(const PassResources&, CommandBuffer*)> execute;

        std::vector<TextureAccess> textureReads;
        std::vector<TextureAccess> textureWrites;
    };

    class RenderGraphContext {
      public:
        template <typename T> void add(const T& data) { internalData[typeid(T)] = data; }

        template <typename T> const T& get() const {
            if (internalData.find(typeid(T)) == internalData.end()) {
                AX_CORE_LOG_ERROR("Current Render Graph Context has no data of type:", typeid(T).name());
            }
            return std::any_cast<const T&>(internalData.at(typeid(T)));
        }

      private:
        std::unordered_map<std::type_index, std::any> internalData;
    };

    class RenderGraph {
      public:
        RenderGraph() = default;
        ~RenderGraph() = default;

        void begin();
        template <typename PassData>
        void addPass(const std::string& name,
                     std::function<std::function<void(const PassData&, const PassResources&, CommandBuffer*)>(PassBuilder&, PassData&)> setupFn) {
            PassData passData{};
            RenderPassNode passNode = {.name = name};

            PassBuilder builder{};
            builder.renderGraph = this;
            builder.currentPassIndex = static_cast<uint32_t>(passes.size());

            auto executeFn = setupFn(builder, passData);

            if (executeFn) {
                passNode.execute = [passData, executeFn](const PassResources& resources, CommandBuffer* cmd) { executeFn(passData, resources, cmd); };
            }

            passNode.textureReads = std::move(builder.currentReads);
            passNode.textureWrites = std::move(builder.currentWrites);

            passes.push_back(std::move(passNode));
        }
        void execute(CommandBuffer* commandBuffer);

        RGTextureHandle importTexture(const std::string& name, Texture* texture);

        RenderGraphContext& getContext() { return context; }
        const RenderGraphContext& getContext() const { return context; }

      private:
        struct VirtualResource {
            std::string name;
            Texture::CreateInfo createInfo;
            Texture* texture = nullptr;
            uint32_t firstPass = 0xFFFFFFFF;
            uint32_t lastPass = 0;
            TextureState currentState = TextureState::Undefined;

            bool isExternal = false;
        };

      private:
        friend class PassBuilder;
        RenderGraphContext context;
        std::vector<RenderPassNode> passes;
        std::vector<VirtualResource> virtualResources;
        std::vector<std::unique_ptr<Texture>> texturesCache;
    };
} // namespace Axiom
