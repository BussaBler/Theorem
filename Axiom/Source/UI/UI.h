#pragma once
#include "axpch.h"

#include "Event/KeyCodes.h"
#include "Math/Color.h"
#include "UIRenderer.h"

namespace Axiom {
    struct UIContext {
        Math::Vec2 cursorPos = Math::Vec2::zero();
        Math::Vec2 panelPos = Math::Vec2::zero();
        Math::Vec2 panelSize = Math::Vec2::zero();
        Math::Vec2 lastItemPos = Math::Vec2::zero();
        Math::Vec2 lastItemSize = Math::Vec2::zero();

        float lineHeight = 0.0f;
        float indentLevel = 0.0f;
        Color currentColor = Color::white();

        uint32_t hotItem = 0;
        uint32_t activeItem = 0;
        uint32_t focusedItem = 0;

        std::unordered_map<uint32_t, bool> nodeStates;
        struct GroupState {
            Math::Vec2 panelSize;
            Math::Vec2 cursorPos;
            float indentLevel;
        };
        std::vector<GroupState> groupStack;
        std::vector<float> indentStack;
    };

    struct UIInputState {
        Math::Vec2 mousePosition = Math::Vec2::zero();
        Math::Vec2 lastMousePosition = Math::Vec2::zero();
        Math::Vec2 mouseDelta = Math::Vec2::zero();
        bool isMouseButtonOneDown = false;
        bool isMouseButtonTwoDown = false;
        bool shouldConsumeMouse = false;
        std::string currentTextInput = "";
        uint16_t backspacesThisFrame = 0;
        bool isEnterPressed = false;
        bool shouldConsumeKeyboard = false;
    };

    struct UIStyle {
        float padding = 8.0f;
        float itemSpacing = 4.0f;
        float defaultWidgetHeight = 22.0f;
        Color panelColor = Color(0.18f, 0.18f, 0.18f, 1.0f);
        Color headerColor = Color(0.12f, 0.12f, 0.12f, 1.0f);
        Color buttonColor = Color(0.28f, 0.28f, 0.28f, 1.0f);
        Color buttonHoverColor = Color(0.35f, 0.35f, 0.35f, 1.0f);
        Color buttonActiveColor = Color(0.15f, 0.4f, 0.75f, 1.0f);
        Color textColor = Color::white();
        float borderRadius = 3.0f;
    };

    class UI {
      public:
        static void init(UIContext& newContext);
        static void shutdown();

        inline static void setContext(UIContext* newContext) { context = newContext; }

        static void onEvent(Event& event);

        static void beginFrame();
        static void endFrame();
        static void render(CommandBuffer* commandBuffer, Texture* targetTexture);

        static void beginPanel(const std::string& title, Math::Vec2 pos, Math::Vec2 size);
        static void endPanel();
        static void sameLine(float spacingOffset = -1.0f);
        static void spacing(float verticalSpacing = -1.0f);
        static void beginGroup(float width);
        static void endGroup();
        static void pushIndent(float amount);
        static void popIndent();
        static void clippedText(const std::string& str, float maxWidth, const Color& color, uint16_t size = 11);
        static bool button(const std::string& label, const Math::Vec2& size, Math::Vec4 radii = Math::Vec4(-1.0f));
        static void text(const std::string& text, const Color& color, uint16_t size = 11);
        static void rawText(const std::string& text, Math::Vec2 pos, Math::Vec4 color = Math::Vec4(1.0f), uint16_t size = 11);
        static void inputText(const std::string& label, std::string& value, uint16_t size = 11);
        static float calcTextWidth(const std::string& text, uint16_t size = 11);
        static void checkbox(const std::string& label, bool& value);
        static void dragFloat(const std::string& label, float& value, float speed = 0.1f, float width = -1.0f);
        static void dragVec2(const std::string& label, Math::Vec2& value, float speed = 0.1f);
        static void dragVec3(const std::string& label, Math::Vec3& value, float speed = 0.1f);
        static void dragVec4(const std::string& label, Math::Vec4& value, float speed = 0.1f);
        static void colorEdit(const std::string& label, Color& color);
        static void dragInt(const std::string& label, int& value, float speed = 1.0f, float width = 0.0f);
        static bool treeNode(const std::string& label);
        static void treePop();
        static void image(Texture* texture, const Math::Vec2& size);
        static void component(std::type_index componentId, void* componentData);
        static bool selectable(const std::string& label, bool isSelected);
        // returns the width avaible for the current panel
        static float getAvaibleWidth();
        static const UIStyle& getCurrentStyle() { return style; }

        static void demoWindow();

      private:
        UI() = delete;
        ~UI() = delete;

        static float getWidgetHeight(uint16_t textSize = 11);
        static float getTextYOffset(float rowHeight, uint16_t textSize);

        static void setMousePosition(Math::Vec2 pos);
        static void setMouseButtonState(KeyCode button, bool pressed);

        static bool shouldConsumeMouseEvents();
        static bool shouldConsumeKeyboardEvents();

      private:
        static std::unique_ptr<UIRenderer> renderer;

        static UIContext* context;
        static UIStyle style;
        static UIInputState inputState;

        static bool showDebugOutlines;
    };
} // namespace Axiom
