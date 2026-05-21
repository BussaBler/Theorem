#pragma once
#include "Event/Event.h"
#include "Event/MouseEvent.h"
#include "Math/AxMath.h"
#include "UI/UIRenderer.h"

#include <memory>
#include <string>

namespace Axiom {
    enum class UIAlignment { Fill, Start, Center, End };

    struct UIMargin {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
    };

    struct UITheme {
        Color windowBackgroundColor = Color(0.12f, 0.12f, 0.12f, 1.0f);
        Color panelBackgroundColor = Color(0.16f, 0.16f, 0.16f, 1.0f);
        Color headerBackgroundColor = Color(0.10f, 0.10f, 0.10f, 1.0f);

        Color controlNormalColor = Color(0.20f, 0.20f, 0.20f, 1.0f);
        Color controlHoverColor = Color(0.25f, 0.25f, 0.25f, 1.0f);
        Color controlActiveColor = Color(0.18f, 0.18f, 0.18f, 1.0f);

        Color accentColor = Color(0.15f, 0.45f, 0.85f, 1.0f);
        Color errorColor = Color(0.80f, 0.25f, 0.25f, 1.0f);

        Color textColor = Color(0.85f, 0.85f, 0.85f, 1.0f);
        Color textMutedColor = Color(0.60f, 0.60f, 0.60f, 1.0f);
        Color borderColor = Color(0.08f, 0.08f, 0.08f, 1.0f);

        float fontSize = 6.0f;
        Math::Vec4 borderRadius = Math::Vec4(4.0f);

        static std::shared_ptr<UITheme> getDefault() {
            static std::shared_ptr<UITheme> defaultTheme = std::make_shared<UITheme>();
            return defaultTheme;
        }
    };

    struct UIContext {
        UIRenderer* renderer;
        float dpiScale;
        uint8_t layer = 0;
    };

    class UIElement {
        friend class UIContainer;

      public:
        UIElement() = default;
        virtual ~UIElement() = default;

        inline UIElement* getParent() const { return parent; }

        virtual Math::Vec2 getDesiredSize(const UIContext& context) { return desiredSize; }
        virtual void arrange(const UIContext& context, const Math::Vec2& position, const Math::Vec2& size) {
            arrangedPosition = position;
            arrangedSize = size;
        }
        virtual void resolveTheme() {
            if (customTheme) {
                resolvedTheme = customTheme;
            } else if (parent) {
                resolvedTheme = parent->resolvedTheme;
            }
        }

        virtual void onRender(const UIContext& context, const Math::Rect& scissorRect) {}
        virtual bool onEvent(Event& event) {
            if (event.isHandled()) {
                return true;
            }

            EventDispatcher dispatcher(event);
            dispatcher.dispatch<MouseMovedEvent>([this](const MouseMovedEvent& e) {
                float mouseX = e.getMouseX();
                float mouseY = e.getMouseY();

                isHovered = false;
                if (mouseX >= arrangedPosition.x() && mouseX <= arrangedPosition.x() + arrangedSize.x() && mouseY >= arrangedPosition.y() &&
                    mouseY <= arrangedPosition.y() + arrangedSize.y()) {
                    isHovered = true;
                }

                return isHovered;
            });

            return event.isHandled();
        }

        void setID(const std::string& newID) { id = newID; }
        void setMargin(const UIMargin& newMargin) { margin = newMargin; }
        void setPadding(const UIMargin& newPadding) { padding = newPadding; }
        void setTheme(std::shared_ptr<UITheme> newTheme) { customTheme = newTheme; }
        void setHorizontalAlignment(UIAlignment alignment) { horizontalAlignment = alignment; }
        void setVerticalAlignment(UIAlignment alignment) { verticalAlignment = alignment; }
        void setFixedSize(const Math::Vec2& size) { fixedSize = size; }

        const std::string& getID() const { return id; }
        const UIMargin& getMargin() const { return margin; }
        const UIMargin& getPadding() const { return padding; }
        UIAlignment getHorizontalAlignment() const { return horizontalAlignment; }
        UIAlignment getVerticalAlignment() const { return verticalAlignment; }
        Math::Vec2 getArrangedPosition() const { return arrangedPosition; }
        Math::Vec2 getArrangedSize() const { return arrangedSize; }
        const Math::Vec2& getFixedSize() const { return fixedSize; }
        const std::shared_ptr<UITheme>& getTheme() const { return resolvedTheme; }

      protected:
        UIElement* parent = nullptr;

        std::string id;

        UIMargin margin;
        UIMargin padding;

        UIAlignment horizontalAlignment = UIAlignment::Fill;
        UIAlignment verticalAlignment = UIAlignment::Fill;

        Math::Vec2 arrangedPosition;
        Math::Vec2 arrangedSize;

        Math::Vec2 desiredSize = Math::Vec2::zero();
        Math::Vec2 fixedSize = Math::Vec2(-1.0f);

        std::shared_ptr<UITheme> resolvedTheme = UITheme::getDefault();

        bool isHovered = false;

      private:
        std::shared_ptr<UITheme> customTheme = nullptr;
    };

} // namespace Axiom
