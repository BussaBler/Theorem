#pragma once
#include "Asset/UUID.h"
#include "Core/Base.h"

namespace Axiom {
    struct AX_COMPONENT() MeshComponent {
        AX_PROPERTY(AssetType::Mesh) UUID meshId;
    };
} // namespace Axiom