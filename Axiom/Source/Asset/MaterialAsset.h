#pragma once

#include "Asset/Asset.h"
#include "Asset/UUID.h"
#include "Math/Color.h"
#include "Math/Vec.h"
#include "Renderer/ResourceSet.h"

#include <memory>
#include <string>

namespace Axiom {
    class MaterialAsset : public Asset {
      public:
        MaterialAsset(UUID handle, const std::string& name, UUID shader, std::unique_ptr<ResourceSet> resourceSet)
            : Asset(handle, AssetType::Material, name), shader(shader), resourceSet(std::move(resourceSet)) {}

        inline void setAlbedoColor(Color color) { albedoColor = color; }
        inline void setMetallic(float value) { metallic = value; }
        inline void setRoughness(float value) { roughness = value; }
        inline void setEmission(float value) { emission = value; }
        inline void setUvTiling(Math::Vec2 tiling) { uvTiling = tiling; }
        inline void setAlbedoMap(UUID uuid) { albedoMap = uuid; }
        inline void setNormalMap(UUID uuid) { normalMap = uuid; }
        inline void setMetallicRoughnessMap(UUID uuid) { roughness = uuid; }

        inline Color getAlbedoColor() const { return albedoColor; }

        inline UUID getShader() const { return shader; }

        inline ResourceSet* getResourceSet() const { return resourceSet.get(); }

      private:
        Color albedoColor = Color::white();
        float metallic = 0.0f;
        float roughness = 0.5f;
        float emission = 0.0f;
        Math::Vec2 uvTiling = Math::Vec2::one();

        UUID albedoMap = UUID();
        UUID normalMap = UUID();
        UUID metallicRoughnessMap = UUID();

        UUID shader;
        std::unique_ptr<ResourceSet> resourceSet;
    };
} // namespace Axiom
