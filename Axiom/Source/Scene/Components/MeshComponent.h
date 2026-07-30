#pragma once
#include "Asset/UUID.h"
#include "Core/Base.h"

namespace Axiom {
    struct AX_COMPONENT() MeshComponent {
        AX_PROPERTY(AssetType::Mesh) UUID meshId;
        AX_PROPERTY(AssetType::Material) UUID meterialId;
    };
} // namespace Axiom
