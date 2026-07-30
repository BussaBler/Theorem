#pragma once
#include "Buffer.h"
#include "Math/AxMath.h"
#include "Utils/BitMaskEnum.h"

namespace Axiom {
    enum class Format {
        Undefined,
        B8G8R8A8Unorm,
        R8Unorm,
        R8G8Unorm,
        R8G8B8Unorm,
        R8G8B8A8Unorm,
        B8G8R8A8Srgb,
        R8Srgb,
        R8G8Srgb,
        R8G8B8Srgb,
        R8G8B8A8Srgb,
        D24UnormS8Uint,
        D32sFloat,
        R32G32Sfloat,
        R32G32B32Sfloat,
        R32G32B32A32Sfloat
    };

    enum class TextureType {
        Texture1D,
        Texture1DArray,
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCube,
    };

    enum class TextureUsage {
        None = 0,
        TransferSrc = 1 << 0,
        TransferDst = 1 << 1,
        Sampled = 1 << 2,
        Storage = 1 << 3,
        ColorAttachment = 1 << 4,
        DepthStencilAttachment = 1 << 5,
    };

    enum class TextureAspect { None = 0, Color = 1 << 0, Depth = 1 << 1, Stencil = 1 << 2 };

    template <> struct EnableBitMaskOperators<TextureUsage> : std::true_type {};
    template <> struct EnableBitMaskOperators<TextureAspect> : std::true_type {};

    enum class TextureState { Undefined, RenderTarget, DepthStencilTarget, ShaderResource, TransferDst, TransferSrc, Present };

    class Texture {
      public:
        struct CreateInfo {
            uint32_t width = 0;
            uint32_t height = 0;
            uint32_t mipLevels = 1;
            uint32_t arrayLayers = 1;
            Format format = Format::Undefined;
            TextureUsage usage = TextureUsage::None;
            TextureAspect aspect = TextureAspect::Color;
            TextureState initialState = TextureState::Undefined;
            MemoryUsage memoryUsage = MemoryUsage::GPUOnly;
            TextureType type = TextureType::Texture2D;

            bool operator==(const CreateInfo& other) {
                return width == other.width && height == other.height && mipLevels == other.mipLevels && arrayLayers == other.arrayLayers &&
                       format == other.format && usage == other.usage && aspect == other.aspect && memoryUsage == other.memoryUsage && type == other.type;
            }
        };

        struct Barrier {
            Texture* texture;
            TextureState oldState = TextureState::Undefined;
            TextureState newState = TextureState::Undefined;
            TextureAspect aspect = TextureAspect::None;

            uint32_t baseMipLevel = 0;
            uint32_t mipLevelCount = 1;
            uint32_t baseArrayLayer = 0;
            uint32_t arrayLayerCount = 1;
        };

      public:
        Texture(const CreateInfo& createInfo) : createInfo(createInfo) {}
        virtual ~Texture() = default;

        const CreateInfo& getCreateInfo() { return createInfo; }
        TextureState getCurrentState() { return currentState; }
        void setCurrentState(TextureState newState) { currentState = newState; }

        virtual Format getFormat() const = 0;
        virtual Math::iVec2 getSize() const = 0;
        virtual uint32_t getMipLevels() const = 0;
        virtual uint32_t getArrayLayers() const = 0;

      protected:
        CreateInfo createInfo;
        TextureState currentState = TextureState::Undefined;
    };
} // namespace Axiom
