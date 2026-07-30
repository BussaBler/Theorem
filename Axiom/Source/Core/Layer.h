#pragma once
#include "Event/Event.h"
#include "Renderer/RenderGraph.h"

namespace Axiom {
    class Layer {
      public:
        Layer(const char* name = "Layer") : debugName(name) {}
        virtual ~Layer() = default;
        virtual void onAttach() {}
        virtual void onDetach() {}
        virtual void onUpdate() {}
        virtual void onEvent(Event& event) {}
        virtual void onUIRender() {}
        virtual void onRender(RenderGraph& renderGraph) {}
        virtual void onSuspend() {}
        virtual void onResume() {}

        const std::string& getName() const { return debugName; }

      protected:
        template <typename T, typename... Args> void transitionTo(Args&&... args) {
            requestLayerAction(std::make_unique<T>(std::forward<Args>(args)...), ActionType::Transition);
        }
        template <typename T, typename... Args> void suspendTo(Args&&... args) {
            requestLayerAction(std::make_unique<T>(std::forward<Args>(args)...), ActionType::Suspend);
        }
        void pop() { requestLayerAction(nullptr, ActionType::Pop); }

      public:
        enum class ActionType { Transition, Suspend, Pop };

      private:
        void requestLayerAction(std::unique_ptr<Layer> newLayer, ActionType action);

      public:
        bool isSuspended = false;

      protected:
        std::string debugName;
    };
} // namespace Axiom
