#pragma once
#include "UIContainer.h"

#include <string>

namespace Axiom {
    class UICollapsableGroup : public UIContainer {
      public:
        explicit UICollapsableGroup(const std::string& title) : title(title) {}
        ~UICollapsableGroup() = default;

        Math::Vec2 getDesiredSize(const UIContext& context) override;
        void arrange(const UIContext& context, const Math::Vec2& position, const Math::Vec2& size) override;
        void onRender(const UIContext& context, const Math::Rect& scissorRect) override;
        bool onEvent(Event& event) override;

      private:
        static constexpr float HEADER_HEIGHT = 24.0f;

        std::string title;
        bool isHeaderHovered = false;
        bool isActive = false;
        bool isOpen = false;
    };
} // namespace Axiom
