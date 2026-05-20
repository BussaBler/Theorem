#pragma once
#include "Event/KeyEvent.h"
#include "Event/MouseEvent.h"
#include "UIElement.h"

namespace Axiom {
    enum class ScalarFieldState { Normal, Hovered, Active, Typing };

    template <typename T> class UIScalarField : public UIElement {
      public:
        Math::Vec2 getDesiredSize(const UIContext& context) override;

        void onRender(const UIContext& context, const Math::Rect& scissorRect) override;
        bool onEvent(Event& event) override;

        void setDragSpeed(float speed) { dragSpeed = speed; }
        void setNormalColor(const Color& color) { overrideNormalColor = color; }
        void setHoverColor(const Color& color) { overrideHoverColor = color; }
        void setActiveColor(const Color& color) { overrideActiveColor = color; }
        void setLimits(T min, T max) {
            minLimit = min;
            maxLimit = max;
        }
        void setValueGetter(std::function<T()> getter) { getValue = std::move(getter); }
        void setValueSetter(std::function<void(T)> setter) { setValue = std::move(setter); }
        void setFontSize(float fontSize) { overrideFontSize = fontSize; }

      private:
        void commitTyping();

      private:
        ScalarFieldState state = ScalarFieldState::Normal;

        float dragSpeed = 0.1f;
        std::optional<Color> overrideNormalColor;
        std::optional<Color> overrideHoverColor;
        std::optional<Color> overrideActiveColor;

        bool isActive = false;

        T minLimit = std::numeric_limits<T>::lowest();
        T maxLimit = std::numeric_limits<T>::max();

        float lastMouseX = 0.0f;

        std::function<void(T)> setValue = nullptr;
        std::function<T()> getValue = nullptr;

        std::optional<float> overrideFontSize;

        std::string buffer;
    };

    template <typename T> inline Math::Vec2 UIScalarField<T>::getDesiredSize(const UIContext& context) {
        desiredSize.x() = 60.0f + padding.left + padding.right + margin.left + margin.right;
        desiredSize.y() = 20.0f + padding.top + padding.bottom + margin.top + margin.bottom;

        if (fixedSize.x() > 0) {
            desiredSize.x() = fixedSize.x();
        }
        if (fixedSize.y() > 0) {
            desiredSize.y() = fixedSize.y();
        }

        return desiredSize;
    }

    template <typename T> inline void UIScalarField<T>::onRender(const UIContext& context, const Math::Rect& scissorRect) {
        Color normalColor = overrideNormalColor.value_or(resolvedTheme->controlNormalColor);
        Color hoverColor = overrideHoverColor.value_or(resolvedTheme->controlHoverColor);
        Color activeColor = overrideActiveColor.value_or(resolvedTheme->controlActiveColor);
        Color backgroundColor = isActive ? activeColor : (isHovered ? hoverColor : normalColor);
        context.renderer->addBasicQuad(arrangedPosition, arrangedSize, backgroundColor, resolvedTheme->borderRadius, context.layer);

        std::string displayText;
        if (state == ScalarFieldState::Typing) {
            displayText = buffer + "|";
        } else {
            if constexpr (std::is_floating_point_v<T>) {
                displayText = std::format("{:.2f}", getValue());
            } else {
                displayText = std::to_string(getValue());
            }
        }

        float fontSize = overrideFontSize.value_or(resolvedTheme->fontSize);

        float textWidth = context.renderer->calculateTextWidth(displayText, fontSize, context.dpiScale);
        float textHeight = context.renderer->calculateTextHeight(fontSize, context.dpiScale);

        float textX = arrangedPosition.x() + (arrangedSize.x() - textWidth) / 2.0f;
        float textY = arrangedPosition.y() + (arrangedSize.y() - textHeight) / 2.0f;

        context.renderer->addText(displayText, Math::Vec2(textX, textY), fontSize, context.dpiScale, resolvedTheme->textColor, context.layer);

        UIElement::onRender(context, scissorRect);
    }

    template <typename T> inline bool UIScalarField<T>::onEvent(Event& event) {
        if (event.isHandled()) {
            return true;
        }

        EventDispatcher dispatcher(event);

        dispatcher.dispatch<MouseButtonPressedEvent>([this](const MouseButtonPressedEvent& event) {
            if (event.getMouseButton() == KeyCode::LeftButton) {
                if (isHovered) {
                    isActive = true;
                    lastMouseX = Input::getMouseX();
                    return true;
                } else {
                    if (state == ScalarFieldState::Typing) {
                        commitTyping();
                    }
                }
            }
            return false;
        });

        dispatcher.dispatch<MouseMovedEvent>([this](const MouseMovedEvent& event) {
            float mx = event.getMouseX();
            float my = event.getMouseY();

            if (isActive && state != ScalarFieldState::Typing) {
                float deltaX = mx - lastMouseX;
                if (deltaX != 0.0f && getValue && setValue) {
                    T newValue = getValue() + static_cast<T>(deltaX * dragSpeed);

                    newValue = std::clamp(newValue, minLimit, maxLimit);

                    setValue(newValue);
                }
                lastMouseX = mx;
                return true;
            }
            return false;
        });

        dispatcher.dispatch<MouseButtonReleasedEvent>([this](const MouseButtonReleasedEvent& event) mutable -> bool {
            if (isActive && event.getMouseButton() == KeyCode::LeftButton) {
                isActive = false;
                if (isHovered) {
                    state = ScalarFieldState::Typing;

                    if constexpr (std::is_floating_point_v<T>) {
                        buffer = std::format("{:.2f}", getValue());
                    } else {
                        buffer = std::to_string(getValue());
                    }
                }
                return true;
            }
            return false;
        });

        dispatcher.dispatch<KeyPressedEvent>([this](const KeyPressedEvent& event) {
            if (state == ScalarFieldState::Typing) {
                if (event.getKeyCode() == KeyCode::Return) {
                    commitTyping();
                    return true;
                } else if (event.getKeyCode() == KeyCode::Escape) {
                    state = ScalarFieldState::Normal;
                    buffer.clear();
                    return true;
                } else if (event.getKeyCode() == KeyCode::Backspace) {
                    if (!buffer.empty()) {
                        buffer.pop_back();
                    }
                    return true;
                }
            }
            return false;
        });

        dispatcher.dispatch<KeyTypedEvent>([this](const KeyTypedEvent& event) {
            if (state == ScalarFieldState::Typing) {
                char typedChar = event.getKeyChar();
                if (std::isdigit(typedChar)) {
                    buffer += typedChar;
                } else if (typedChar == '-' && buffer.empty()) {
                    buffer += typedChar;
                } else if constexpr (std::is_floating_point_v<T>) {
                    if (typedChar == '.' && buffer.find('.') == std::string::npos) {
                        buffer += typedChar;
                    }
                }
                return true;
            }
            return false;
        });

        return UIElement::onEvent(event);
    }

    template <typename T> inline void UIScalarField<T>::commitTyping() {
        if (state != ScalarFieldState::Typing) {
            return;
        }

        try {
            T newValue;
            if constexpr (std::is_floating_point_v<T>) {
                newValue = static_cast<T>(std::stof(buffer));
            } else {
                newValue = static_cast<T>(std::stoi(buffer));
            }

            newValue = std::clamp(newValue, minLimit, maxLimit);

            if (setValue) {
                setValue(newValue);
            }
        } catch (...) {
        }
        state = ScalarFieldState::Normal;
        buffer.clear();
    }
} // namespace Axiom