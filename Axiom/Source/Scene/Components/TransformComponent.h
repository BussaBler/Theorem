#pragma once
#include "Core/Base.h"
#include "Math/Mat.h"
#include "Math/Vec.h"

namespace Axiom {
    struct AX_COMPONENT() TransformComponent {
        AX_PROPERTY() Math::Vec3 position = Math::Vec3::zero();
        AX_PROPERTY() Math::Vec3 rotation = Math::Vec3::zero();
        AX_PROPERTY() Math::Vec3 scale = Math::Vec3::one();

        Math::Mat4 modelMatrix() { return Math::Mat4::model(position, rotation, scale); }
    };
} // namespace Axiom
