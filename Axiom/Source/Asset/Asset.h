#pragma once
#include "UUID.h"

#include <string>

namespace Axiom {
    enum class AssetType { None, Texture, Mesh, Font, Audio, Shader };

    class Asset {
      public:
        Asset(UUID handle, AssetType type, const std::string& name) : handle(handle), type(type), name(name) {}
        virtual ~Asset() = default;

        inline UUID getHandle() const { return handle; }
        inline AssetType getType() const { return type; }
        inline const std::string& getName() const { return name; }

      private:
        UUID handle = 0;
        AssetType type = AssetType::None;
        const std::string name;
    };
} // namespace Axiom
