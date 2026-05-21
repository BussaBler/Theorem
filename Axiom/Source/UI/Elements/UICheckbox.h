#pragma once
#include "UIElement.h"

#include <functional>
#include <optional>

namespace Axiom {
    class UICheckbox : public UIElement {
      public:
        Math::Vec2 getDesiredSize(const UIContext& context) override;

        void onRender(const UIContext& context, const Math::Rect& scissorRect) override;
        bool onEvent(Event& event) override;

        void setNormalColor(const Color& color) { overrideNormalColor = color; }
        void setHoverColor(const Color& color) { overrideHoverColor = color; }
        void setActiveColor(const Color& color) { overrideActiveColor = color; }
        void setValueGetter(std::function<bool()> getter) { valueGetter = std::move(getter); }
        void setValueSetter(std::function<void(bool)> setter) { valueSetter = std::move(setter); }

      private:
        bool isActive = false;

        std::optional<Color> overrideNormalColor;
        std::optional<Color> overrideHoverColor;
        std::optional<Color> overrideActiveColor;

        std::function<bool()> valueGetter;
        std::function<void(bool)> valueSetter;
    };
} // namespace Axiom
