#include "TheoremApplication.h"

#include "ProjectHubLayer.h"

TheoremApplication::TheoremApplication(const Axiom::ApplicationInfo& appInfo) : Axiom::Application(appInfo) {
    if (!appInfo.applicationWorkingDirectory.empty()) {
        pushLayer<EditorLayer>();
    } else {
        pushLayer<ProjectHubLayer>();
    }
    pushOverlay<Axiom::ConsoleLayer>();
}

TheoremApplication::~TheoremApplication() {
}

Axiom::Application* Axiom::createApplication(int argc, char** argv, const std::filesystem::path& axiomRootDir) {
    Axiom::ApplicationInfo appInfo = {
        .name = "Theorem", .engineWorkingDirectory = axiomRootDir, .applicationWorkingDirectory = "", .width = 360, .height = 720};
    if (argc > 1) {
        appInfo.applicationWorkingDirectory = std::filesystem::path(argv[1]);
        appInfo.width = 1280;
        appInfo.height = 720;
    }
    return new TheoremApplication(appInfo);
}
