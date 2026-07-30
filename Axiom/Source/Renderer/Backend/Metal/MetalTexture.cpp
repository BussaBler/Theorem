#include "axpch.h"

#include "MetalTexture.h"

#include "Renderer/Texture.h"

namespace Axiom {
    MetalTexture::MetalTexture(const CreateInfo& createInfo, MTL::Device* device) : Texture(createInfo), size(createInfo.width, createInfo.height) {
        format = createInfo.format;
        mipLevels = createInfo.mipLevels;
        arrayLayers = createInfo.arrayLayers;

        MTL::TextureDescriptor* descriptor = MTL::TextureDescriptor::alloc()->init();
        descriptor->setPixelFormat(axToMetalPixelFormat(createInfo.format));
        descriptor->setWidth(createInfo.width);
        descriptor->setHeight(createInfo.height);
        descriptor->setUsage(axToMetalTextureUsage(createInfo.usage));
        descriptor->setStorageMode(axToMetalStorageMode(createInfo.memoryUsage));
        descriptor->setMipmapLevelCount(createInfo.mipLevels);
        descriptor->setTextureType(axToMetalTextureType(createInfo.type));
        if (createInfo.type == TextureType::TextureCube) {
            arrayLayers = 1;
        }
        descriptor->setArrayLength(arrayLayers);
        descriptor->setResourceOptions(axToMetalResourceOptions(createInfo.memoryUsage));
        descriptor->setAllowGPUOptimizedContents(true);

        metalTexture = device->newTexture(descriptor);
    }

    MetalTexture::MetalTexture(MTL::Texture* texture) : Texture({}), metalTexture(texture) {
        if (metalTexture) {
            metalTexture->retain();
            format = metalToAxPixelFormat(metalTexture->pixelFormat());
            size = Math::iVec2(metalTexture->width(), metalTexture->height());
        }
    }

    MetalTexture::~MetalTexture() {
        if (metalTexture) {
            metalTexture->release();
            metalTexture = nullptr;
        }
    }

    Format MetalTexture::getFormat() const {
        return format;
    }

    Math::iVec2 MetalTexture::getSize() const {
        return size;
    }

    uint32_t MetalTexture::getMipLevels() const {
        return mipLevels;
    }

    uint32_t MetalTexture::getArrayLayers() const {
        return arrayLayers;
    }
} // namespace Axiom
