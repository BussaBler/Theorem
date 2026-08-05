#pragma once
#include "Application.h"

extern Axiom::Application* Axiom::createApplication(const Axiom::ApplicationInfo& appInfo);

int main(int argc, char** argv) {
    auto app = Axiom::createApplication({"Axiom Application", AX_ROOT_DIR});
    app->run();
    delete app;
#ifdef AX_DEBUG
    std::cin.get();
#endif
}
