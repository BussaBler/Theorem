#include "axpch.h"

#include "ConsoleLayer.h"

#include "CommandRegistry.h"
#include "UI/Elements/UIPanel.h"
#include "UI/Elements/UIText.h"
#include "UI/Elements/UITextInput.h"
#include "UI/Elements/UIVerticalBox.h"
#include "Window.h"

namespace Axiom {
    void ConsoleLayer::onAttach() {
        AX_CORE_LOG_DEBUG("ConsoleLayer attached");
        mainUiContext = {
            .renderer = Locator::getUIRenderer(),
            .dpiScale = Locator::getWindow()->getWindowDPI() / 96.0f,
            .layer = 0,
        };
        uiRoot = std::make_shared<UICanvas>();

        auto consolePanel = std::make_shared<UIPanel>();
        Color panelColor = Color::darkGray();
        panelColor.a() = 0.8f;
        consolePanel->setBackgroundColor(panelColor);
        consolePanel->setPadding({10.0f, 10.0f, 10.0f, 10.0f});
        consolePanel->setHorizontalAlignment(UIAlignment::Fill);
        consolePanel->setVerticalAlignment(UIAlignment::Fill);

        auto verticalBox = std::make_shared<UIVerticalBox>();
        verticalBox->setHorizontalAlignment(UIAlignment::Fill);
        verticalBox->setVerticalAlignment(UIAlignment::Fill);

        auto consoleText = std::make_shared<UIText>("Console");
        consoleText->setHorizontalAlignment(UIAlignment::Start);
        consoleText->setVerticalAlignment(UIAlignment::Start);
        consoleText->setMargin({0.0f, 0.0f, 0.0f, 5.0f});
        verticalBox->addChild(consoleText);

        consoleScrollBox = std::make_shared<UIScrollBox>();
        consoleScrollBox->setHorizontalAlignment(UIAlignment::Fill);
        consoleScrollBox->setVerticalAlignment(UIAlignment::Fill);
        verticalBox->addChild(consoleScrollBox);

        auto consoleInput = std::make_shared<UITextInput>();
        consoleInput->setHorizontalAlignment(UIAlignment::Fill);
        consoleInput->setVerticalAlignment(UIAlignment::End);
        consoleInput->setValueGetter([this]() { return ""; });
        consoleInput->setValueSetter([this](const std::string& input) {
            consoleInputBuffer = input;
            CommandRegistry::executeCommand(input);
            consoleInputBuffer.clear();
            shouldRefreshHistory = true;
        });
        verticalBox->addChild(consoleInput);

        consolePanel->addChild(verticalBox);
        uiRoot->addChild(consolePanel);
    }

    void ConsoleLayer::onDetach() {
    }

    void ConsoleLayer::onUpdate() {
        if (isOpen) {
            Math::Vec2 winSize = Math::Vec2(Locator::getWindow()->getWidth(), Locator::getWindow()->getHeight());
            uiRoot->arrange(mainUiContext, Math::Vec2(0, winSize.y() * (1.0f - CONSOLE_HEIGHT_RATIO)),
                            Math::Vec2(winSize.x(), winSize.y() * CONSOLE_HEIGHT_RATIO));
        }
    }

    void ConsoleLayer::onEvent(Event& event) {
        EventDispatcher dispatcher(event);

        dispatcher.dispatch<KeyPressedEvent>(std::bind(&ConsoleLayer::onKeyPressed, this, std::placeholders::_1));

        if (isOpen && !event.isHandled()) {
            event.handled |= uiRoot->onEvent(event);
            if ((event.getCategoryFlags() & EventCategory::EventCategoryApplicationInput) != EventCategory::Empty) {
                event.handled = true;
            }
        }
    }

    void ConsoleLayer::onUIRender() {
        if (isOpen) {
            Math::Rect logicalScreen({0, 0}, {Locator::getWindow()->getWidth(), Locator::getWindow()->getHeight()});
            mainUiContext.renderer->pushScissorRect(logicalScreen, mainUiContext.layer);
            uiRoot->onRender(mainUiContext, logicalScreen);
            mainUiContext.renderer->popScissorRect(mainUiContext.layer);
        }
    }

    void ConsoleLayer::onRender(CommandBuffer* commandBuffer) {
        if (shouldRefreshHistory) {
            refreshConsoleHistory();
            shouldRefreshHistory = false;
        }
    }

    void ConsoleLayer::refreshConsoleHistory() {
        consoleScrollBox->clearChildren();
        for (const auto& entry : CommandRegistry::getCommandHistory()) {
            auto text = std::make_shared<UIText>(entry);
            text->setHorizontalAlignment(UIAlignment::Fill);
            text->setVerticalAlignment(UIAlignment::Start);
            text->setMargin({0.0f, 0.0f, 0.0f, 2.0f});
            consoleScrollBox->addChild(text);
        }
    }

    bool ConsoleLayer::onKeyPressed(KeyPressedEvent& event) {
        if (event.getKeyCode() == KeyCode::Grave) {
            isOpen = !isOpen;
            return true;
        }

        return false;
    }
} // namespace Axiom
