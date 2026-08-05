#include "axpch.h"

#include "MetalShader.h"

#include <SpirvCross/spirv_msl.hpp>
#include <cstdint>
#include <shaderc/shaderc.hpp>
#include <vector>

namespace Axiom {
    MetalShader::MetalShader(const std::string& vertexSource, const std::string& fragmentSource, MTL::Device* device) {
        std::vector<uint32_t> vertexSPIRV = compileGLSLToSPIRV(vertexSource);
        std::vector<uint32_t> fragmentSPIRV = compileGLSLToSPIRV(fragmentSource);

        std::string vertexMSL = compileSPIRVtoMSL(vertexSPIRV);
        std::string fragmentMSL = compileSPIRVtoMSL(fragmentSPIRV);
        NS::String* vertexMSLStr = NS::String::alloc()->init(vertexMSL.c_str(), NS::StringEncoding::UTF8StringEncoding);
        NS::String* fragmentMSLStr = NS::String::alloc()->init(fragmentMSL.c_str(), NS::StringEncoding::UTF8StringEncoding);

        MTL::CompileOptions* compileOptions = MTL::CompileOptions::alloc()->init();
        NS::Error* error = nullptr;

        vertexLibrary = device->newLibrary(vertexMSLStr, compileOptions, &error);
        AX_CORE_ASSERT(error == nullptr, "Failed to compile vertex shader library: {}", error->localizedDescription()->utf8String());
        AX_CORE_ASSERT(vertexLibrary, "Failed to create vertex shader library");

        fragmentLibrary = device->newLibrary(fragmentMSLStr, compileOptions, &error);
        AX_CORE_ASSERT(error == nullptr, "Failed to compile fragment shader library: {}", error->localizedDescription()->utf8String());
        AX_CORE_ASSERT(fragmentLibrary, "Failed to create fragment shader library");

        compileOptions->release();
        vertexMSLStr->release();
        fragmentMSLStr->release();
    }

    MetalShader::~MetalShader() {
        if (vertexLibrary) {
            vertexLibrary->release();
            vertexLibrary = nullptr;
        }
        if (fragmentLibrary) {
            fragmentLibrary->release();
            fragmentLibrary = nullptr;
        }
    }

    std::vector<uint32_t> MetalShader::compileGLSLToSPIRV(const std::string& source) {
        shaderc::Compiler compiler = {};
        shaderc::CompileOptions options = {};
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
        shaderc::CompilationResult compResult =
            compiler.CompileGlslToSpv(source.c_str(), source.size(), shaderc_shader_kind::shaderc_glsl_infer_from_source, "shader", "main", options);
        AX_CORE_ASSERT(compResult.GetCompilationStatus() == shaderc_compilation_status_success, "Failed to compile shader: {0}", compResult.GetErrorMessage());
        return std::vector<uint32_t>(compResult.begin(), compResult.end());
    }

    std::string MetalShader::compileSPIRVtoMSL(const std::vector<uint32_t>& spirv) {
        spirv_cross::CompilerMSL mslCompiler(spirv);
        spirv_cross::CompilerMSL::Options mslOptions = {};
        mslOptions.platform = spirv_cross::CompilerMSL::Options::Platform::macOS;
        mslOptions.set_msl_version(3, 0);
        mslOptions.argument_buffers = true;
        mslCompiler.set_msl_options(mslOptions);

        spv::ExecutionModel executionModel = mslCompiler.get_execution_model();

        spirv_cross::MSLResourceBinding pushConstantMap = {};
        pushConstantMap.stage = executionModel;
        pushConstantMap.desc_set = spirv_cross::kPushConstDescSet;
        pushConstantMap.binding = spirv_cross::kPushConstBinding;
        pushConstantMap.msl_buffer = 4;
        mslCompiler.add_msl_resource_binding(pushConstantMap);

        spirv_cross::ShaderResources resources = mslCompiler.get_shader_resources();
        uint32_t metalBufferOffset = 8;
        std::vector<uint32_t> mappedSets;

        auto mapDescriptorSets = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources) {
            for (const auto& resource : resources) {
                uint32_t set = mslCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                if (std::find(mappedSets.begin(), mappedSets.end(), set) == mappedSets.end()) {
                    spirv_cross::MSLResourceBinding argBinding = {};
                    argBinding.stage = executionModel;
                    argBinding.desc_set = set;

                    argBinding.binding = spirv_cross::kArgumentBufferBinding;
                    argBinding.msl_buffer = metalBufferOffset + set;

                    mslCompiler.add_msl_resource_binding(argBinding);
                    mappedSets.push_back(set);
                }
            }
        };

        mapDescriptorSets(resources.uniform_buffers);
        mapDescriptorSets(resources.storage_buffers);
        mapDescriptorSets(resources.sampled_images);
        mapDescriptorSets(resources.separate_images);
        mapDescriptorSets(resources.separate_samplers);

        return mslCompiler.compile();
    }
} // namespace Axiom
