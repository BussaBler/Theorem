#include "VulkanTexture.h"

#include "Renderer/Texture.h"

namespace Axiom {
    VulkanTexture::VulkanTexture(Vk::Device logicalDevice, const CreateInfo& createInfo)
        : Texture(createInfo), device(logicalDevice), ownsImage(true), size(createInfo.width, createInfo.height) {
        mipLevels = createInfo.mipLevels;
        arrayLayers = createInfo.arrayLayers;

        Vk::ImageCreateFlags flags = {};
        if (createInfo.type == TextureType::TextureCube) {
            flags |= Vk::ImageCreateFlagBits::eCubeCompatible;
            arrayLayers = 6;
        }

        Vk::ImageCreateInfo imageCreateInfo(flags, Vk::ImageType::e2D, axToVkFormat(createInfo.format), {createInfo.width, createInfo.height, 1},
                                            createInfo.mipLevels, arrayLayers, Vk::SampleCountFlagBits::e1, Vk::ImageTiling::eOptimal,
                                            axToVkImageUsage(createInfo.usage));

        imageCreateInfo.setInitialLayout(axToVkImageLayout(createInfo.initialState));
        imageCreateInfo.setImageType(axToVkImageType(createInfo.type));

        Vk::ResultValue<Vk::Image> imageResult = device.createImage(imageCreateInfo);
        AX_CORE_ASSERT(imageResult.result == Vk::Result::eSuccess, "Failed to create image for texture!");
        image = imageResult.value;

        imageAllocation = VulkanAllocator::allocate(image, axToVkMemProperty(createInfo.memoryUsage));

        Vk::Result result = device.bindImageMemory(image, imageAllocation.memory, imageAllocation.offset);
        AX_CORE_ASSERT(result == Vk::Result::eSuccess, "Failed to bind image memory");

        createImageView(imageCreateInfo.format, axToVkImageAspectFlags(createInfo.aspect), axToVkImageViewType(createInfo.type));
    }

    VulkanTexture::VulkanTexture(Vk::Device logicalDevice, Vk::Image existingImage, Vk::Format format, Math::iVec2 size)
        : device(logicalDevice), image(existingImage), ownsImage(false), imageFormat(format), size(size) {
    }

    VulkanTexture::~VulkanTexture() {
        if (ownsImage) {
            VulkanAllocator::free(imageAllocation);
            if (image)
                device.destroyImage(image);
        }

        if (imageView)
            device.destroyImageView(imageView);
    }

    Format VulkanTexture::getFormat() const {
        return vkToAxFormat(imageFormat);
    }

    Math::iVec2 VulkanTexture::getSize() const {
        return size;
    }

    uint32_t VulkanTexture::getMipLevels() const {
        return mipLevels;
    }

    uint32_t VulkanTexture::getArrayLayers() const {
        return arrayLayers;
    }

    void VulkanTexture::createImageView(Vk::Format format, Vk::ImageAspectFlags aspectFlags, Vk::ImageViewType viewType) {
        imageFormat = format;

        Vk::ImageViewCreateInfo createInfo({}, image, viewType, format,
                                           {Vk::ComponentSwizzle::eR, Vk::ComponentSwizzle::eG, Vk::ComponentSwizzle::eB, Vk::ComponentSwizzle::eA},
                                           {aspectFlags, 0, Vk::RemainingMipLevels, 0, Vk::RemainingArrayLayers});

        Vk::ResultValue<Vk::ImageView> imageViewResult = device.createImageView(createInfo);
        AX_CORE_ASSERT(imageViewResult.result == Vk::Result::eSuccess, "Failed to create image view for texture!");
        imageView = imageViewResult.value;
    }
} // namespace Axiom
