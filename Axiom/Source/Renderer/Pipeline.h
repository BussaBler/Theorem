#pragma once
#include "ResourceLayout.h"
#include "ResourceSet.h"
#include "Shader.h"

#include <cstddef>
#include <functional>
#include <memory>

namespace Axiom {
    enum class VertexInputRate { Vertex, Instance };

    enum class PrimitiveTopology { TriangleList, LineList, PointList };

    enum class PolygonMode { Fill, Line, Point };

    enum class CullMode { None, Front, Back };

    struct VertexBindingDescription {
        uint32_t binding = 0;
        uint32_t stride = 0;
        VertexInputRate inputRate = VertexInputRate::Vertex;

        bool operator==(const VertexBindingDescription& other) const noexcept {
            return binding == other.binding && stride == other.stride && inputRate == other.inputRate;
        }
    };

    struct VertexAttributeDescription {
        uint32_t location = 0;
        uint32_t binding = 0;
        Format format = Format::Undefined;
        uint32_t offset = 0;

        bool operator==(const VertexAttributeDescription& other) const noexcept {
            return location == other.location && binding == other.binding && format == other.format && offset == other.offset;
        }
    };

    class Pipeline {
      public:
        struct CreateInfo {
            Shader* shader = nullptr;

            std::vector<VertexBindingDescription> vertexBindings;
            std::vector<VertexAttributeDescription> vertexAttributes;

            PrimitiveTopology topology = PrimitiveTopology::TriangleList;
            PolygonMode polygonMode = PolygonMode::Fill;
            CullMode cullMode = CullMode::Back;
            bool frontFaceClockwise = true;

            bool enableBlending = false;
            bool enableDepthTest = true;
            bool enableDepthWrite = true;

            std::vector<Format> colorAttachmentFormats;
            Format depthAttachmentFormat = Format::Undefined;

            std::vector<ResourceLayout*> resourceLayouts;

            bool operator==(const Pipeline::CreateInfo& other) const noexcept {
                return shader == other.shader && topology == other.topology && polygonMode == other.polygonMode && cullMode == other.cullMode &&
                       frontFaceClockwise == other.frontFaceClockwise && enableBlending == other.enableBlending && enableDepthTest == other.enableDepthTest &&
                       enableDepthWrite == other.enableDepthWrite && depthAttachmentFormat == other.depthAttachmentFormat &&
                       vertexBindings == other.vertexBindings && vertexAttributes == other.vertexAttributes &&
                       colorAttachmentFormats == other.colorAttachmentFormats && resourceLayouts == other.resourceLayouts;
            }
        };

        Pipeline() = default;
        virtual ~Pipeline() = default;
    };
} // namespace Axiom

template <class T> inline void hashCombine(size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std {
    template <> struct hash<Axiom::Pipeline::CreateInfo> {
        size_t operator()(const Axiom::Pipeline::CreateInfo& spec) const {
            size_t seed = 0;
            hashCombine(seed, spec.shader);
            hashCombine(seed, static_cast<uint32_t>(spec.topology));
            hashCombine(seed, static_cast<uint32_t>(spec.polygonMode));
            hashCombine(seed, static_cast<uint32_t>(spec.cullMode));
            hashCombine(seed, spec.frontFaceClockwise);
            hashCombine(seed, spec.enableBlending);
            hashCombine(seed, spec.enableDepthTest);
            hashCombine(seed, spec.enableDepthWrite);
            hashCombine(seed, static_cast<uint32_t>(spec.depthAttachmentFormat));

            for (const auto& binding : spec.vertexBindings) {
                hashCombine(seed, binding.binding);
                hashCombine(seed, binding.stride);
                hashCombine(seed, static_cast<uint32_t>(binding.inputRate));
            }

            for (const auto& attr : spec.vertexAttributes) {
                hashCombine(seed, attr.location);
                hashCombine(seed, attr.binding);
                hashCombine(seed, static_cast<uint32_t>(attr.format));
                hashCombine(seed, attr.offset);
            }

            for (const auto& format : spec.colorAttachmentFormats) {
                hashCombine(seed, static_cast<uint32_t>(format));
            }

            for (const auto& layout : spec.resourceLayouts) {
                hashCombine(seed, layout);
            }

            return seed;
        }
    };
} // namespace std
