#pragma once
#include "Math/AxMath.h"
#include "Texture.h"

#include <array>

namespace Axiom {
    enum class LoadOp { Load, Clear, DontCare };

    enum class StoreOp { Store, DontCare };

    struct RenderAttachment {
        const Texture* texture = nullptr;
        LoadOp loadOp = LoadOp::DontCare;
        StoreOp storeOp = StoreOp::DontCare;
        Math::Vec4 clearColor = Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        float clearDepth = 1.0f;
        uint32_t clearStencil = 0;
    };

    class RenderPass {
      public:
        std::array<RenderAttachment, 4> colorAttachments;
        uint32_t colorAttachmentCount = 0;

        RenderAttachment depthAttachment;
        bool hasDepthAttachment = false;

        uint32_t width = 0;
        uint32_t height = 0;
    };
} // namespace Axiom
