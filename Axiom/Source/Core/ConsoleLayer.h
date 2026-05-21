#pragma once
#include "Event/KeyEvent.h"
#include "Layer.h"
#include "UI/Elements/UICanvas.h"
#include "UI/Elements/UIScrollBox.h"

#include <memory>
#include <string>

namespace Axiom {
    class ConsoleLayer : public Layer {
      public:
        ConsoleLayer() : Layer("ConsoleLayer") {}
        ~ConsoleLayer() = default;

        void onAttach() override;
        void onDetach() override;
        void onUpdate() override;
        void onEvent(Event& event) override;
        void onUIRender() override;
        void onRender(CommandBuffer* commandBuffer) override;

      private:
        void refreshConsoleHistory();

        bool onKeyPressed(KeyPressedEvent& event);

      private:
        static constexpr float CONSOLE_HEIGHT_RATIO = 0.4f;

        bool isOpen = false;

        UIContext mainUiContext;
        std::shared_ptr<UICanvas> uiRoot;
        std::shared_ptr<UIScrollBox> consoleScrollBox;
        bool shouldRefreshHistory = false;

        std::string consoleInputBuffer;
    };
} // namespace Axiom
