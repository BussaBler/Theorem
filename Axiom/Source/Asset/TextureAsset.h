#pragma once
#include "Asset.h"
#include "Renderer/Sampler.h"
#include "Renderer/Texture.h"

namespace Axiom {
    class TextureAsset : public Asset {
      public:
        TextureAsset(UUID handle, const std::string& name, std::unique_ptr<Texture> texture)
            : Asset(handle, AssetType::Texture, name), texture(std::move(texture)) {}
        ~TextureAsset() = default;

        inline Texture* getTexture() const { return texture.get(); }
        inline SamplerAddressMode getAddressMode() const { return addressMode; }
        inline SamplerFilterMode getFilterMode() const { return filterMode; }

        inline void setAddressMode(SamplerAddressMode mode) { addressMode = mode; }
        inline void setFilterMode(SamplerFilterMode mode) { filterMode = mode; }

      private:
        std::unique_ptr<Texture> texture;
        SamplerAddressMode addressMode = SamplerAddressMode::Repeat;
        SamplerFilterMode filterMode = SamplerFilterMode::Linear;
    };
} // namespace Axiom
