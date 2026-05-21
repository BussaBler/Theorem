#include "axpch.h"

#include "Application.h"

#include "Profiler.h"

namespace Axiom {
    Application* Application::instance = nullptr;

    Application::Application(const ApplicationInfo& appInfo) {
        Log::init();
        FileSystem::setWorkingDirectory(appInfo.workingDirectory);
        AX_CORE_ASSERT(Log::getCoreLogger()->outputToFile(), "Failed to create log file!");

        AX_CORE_LOG_INFO("Application started: {0}", appInfo.name);
        AX_CORE_LOG_INFO("Current working directory: {0}", FileSystem::getWorkingDirectory().string());

        AX_CORE_ASSERT(!instance, "Application already exists!");
        instance = this;

        WindowProps windowProps{appInfo.name, 1280, 720};
        window = Window::create(windowProps);
        Locator::provideWindow(window.get());
        window->setEventCallback(std::bind(&Application::onEvent, this, std::placeholders::_1));

        renderer = std::make_unique<Renderer>(window.get());
        Locator::provideRenderer(renderer.get());

        AssetManager::init();

        uiRenderer = std::make_unique<UIRenderer>();
        Locator::provideUIRenderer(uiRenderer.get());

        ComponentReflection::init();
    }

    Application::~Application() {
        AssetManager::shutdown();
    }

    void Application::onEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<WindowCloseEvent>(std::bind(&Application::onWindowClose, this, std::placeholders::_1));
        dispatcher.dispatch<WindowResizeEvent>(std::bind(&Application::onWindowResize, this, std::placeholders::_1));

        dispatcher.dispatch<KeyPressedEvent>(std::bind(&Application::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<KeyReleasedEvent>(std::bind(&Application::onKeyReleased, this, std::placeholders::_1));
        dispatcher.dispatch<MouseButtonPressedEvent>(std::bind(&Application::onMouseButtonPressed, this, std::placeholders::_1));
        dispatcher.dispatch<MouseButtonReleasedEvent>(std::bind(&Application::onMouseButtonReleased, this, std::placeholders::_1));
        dispatcher.dispatch<MouseMovedEvent>(std::bind(&Application::onMouseMoved, this, std::placeholders::_1));

        for (auto it = layerStack.rbegin(); it != layerStack.rend(); it++) {
            if (event.isHandled()) {
                break;
            }
            (*it)->onEvent(event);
        }
    }

    void Application::queueLayerAction(Layer* requester, std::unique_ptr<Layer> newLayer, Layer::ActionType action) {
        layerActionQueue.emplace_back(LayerAction{requester, std::move(newLayer), action});
    }

    bool Application::onWindowClose(WindowCloseEvent& e) {
        running = false;
        return true;
    }

    bool Application::onWindowResize(WindowResizeEvent& e) {
        if (e.getWidth() == 0 || e.getHeight() == 0) {
            return false;
        }
        renderer->recreateSwapChain();
        return true;
    }

    bool Application::onKeyPressed(KeyPressedEvent& e) {
        Input::setKeyPressed(e.getKeyCode(), true);
        return false;
    }

    bool Application::onKeyReleased(KeyReleasedEvent& e) {
        Input::setKeyPressed(e.getKeyCode(), false);
        return false;
    }

    bool Application::onMouseButtonPressed(MouseButtonPressedEvent& e) {
        Input::setMouseButtonPressed(e.getMouseButton(), true);
        return false;
    }

    bool Application::onMouseButtonReleased(MouseButtonReleasedEvent& e) {
        Input::setMouseButtonPressed(e.getMouseButton(), false);
        return false;
    }

    bool Application::onMouseMoved(MouseMovedEvent& e) {
        Input::setMousePosition(e.getMouseX(), e.getMouseY());
        return false;
    }

    void Application::processLayerActions() {
        for (auto& layerAction : layerActionQueue) {

            auto it = std::find_if(layerStack.begin(), layerStack.end(),
                                   [&layerAction](const std::unique_ptr<Layer>& layerPtr) { return layerPtr.get() == layerAction.requester; });

            if (it == layerStack.end()) {
                AX_CORE_LOG_ERROR("Layer action requester not found in layer stack!");
                continue;
            }

            switch (layerAction.action) {
            case Layer::ActionType::Transition:
                (*it)->onDetach();
                layerAction.newLayer->onAttach();

                *it = std::move(layerAction.newLayer);
                break;

            case Layer::ActionType::Suspend:
                (*it)->onSuspend();
                (*it)->isSuspended = true;
                layerAction.newLayer->onAttach();

                layerStack.insertLayer(it + 1, std::move(layerAction.newLayer));
                break;

            case Layer::ActionType::Pop:
                (*it)->onDetach();

                if (it != layerStack.begin()) {
                    auto prevIt = std::prev(it);
                    (*prevIt)->isSuspended = false;
                    (*prevIt)->onResume();
                }
                layerStack.eraseLayer(it);
                break;
            }
        }

        layerActionQueue.clear();
    }

    void Application::run() {
        while (running) {
            Profiler::beginFrame();
            window->beginFrame();
            for (const auto& layer : layerStack) {
                layer->onUpdate();
            }

            uiRenderer->beginFrame();
            for (const auto& layer : layerStack) {
                layer->onUIRender();
            }

            CommandBuffer* commandBuffer = renderer->beginFrame();
            for (const auto& layer : layerStack) {
                layer->onRender(commandBuffer);
            }
            uiRenderer->onRender(commandBuffer, renderer->getCurrentRenderTarget());
            renderer->endFrame();

            window->onUpdate();
            window->endFrame();
            processLayerActions();
        }
        renderer->waitIdle();
    }
} // namespace Axiom
