#pragma once

#include "Renderer/Buffer.h"
#include "Renderer/RenderPass.h"
#include "Renderer/Sampler.h"
#include "Renderer/Texture.h"

#include <Metal/Metal.hpp>

namespace Axiom {
    inline MTL::VertexFormat axToMetalVertexFormat(Format format) {
        switch (format) {
        case Format::Undefined:
            return MTL::VertexFormatInvalid;
        case Format::B8G8R8A8Unorm:
            return MTL::VertexFormatUChar4Normalized_BGRA;
        case Format::R8Unorm:
            return MTL::VertexFormatUChar;
        case Format::R8G8Unorm:
            return MTL::VertexFormatUChar2Normalized;
        case Format::R8G8B8Unorm:
            return MTL::VertexFormatUChar3Normalized;
        case Format::R8G8B8A8Unorm:
            return MTL::VertexFormatUChar4Normalized;
        case Format::B8G8R8A8Srgb:
            return MTL::VertexFormatUChar4Normalized_BGRA; // Metal doesn't have separate sRGB vertex formats
        case Format::R8Srgb:
            return MTL::VertexFormatUChar; // Metal doesn't have separate sRGB vertex formats
        case Format::R8G8Srgb:
            return MTL::VertexFormatUChar2Normalized; // Metal doesn't have separate sRGB vertex formats
        case Format::R8G8B8Srgb:
            return MTL::VertexFormatUChar3Normalized; // Metal doesn't have separate sRGB vertex formats
        case Format::R8G8B8A8Srgb:
            return MTL::VertexFormatUChar4Normalized; // Metal doesn't have separate sRGB vertex formats
        case Format::D24UnormS8Uint:
            return MTL::VertexFormatInvalid; // Depth-stencil formats are not used for vertex attributes
        case Format::D32sFloat:
            return MTL::VertexFormatInvalid; // Depth formats are not used for vertex attributes
        case Format::R32G32Sfloat:
            return MTL::VertexFormatFloat2;
        case Format::R32G32B32Sfloat:
            return MTL::VertexFormatFloat3;
        case Format::R32G32B32A32Sfloat:
            return MTL::VertexFormatFloat4;
        default:
            return MTL::VertexFormatInvalid;
        }
    }

    inline MTL::PixelFormat axToMetalPixelFormat(Format format) {
        switch (format) {
        case Format::Undefined:
            return MTL::PixelFormatInvalid;
        case Format::B8G8R8A8Unorm:
            return MTL::PixelFormatBGRA8Unorm;
        case Format::R8Unorm:
            return MTL::PixelFormatR8Unorm;
        case Format::R8G8Unorm:
            return MTL::PixelFormatRG8Unorm;
        case Format::R8G8B8Unorm:
            return MTL::PixelFormatRGBA8Unorm;
        case Format::R8G8B8A8Unorm:
            return MTL::PixelFormatRGBA8Unorm;
        case Format::B8G8R8A8Srgb:
            return MTL::PixelFormatBGRA8Unorm_sRGB;
        case Format::R8Srgb:
            return MTL::PixelFormatR8Unorm_sRGB;
        case Format::R8G8Srgb:
            return MTL::PixelFormatRG8Unorm_sRGB;
        case Format::R8G8B8Srgb:
            return MTL::PixelFormatRGBA8Unorm_sRGB;
        case Format::R8G8B8A8Srgb:
            return MTL::PixelFormatRGBA8Unorm_sRGB;
        case Format::D24UnormS8Uint:
            return MTL::PixelFormatDepth24Unorm_Stencil8;
        case Format::D32sFloat:
            return MTL::PixelFormatDepth32Float;
        case Format::R32G32Sfloat:
            return MTL::PixelFormatRG32Float;
        case Format::R32G32B32Sfloat:
            return MTL::PixelFormatRGBA32Float;
        case Format::R32G32B32A32Sfloat:
            return MTL::PixelFormatRGBA32Float;
        default:
            return MTL::PixelFormatInvalid;
        }
    }

    inline Format metalToAxPixelFormat(MTL::PixelFormat pixelFormat) {
        switch (pixelFormat) {
        case MTL::PixelFormatInvalid:
            return Format::Undefined;
        case MTL::PixelFormatBGRA8Unorm:
            return Format::B8G8R8A8Unorm;
        case MTL::PixelFormatR8Unorm:
            return Format::R8Unorm;
        case MTL::PixelFormatRG8Unorm:
            return Format::R8G8Unorm;
        case MTL::PixelFormatRGBA8Unorm:
            return Format::R8G8B8A8Unorm;
        case MTL::PixelFormatBGRA8Unorm_sRGB:
            return Format::B8G8R8A8Srgb;
        case MTL::PixelFormatR8Unorm_sRGB:
            return Format::R8Srgb;
        case MTL::PixelFormatRG8Unorm_sRGB:
            return Format::R8G8Srgb;
        case MTL::PixelFormatRGBA8Unorm_sRGB:
            return Format::R8G8B8A8Srgb;
        case MTL::PixelFormatDepth24Unorm_Stencil8:
            return Format::D24UnormS8Uint;
        case MTL::PixelFormatDepth32Float:
            return Format::D32sFloat;
        case MTL::PixelFormatRG32Float:
            return Format::R32G32Sfloat;
        case MTL::PixelFormatRGBA32Float:
            return Format::R32G32B32A32Sfloat; // Metal doesn't have a separate RGBA32Float format, using RGBA32Float for both
        default:
            return Format::Undefined;
        }
    }

