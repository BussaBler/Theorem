#include "axpch.h"

#include "UIButton.h"

#include "Event/MouseEvent.h"

namespace Axiom {
    Math::Vec2 UIButton::getDesiredSize(const UIContext& context) {
        float fontSize = overrideFontSize.value_or(getTheme()->fontSize);
        textWidth = context.renderer->calculateTextWidth(text, fontSize, context.dpiScale);
        textHeight = context.renderer->calculateTextHeight(fontSize, context.dpiScale);
        desiredSize.x() = textWidth + padding.left + padding.right + margin.left + margin.right;
        desiredSize.y() = textHeight + padding.top + padding.bottom + margin.top + margin.bottom;

        if (fixedSize.x() > 0) {
            desiredSize.x() = fixedSize.x();
        }
        if (fixedSize.y() > 0) {
            desiredSize.y() = fixedSize.y();
        }
        return desiredSize;
    }

    void UIButton::onRender(const UIContext& context, const Math::Rect& scissorRect) {
        Color normalColor = overrideNormalColor.value_or(resolvedTheme->controlNormalColor);
        Color hoverColor = overrideHoverColor.value_or(resolvedTheme->controlHoverColor);
        Color activeColor = overrideActiveColor.value_or(resolvedTheme->controlActiveColor);
        Color backgroundColor = isActive ? activeColor : (isHovered ? hoverColor : normalColor);
        float fontSize = overrideFontSize.value_or(getTheme()->fontSize);

        context.renderer->addBasicQuad(arrangedPosition, arrangedSize, backgroundColor, resolvedTheme->borderRadius, context.layer);

        float textX = arrangedPosition.x() + (arrangedSize.x() - textWidth) / 2.0f;
        float textY = arrangedPosition.y() + (arrangedSize.y() - textHeight) / 2.0f;

        context.renderer->addText(text, Math::Vec2(textX, textY), fontSize, context.dpiScale, getTheme()->textColor, context.layer);

        UIElement::onRender(context, scissorRect);
    }

    bool UIButton::onEvent(Event& event) {
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
            if (isActive && event.getMouseButton() == KeyCode::LeftButton) {
                isActive = false;
                if (isHovered && onClick) {
                    onClick();
                }
                return true;
            }
            return false;
        });

        return UIElement::onEvent(event);
    }
} // namespace Axiom
