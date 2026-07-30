#pragma once
#include "MetalUtils.h"
#include "Renderer/ResourceSet.h"

namespace Axiom {
    class MetalResourceSet : public ResourceSet {
      public:
        MetalResourceSet(const ResourceLayout* layout, MTL::Device* device);
        ~MetalResourceSet() override;

        void update(const std::vector<Binding>& bindings) override;

        MTL::Buffer* getArgumentBuffer() const { return argumentBuffer; }
        std::vector<MTL::Buffer*>& getResidentBuffers() { return residentBuffers; }
        std::vector<MTL::Texture*>& getResidentTextures() { return residentTextures; }

      private:
        MTL::Buffer* argumentBuffer = nullptr;
        MTL::ArgumentEncoder* argumentEncoder = nullptr;

        std::vector<MTL::Buffer*> residentBuffers;
        std::vector<MTL::Texture*> residentTextures;
    };
} // namespace Axiom