    inline uint32_t getStride(Format format) {
        switch (format) {
        case Format::Undefined:
            return 0;
        case Format::B8G8R8A8Unorm:
        case Format::R8G8B8A8Unorm:
        case Format::B8G8R8A8Srgb:
        case Format::R8G8B8A8Srgb:
            return 4;
        case Format::R8Unorm:
            return 1;
        case Format::R8G8Unorm:
            return 2;
        case Format::R8G8B8Unorm:
            return 3;
        case Format::D24UnormS8Uint:
            return 4;
        case Format::D32sFloat:
            return 4;
        case Format::R32G32Sfloat:
            return 8;
        case Format::R32G32B32Sfloat:
            return 12;
        case Format::R32G32B32A32Sfloat:
            return 16;
        default:
            return 0;
        }
    }

    inline MTL::LoadAction axToMetalLoadAction(LoadOp loadOp) {
        switch (loadOp) {
        case LoadOp::Load:
            return MTL::LoadActionLoad;
        case LoadOp::Clear:
            return MTL::LoadActionClear;
        case LoadOp::DontCare:
            return MTL::LoadActionDontCare;
        default:
            return MTL::LoadActionDontCare;
        }
    }

    inline MTL::StoreAction axToMetalStoreAction(StoreOp storeOp) {
        switch (storeOp) {
        case StoreOp::Store:
            return MTL::StoreActionStore;
        case StoreOp::DontCare:
            return MTL::StoreActionDontCare;
        default:
            return MTL::StoreActionDontCare;
        }
    }

    inline MTL::ResourceOptions axToMetalResourceOptions(MemoryUsage memoryUsage) {
        switch (memoryUsage) {
        case MemoryUsage::GPUOnly:
            return MTL::ResourceStorageModePrivate;
        case MemoryUsage::GPUandCPU:
            return MTL::ResourceStorageModeShared;
        default:
            return MTL::ResourceStorageModeShared;
        }
    }

    inline MTL::TextureUsage axToMetalTextureUsage(TextureUsage usage) {
        MTL::TextureUsage metalUsage = MTL::TextureUsageUnknown;
        if ((usage & TextureUsage::Sampled) != TextureUsage::None) {
            metalUsage |= MTL::TextureUsageShaderRead;
        }
        if ((usage & TextureUsage::Storage) != TextureUsage::None) {
            metalUsage |= MTL::TextureUsageShaderWrite;
        }
        if ((usage & TextureUsage::ColorAttachment) != TextureUsage::None) {
            metalUsage |= MTL::TextureUsageRenderTarget;
        }
        if ((usage & TextureUsage::DepthStencilAttachment) != TextureUsage::None) {
            metalUsage |= MTL::TextureUsageRenderTarget;
        }
        return metalUsage;
    }

    inline MTL::TextureType axToMetalTextureType(TextureType type) {
        switch (type) {
        case TextureType::Texture1D:
            return MTL::TextureType1D;
        case TextureType::Texture1DArray:
            return MTL::TextureType1DArray;
        case TextureType::Texture2D:
            return MTL::TextureType2D;
        case TextureType::Texture2DArray:
            return MTL::TextureType2DArray;
        case TextureType::Texture3D:
            return MTL::TextureType3D;
        case TextureType::TextureCube:
            return MTL::TextureTypeCube;
        default:
            return MTL::TextureType2D;
        }
    }

    inline MTL::StorageMode axToMetalStorageMode(MemoryUsage memoryUsage) {
        switch (memoryUsage) {
        case MemoryUsage::GPUOnly:
            return MTL::StorageModePrivate;
        case MemoryUsage::GPUandCPU:
            return MTL::StorageModeShared;
        default:
            return MTL::StorageModeShared;
        }
    }

    inline MTL::SamplerAddressMode axToMetalAdressMode(SamplerAddressMode adressMode) {
        switch (adressMode) {
        case SamplerAddressMode::Repeat:
            return MTL::SamplerAddressModeRepeat;
        case SamplerAddressMode::MirroredRepeat:
            return MTL::SamplerAddressModeMirrorRepeat;
        case SamplerAddressMode::ClampToEdge:
            return MTL::SamplerAddressModeClampToEdge;
        case SamplerAddressMode::ClampToBorder:
            return MTL::SamplerAddressModeClampToBorderColor;
        default:
            return MTL::SamplerAddressModeRepeat;
        }
    }

    inline MTL::SamplerMinMagFilter axToMetalFilterMode(SamplerFilterMode filterMode) {
        switch (filterMode) {
        case SamplerFilterMode::Nearest:
            return MTL::SamplerMinMagFilterNearest;
        case SamplerFilterMode::Linear:
            return MTL::SamplerMinMagFilterLinear;
        default:
            return MTL::SamplerMinMagFilterNearest;
        }
    }

    inline MTL::SamplerMipFilter axToMetalMipmapFilterMode(SamplerFilterMode filterMode) {
        switch (filterMode) {
        case SamplerFilterMode::Nearest:
            return MTL::SamplerMipFilterNotMipmapped;
        case SamplerFilterMode::Linear:
            return MTL::SamplerMipFilterNotMipmapped; // Metal doesn't have a separate mip filter mode for linear mipmapping
        default:
            return MTL::SamplerMipFilterNotMipmapped;
        }
    }

} // namespace Axiom
