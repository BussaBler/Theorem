#pragma once
#include "Asset/ShaderAsset.h"
#include "Core/Locator.h"
#include "Font.h"
#include "Math/AxMath.h"
#include "Math/Color.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/ResourceSet.h"
#include "UIVertex.h"

#include <array>
#include <stack>

namespace Axiom {
    class UIRenderer {
      public:
        UIRenderer();
        ~UIRenderer() = default;

        void beginFrame();

        void addBasicQuad(const Math::Vec2& pos, const Math::Vec2& size, const Color& color, const Math::Vec4& radii = Math::Vec4::zero(), uint8_t layer = 0);
        void addDebugRect(const Math::Vec2& pos, const Math::Vec2& size, const Color& color);
        void addFontQuad(const Math::Vec2& pos, const Math::Vec2& size, const Math::Vec2& uv0, const Math::Vec2& uv1, const Color& color, uint8_t layer = 0);
        void addText(const std::string& text, const Math::Vec2& pos, float fontSize, float dpiScale, const Color& color, uint8_t layer = 0);
        float calculateTextWidth(const std::string& text, float fontSize, float dpiScale);
        float calculateTextHeight(float fontSize, float dpiScale);
        void addImageQuad(const Math::Vec2& pos, const Math::Vec2& size, Texture* texture, uint8_t layer = 0);

        void pushScissorRect(const Math::Rect& rect, uint8_t layer = 0);
        void popScissorRect(uint8_t layer = 0);

        void onRender(CommandBuffer* commandBuffer, Texture* targetTexture);

        inline Font& getFont() { return openSansFont; }

      private:
        void createBasicRenderObjects();
        void createFontRenderObjects();
        void createImageRenderObjects();

        ResourceSet* getResourceSetForTexture(Texture* texture);

      private:
        // used for rendering lines in the editor (e.g. gizmos)
        Math::Mat4 gizmosProjection;
        Math::Mat4 gizmosView;

        std::stack<Math::Rect> scissorRectStack;

        static const uint32_t MAX_BASIC_QUADS = 1000;
        std::shared_ptr<ShaderAsset> basicShader = nullptr;
        std::unique_ptr<Pipeline> basicPipeline = nullptr;
        RenderPass basicRenderPass;
        std::unique_ptr<Buffer> basicVertexBuffer = nullptr;
        std::unique_ptr<Buffer> basicIndexBuffer = nullptr;

        static const uint32_t MAX_FONT_QUADS = 1000;
        Font openSansFont{"Assets/Fonts/OpenSans-SemiBold.ttf"};
        std::shared_ptr<Texture> fontAtlasTexture = nullptr;
        std::shared_ptr<ShaderAsset> fontShader = nullptr;
        std::unique_ptr<Pipeline> fontPipeline = nullptr;
        RenderPass fontRenderPass;
        std::unique_ptr<Buffer> fontVertexBuffer = nullptr;
        std::unique_ptr<Buffer> fontIndexBuffer = nullptr;
        std::unique_ptr<ResourceLayout> fontResourceLayout = nullptr;
        std::unique_ptr<ResourceSet> fontResourceSet = nullptr;

        struct ImageDrawCommand {
            Texture* texture;
            Math::Vec2 pos;
            Math::Vec2 size;

            Math::Rect scissorRect;
        };
        static const uint32_t MAX_IMAGE_QUADS = 100;
        std::shared_ptr<ShaderAsset> imageShader = nullptr;
        std::unique_ptr<Pipeline> imagePipeline = nullptr;
        RenderPass imageRenderPass;
        std::unique_ptr<Buffer> imageVertexBuffer = nullptr;
        std::unique_ptr<Buffer> imageIndexBuffer = nullptr;
        std::unique_ptr<ResourceLayout> imageResourceLayout = nullptr;
        std::unordered_map<Texture*, std::unique_ptr<ResourceSet>> imageResourceSets;

        struct RenderBatch {
            size_t vertexOffset = 0;
            uint32_t vertexCount = 0;

            Math::Rect scissorRect;
        };

        struct RenderLayer {
            std::vector<RenderBatch> basicRenderBatches;
            std::vector<RenderBatch> fontRenderBatches;

            std::vector<UIVertex> basicVertices;
            std::vector<UIVertex> fontVertices;
            std::vector<ImageDrawCommand> imageDrawCommands;
        };
        uint8_t constexpr static MAX_RENDER_LAYERS = 4;
        std::array<RenderLayer, MAX_RENDER_LAYERS> renderLayers;
    };
}; // namespace Axiom
