#pragma once
#include "UIElement.h"

#include <vector>

namespace Axiom {
    class UIContainer : public UIElement {
      public:
        void addChild(std::shared_ptr<UIElement> child) {
            child->parent = this;
            children.push_back(child);
        }
        void removeChild(std::shared_ptr<UIElement> child) {
            child->parent = nullptr;
            children.erase(std::remove(children.begin(), children.end(), child), children.end());
        }
        void clearChildren() {
            for (const auto& child : children) {
                child->parent = nullptr;
            }
            children.clear();
        }

        inline const std::vector<std::shared_ptr<UIElement>>& getChildren() const { return children; }

        virtual void arrange(const UIContext& context, const Math::Vec2& position, const Math::Vec2& size) override {
            UIElement::arrange(context, position, size);
            for (const auto& child : children) {
                child->arrange(context, position, size);
            }
        }

        void resolveTheme() override {
            UIElement::resolveTheme();
            for (const auto& child : children) {
                child->resolveTheme();
            }
        }

        virtual void onRender(const UIContext& context, const Math::Rect& scissorRect) override {
            for (const auto& child : children) {
                child->onRender(context, scissorRect);
            }
        }

        virtual bool onEvent(Event& event) override {
            if (event.isHandled()) {
                return true;
            }

            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                if ((*it)->onEvent(event)) {
                    return true;
                }
            }

            return UIElement::onEvent(event);
        }

      protected:
        std::vector<std::shared_ptr<UIElement>> children;
    };
} // namespace Axiom
