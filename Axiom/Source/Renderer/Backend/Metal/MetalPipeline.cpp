#include "axpch.h"

#include "MetalPipeline.h"

#include "Core/Assert.h"
#include "Metal/Metal.hpp"
#include "MetalShader.h"

namespace Axiom {
    MetalPipeline::MetalPipeline(const CreateInfo& createInfo, MTL::Device* device) : device(device) {
        MetalShader* metalShader = static_cast<MetalShader*>(createInfo.shader);
        MTL::Library* vertexLibrary = metalShader->getVertexLibrary();
        MTL::Library* fragmentLibrary = metalShader->getFragmentLibrary();

        MTL::Function* vertexFunction = vertexLibrary->newFunction(NS::String::string("main0", NS::UTF8StringEncoding));
        AX_CORE_ASSERT(vertexFunction, "Failed to create vertex function");

        MTL::Function* fragmentFunction = fragmentLibrary->newFunction(NS::String::string("main0", NS::UTF8StringEncoding));
        AX_CORE_ASSERT(fragmentFunction, "Failed to create fragment function");

        MTL::RenderPipelineDescriptor* pipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
        pipelineDescriptor->setVertexFunction(vertexFunction);
        pipelineDescriptor->setFragmentFunction(fragmentFunction);

        MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();
        uint32_t vertexBufferOffset = 0; // index 0 to 3 are for vertex buffers

        for (size_t i = 0; i < createInfo.vertexBindings.size(); i++) {
            MTL::VertexBufferLayoutDescriptor* layout = MTL::VertexBufferLayoutDescriptor::alloc()->init();
            layout->setStride(createInfo.vertexBindings[i].stride);
            layout->setStepFunction(createInfo.vertexBindings[i].inputRate == VertexInputRate::Vertex ? MTL::VertexStepFunctionPerVertex
                                                                                                      : MTL::VertexStepFunctionPerInstance);
            vertexDescriptor->layouts()->setObject(layout, createInfo.vertexBindings[i].binding + vertexBufferOffset);
            layout->release();
        }

        for (size_t i = 0; i < createInfo.vertexAttributes.size(); i++) {
            MTL::VertexAttributeDescriptor* attribute = MTL::VertexAttributeDescriptor::alloc()->init();
            attribute->setFormat(axToMetalVertexFormat(createInfo.vertexAttributes[i].format));
            attribute->setOffset(createInfo.vertexAttributes[i].offset);
            attribute->setBufferIndex(createInfo.vertexAttributes[i].binding + vertexBufferOffset);
            vertexDescriptor->attributes()->setObject(attribute, createInfo.vertexAttributes[i].location);
            attribute->release();
        }

        pipelineDescriptor->setVertexDescriptor(vertexDescriptor);
        vertexDescriptor->release();

        pipelineDescriptor->setInputPrimitiveTopology(
            createInfo.topology == PrimitiveTopology::TriangleList
                ? MTL::PrimitiveTopologyClassTriangle
                : (createInfo.topology == PrimitiveTopology::LineList ? MTL::PrimitiveTopologyClassLine : MTL::PrimitiveTopologyClassPoint));
        pipelineDescriptor->setRasterizationEnabled(createInfo.polygonMode == PolygonMode::Fill);
        primitiveType = createInfo.topology == PrimitiveTopology::TriangleList
                            ? MTL::PrimitiveTypeTriangle
                            : (createInfo.topology == PrimitiveTopology::LineList ? MTL::PrimitiveTypeLine : MTL::PrimitiveTypePoint);

        for (size_t i = 0; i < createInfo.colorAttachmentFormats.size(); i++) {
            pipelineDescriptor->colorAttachments()->object(i)->setPixelFormat(axToMetalPixelFormat(createInfo.colorAttachmentFormats[i]));

            if (createInfo.enableBlending) {
                pipelineDescriptor->colorAttachments()->object(i)->setBlendingEnabled(true);
                pipelineDescriptor->colorAttachments()->object(i)->setRgbBlendOperation(MTL::BlendOperationAdd);
                pipelineDescriptor->colorAttachments()->object(i)->setAlphaBlendOperation(MTL::BlendOperationAdd);
                pipelineDescriptor->colorAttachments()->object(i)->setSourceRGBBlendFactor(MTL::BlendFactorOne);
                pipelineDescriptor->colorAttachments()->object(i)->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
                pipelineDescriptor->colorAttachments()->object(i)->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
                pipelineDescriptor->colorAttachments()->object(i)->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
            }
        }

        if (createInfo.depthAttachmentFormat != Format::Undefined) {
            pipelineDescriptor->setDepthAttachmentPixelFormat(axToMetalPixelFormat(createInfo.depthAttachmentFormat));
        }

        MTL::DepthStencilDescriptor* depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
        depthStencilDescriptor->setDepthCompareFunction(createInfo.enableDepthTest ? MTL::CompareFunctionLessEqual : MTL::CompareFunctionAlways);
        depthStencilDescriptor->setDepthWriteEnabled(createInfo.enableDepthWrite);
        depthStencilState = device->newDepthStencilState(depthStencilDescriptor);
        depthStencilDescriptor->release();

        MTL::PipelineOption options = MTL::PipelineOptionArgumentInfo | MTL::PipelineOptionBufferTypeInfo;
        MTL::RenderPipelineReflection* reflection = nullptr;

        NS::Error* error = nullptr;
        pipelineState = device->newRenderPipelineState(pipelineDescriptor, options, &reflection, &error);
        AX_CORE_ASSERT(error == nullptr, "Failed to create render pipeline state: {}", error->localizedDescription()->utf8String());
        AX_CORE_ASSERT(pipelineState, "Failed to create render pipeline state");

        pipelineDescriptor->release();
        vertexFunction->release();
        fragmentFunction->release();
    }

    MetalPipeline::~MetalPipeline() {
        if (pipelineState) {
            depthStencilState->release();
            pipelineState->release();
            pipelineState = nullptr;
        }
    }
} // namespace Axiom
