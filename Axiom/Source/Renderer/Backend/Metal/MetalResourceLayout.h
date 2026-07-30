#pragma once
#include "Renderer/ResourceLayout.h"

#include <vector>

namespace Axiom {
    class MetalResourceLayout : public ResourceLayout {
      public:
        MetalResourceLayout(const std::vector<BindingCreateInfo>& bindingsCreateInfo);
        ~MetalResourceLayout() override = default;

        const std::vector<BindingCreateInfo>& getBindingsCreateInfo() const { return bindingsCreateInfo; }

      private:
        std::vector<BindingCreateInfo> bindingsCreateInfo;
    };
} // namespace Axiom
