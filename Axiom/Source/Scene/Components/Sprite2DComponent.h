#pragma once
#include "Asset/AssetManager.h"
#include "Asset/UUID.h"
#include "Core/Base.h"
#include "Math/Color.h"
#include "Renderer/Sampler.h"

namespace Axiom {
    struct AX_COMPONENT() Sprite2DComponent {
        AX_PROPERTY(AssetType::Texture) UUID textureId = AssetManager::getDefaultTextureHandle();
        AX_PROPERTY() SamplerAddressMode addressMode = SamplerAddressMode::Repeat;
        AX_PROPERTY() SamplerFilterMode filterMode = SamplerFilterMode::Linear;
        AX_PROPERTY() Color color = Color::white();
    };
} // namespace Axiom
