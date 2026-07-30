#pragma once
#include "Assert.h"
#include "Asset/AssetManager.h"
#include "Event/ApplicationEvent.h"
#include "Event/KeyEvent.h"
#include "Event/MouseEvent.h"
#include "Input.h"
#include "LayerStack.h"
#include "Locator.h"
#include "Log.h"
#include "Math/AxMath.h"
#include "Renderer/RenderGraph.h"
#include "Renderer/Renderer.h"
#include "Scene/Components/ComponentReflection.h"
#include "UI/UIRenderer.h"
#include "Utils/FileSystem.h"
#include "Window.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Axiom {
    struct ApplicationInfo {
        std::string name;
        std::filesystem::path workingDirectory;
    };

    class Application {
      public:
        Application(const ApplicationInfo& appInfo);
        virtual ~Application();

        void run();
        void onEvent(Event& event);

        template <typename T>
            requires std::derived_from<T, Layer>
        void pushLayer() {
            layerStack.pushLayer<T>();
        }
        template <typename T>
            requires std::derived_from<T, Layer>
        void pushOverlay() {
            layerStack.pushOverlay<T>();
        }
        template <typename T>
            requires std::derived_from<T, Layer>
        void popLayer() {
            layerStack.popLayer<T>();
        }
        template <typename T>
            requires std::derived_from<T, Layer>
        void popOverlay() {
            layerStack.popOverlay<T>();
        }

        void queueLayerAction(Layer* requester, std::unique_ptr<Layer> newLayer, Layer::ActionType action);

        inline static Application* get() { return instance; }

      private:
        bool onWindowClose(WindowCloseEvent& e);
        bool onWindowResize(WindowResizeEvent& e);
        bool onKeyPressed(KeyPressedEvent& e);
        bool onKeyReleased(KeyReleasedEvent& e);
        bool onMouseButtonPressed(MouseButtonPressedEvent& e);
        bool onMouseButtonReleased(MouseButtonReleasedEvent& e);
        bool onMouseMoved(MouseMovedEvent& e);

        void processLayerActions();

      private:
        struct LayerAction {
            Layer* requester;
            std::unique_ptr<Layer> newLayer;
            Layer::ActionType action;
        };

      private:
        std::unique_ptr<Window> window;
        std::unique_ptr<Renderer> renderer;
        std::unique_ptr<UIRenderer> uiRenderer;
        bool running = true;
        LayerStack layerStack;
        std::vector<LayerAction> layerActionQueue;
        RenderGraph renderGraph{};

      private:
        static Application* instance;
    };

    Application* createApplication(const ApplicationInfo& appInfo);
} // namespace Axiom
