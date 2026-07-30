#include "axpch.h"

#include "MetalResourceSet.h"

#include "Metal/Metal.hpp"
#include "MetalBuffer.h"
#include "MetalSampler.h"
#include "MetalTexture.h"
#include "Renderer/Backend/Metal/MetalResourceLayout.h"
#include "Renderer/ResourceLayout.h"

namespace Axiom {
    MetalResourceSet::MetalResourceSet(const ResourceLayout* layout, MTL::Device* device) {
        const MetalResourceLayout* mtlLayout = static_cast<const MetalResourceLayout*>(layout);
        std::vector<NS::Object*> descriptors;
        descriptors.reserve(mtlLayout->getBindingsCreateInfo().size());

        for (const auto& binding : mtlLayout->getBindingsCreateInfo()) {
            MTL::ArgumentDescriptor* argDescriptor = MTL::ArgumentDescriptor::argumentDescriptor();

            argDescriptor->setIndex(binding.binding);
            argDescriptor->setArrayLength(binding.count);

            switch (binding.type) {
            case ResourceType::UniformBuffer:
                argDescriptor->setDataType(MTL::DataTypePointer);
                argDescriptor->setAccess(MTL::BindingAccessReadOnly);
                break;
            case ResourceType::StorageBuffer:
                argDescriptor->setDataType(MTL::DataTypePointer);
                argDescriptor->setAccess(MTL::BindingAccessReadWrite);
                break;
            case ResourceType::Texture:
                argDescriptor->setDataType(MTL::DataTypeTexture);
                argDescriptor->setAccess(MTL::BindingAccessReadOnly);
                argDescriptor->setTextureType(MTL::TextureType2D);
                break;
            case ResourceType::Sampler:
                argDescriptor->setDataType(MTL::DataTypeSampler);
                argDescriptor->setAccess(MTL::BindingAccessReadOnly);
                break;
            case ResourceType::CombinedTextureSampler:
                break;
            }
            descriptors.push_back(argDescriptor);
        }

        NS::Array* nsDescriptorsArray = NS::Array::array(descriptors.data(), descriptors.size());

        argumentEncoder = device->newArgumentEncoder(nsDescriptorsArray);

        NS::UInteger argumentBufferSize = argumentEncoder->encodedLength();
        argumentBuffer = device->newBuffer(argumentBufferSize, MTL::ResourceStorageModeShared);
        argumentEncoder->setArgumentBuffer(argumentBuffer, 0);
    }

    MetalResourceSet::~MetalResourceSet() {
        if (argumentBuffer) {
            argumentBuffer->release();
            argumentBuffer = nullptr;
        }
        if (argumentEncoder) {
            argumentEncoder->release();
            argumentEncoder = nullptr;
        }
    }

    void MetalResourceSet::update(const std::vector<Binding>& bindings) {
        uint32_t bindingOffset = 0;
        residentBuffers.clear();
        residentTextures.clear();
        for (const auto& binding : bindings) {
            switch (binding.type) {
            case ResourceType::UniformBuffer:
            case ResourceType::StorageBuffer: {
                std::vector<MTL::Buffer*> buffersToSet;
                std::vector<NS::UInteger> bufferOffsets;
                for (Buffer* buffer : binding.buffers) {
                    MetalBuffer* metalBuffer = static_cast<MetalBuffer*>(buffer);
                    buffersToSet.push_back(metalBuffer->getHandle());
                    residentBuffers.push_back(metalBuffer->getHandle());
                    bufferOffsets.push_back(0); // TODO: support buffer offsets
                }
                argumentEncoder->setBuffers(buffersToSet.data(), bufferOffsets.data(), NS::Range::Make(binding.binding + bindingOffset, buffersToSet.size()));
                bindingOffset += binding.maxNumberOfResources - 1;
                break;
            }
            case ResourceType::Texture: {
                std::vector<MTL::Texture*> texturesToSet;
                for (Texture* texture : binding.textures) {
                    MetalTexture* metalTexture = static_cast<MetalTexture*>(texture);
                    texturesToSet.push_back(metalTexture->getHandle());
                    residentTextures.push_back(metalTexture->getHandle());
                }
                argumentEncoder->setTextures(texturesToSet.data(), NS::Range::Make(binding.binding + bindingOffset, texturesToSet.size()));
                bindingOffset += binding.maxNumberOfResources - 1;
                break;
            }
            case ResourceType::Sampler: {
                std::vector<MTL::SamplerState*> samplersToSet;
                for (Sampler* sampler : binding.samplers) {
                    MetalSampler* metalSampler = static_cast<MetalSampler*>(sampler);
                    samplersToSet.push_back(metalSampler->getHandle());
                }
                argumentEncoder->setSamplerStates(samplersToSet.data(), NS::Range::Make(binding.binding + bindingOffset, samplersToSet.size()));
                bindingOffset += binding.maxNumberOfResources - 1;
                break;
            }
            default:
                AX_CORE_LOG_ERROR("Unsupported resource type in MetalResourceSet::update");
                break;
            }
        }
    }
} // namespace Axiom
