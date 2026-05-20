#pragma once
#include "UIElement.h"

namespace Axiom {
    class UIButton : public UIElement {
      public:
        UIButton(const std::string& text) : text(text) {}

        virtual Math::Vec2 getDesiredSize(const UIContext& context) override;

        virtual void onRender(const UIContext& context, const Math::Rect& scissorRect) override;
        virtual bool onEvent(Event& event) override;

        inline virtual void setNormalColor(const Color& color) { overrideNormalColor = color; }
        inline virtual void setHoverColor(const Color& color) { overrideHoverColor = color; }
        inline virtual void setActiveColor(const Color& color) { overrideActiveColor = color; }
        inline virtual void setOnClick(std::function<void()> callback) { onClick = std::move(callback); }
        inline virtual void setText(const std::string& newText) { text = newText; }
        inline virtual void setFontSize(float fontSize) { overrideFontSize = fontSize; }

      private:
        std::string text;
        float textWidth = 0.0f;
        float textHeight = 0.0f;

        std::optional<Color> overrideNormalColor;
        std::optional<Color> overrideHoverColor;
        std::optional<Color> overrideActiveColor;

        bool isActive = false;

        std::function<void()> onClick;

        std::optional<float> overrideFontSize;
    };
} // namespace Axiom