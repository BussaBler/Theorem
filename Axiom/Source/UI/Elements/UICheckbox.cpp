#include "axpch.h"

#include "UICheckbox.h"

#include "Event/MouseEvent.h"

namespace Axiom {
    Math::Vec2 UICheckbox::getDesiredSize(const UIContext& context) {
        desiredSize.x() = 20.0f;
        desiredSize.y() = 20.0f;

        if (fixedSize.x() > 0) {
            desiredSize.x() = fixedSize.x();
        }
        if (fixedSize.y() > 0) {
            desiredSize.y() = fixedSize.y();
        }

        return desiredSize;
    }

    void UICheckbox::onRender(const UIContext& context, const Math::Rect& scissorRect) {
        Color normalColor = overrideNormalColor.value_or(resolvedTheme->controlNormalColor);
        Color hoverColor = overrideHoverColor.value_or(resolvedTheme->controlHoverColor);
        Color activeColor = overrideActiveColor.value_or(resolvedTheme->controlActiveColor);

        bool value = valueGetter && valueGetter();
        Color backgroundColor = isHovered ? hoverColor : normalColor;

        context.renderer->addBasicQuad(arrangedPosition, arrangedSize, backgroundColor, resolvedTheme->borderRadius, context.layer);
        if (value) {
            Math::Vec2 checkSize = arrangedSize * 0.7f;
            Math::Vec2 checkPos = arrangedPosition + (arrangedSize - checkSize) * 0.5f;
            context.renderer->addBasicQuad(checkPos, Math::Vec2(checkSize), activeColor, resolvedTheme->borderRadius * 0.8f, context.layer);
        }
    }

    bool UICheckbox::onEvent(Event& event) {
        if (event.isHandled()) {
            return true;
        }

        EventDispatcher dispatcher(event);

        dispatcher.dispatch<MouseButtonPressedEvent>([this](const MouseButtonPressedEvent& event) {
            if (isHovered && event.getMouseButton() == KeyCode::LeftButton) {
                isActive = true;
                return true;
            }
            return false;
        });

        dispatcher.dispatch<MouseButtonReleasedEvent>([this](const MouseButtonReleasedEvent& event) {
            if (isHovered && event.getMouseButton() == KeyCode::LeftButton) {
                if (isActive && event.getMouseButton() == KeyCode::LeftButton) {
                    isActive = false;
                    if (valueSetter) {
                        bool currentValue = valueGetter ? valueGetter() : false;
                        valueSetter(!currentValue);
                    }
                }
                return true;
            }
            return false;
        });

        return UIElement::onEvent(event);
    }
} // namespace Axiom
