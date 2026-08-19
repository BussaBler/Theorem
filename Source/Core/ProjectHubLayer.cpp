#include "ProjectHubLayer.h"

#include "Core/Log.h"
#include "EditorLayer.h"
#include "Project.h"
#include "Utils/FileSystem.h"

#include <filesystem>
#include <string>
#include <vector>

ProjectHubLayer::ProjectHubLayer() : Axiom::Layer("ProjectHubLayer") {
}

void ProjectHubLayer::onAttach() {
    Axiom::AX_LOG_INFO("Project Hub Layer Attached");
    hubUiContext = {.renderer = Axiom::Locator::getUIRenderer(), .dpiScale = Axiom::Locator::getWindow()->getWindowDPI() / 96.0f, .layer = 0};
    buildUI();
}

void ProjectHubLayer::onDetach() {
    Axiom::AX_LOG_INFO("Project Hub Layer Detached");
}

void ProjectHubLayer::onUpdate() {
    float winWidth = Axiom::Locator::getWindow()->getWidth();
    float winHeight = Axiom::Locator::getWindow()->getHeight();
    uiRoot->arrange(hubUiContext, Math::Vec2::zero(), Math::Vec2(winWidth, winHeight));
}

void ProjectHubLayer::onUIRender() {
    hubUiContext.renderer->pushScissorRect({{0, 0}, {Axiom::Locator::getWindow()->getWidth(), Axiom::Locator::getWindow()->getHeight()}}, hubUiContext.layer);
    uiRoot->onRender(hubUiContext, {{0, 0}, {Axiom::Locator::getWindow()->getWidth(), Axiom::Locator::getWindow()->getHeight()}});
    hubUiContext.renderer->popScissorRect(hubUiContext.layer);
}

void ProjectHubLayer::onEvent(Axiom::Event& event) {
    uiRoot->onEvent(event);
}

void ProjectHubLayer::onRender(Axiom::RenderGraph& renderGraph) {
}

void ProjectHubLayer::buildUI() {
    uiRoot = std::make_shared<Axiom::UIContainer>();
    projectListPanel = std::make_shared<Axiom::UIVerticalBox>();

    for (const auto& project : recentProjects) {
        std::shared_ptr<Axiom::UIButton> projectBtn = std::make_shared<Axiom::UIButton>(project.name);
        projectBtn->setOnClick([this, project]() { Axiom::AX_LOG_INFO("OPEN PROJECT"); });
        projectListPanel->addChild(projectBtn);
    }

    std::shared_ptr<Axiom::UIButton> browseBtn = std::make_shared<Axiom::UIButton>("Browse files...");
    browseBtn->setOnClick([this]() {
        std::optional<std::filesystem::path> selectedFolder = Axiom::FileDialogs::openFolder("Select a Project Folder");
        if (selectedFolder.has_value()) {
            Axiom::AX_LOG_INFO("Selected Project folder: {}", selectedFolder->string());
            openProject(selectedFolder.value());
        }
    });
    projectListPanel->addChild(browseBtn);

    uiRoot->addChild(projectListPanel);
}

void ProjectHubLayer::loadRecentProjects() {
}

void ProjectHubLayer::openProject(const std::filesystem::path& projectPath) {
    Axiom::FileSystem::mount("project://", projectPath);

    if (!Axiom::FileSystem::exists("project://Scenes")) {
        Axiom::FileSystem::createDirectory("project://Scenes");
    }
    if (!Axiom::FileSystem::exists("project://Assets")) {
        Axiom::FileSystem::createDirectory("project://Assets");
    }

    std::vector<Axiom::FileInfo> files = Axiom::FileSystem::getDirectory(projectPath);
    std::filesystem::path projectFilePath = "";
    for (const auto& file : files) {
        if (file.isDirectory) {
            continue;
        }
        if (file.extension.compare(".theorem") == 0) {
            projectFilePath = "project://" + file.name;
            break;
        }
    }

    if (!projectFilePath.empty()) {
        Axiom::AX_LOG_INFO("Loading project: {}", projectFilePath.string());
        Project::load(projectFilePath);
    } else {
        Axiom::AX_LOG_INFO("Initializing a new project...");
        std::string defaultName = projectPath.filename().string();
        Project::createNew(defaultName, "project://");
    }

    std::filesystem::path manifestPath = "project://ProjectManifest.json";
    if (Axiom::FileSystem::exists(manifestPath)) {
        Axiom::AX_LOG_INFO("Loading project assets...");
        Axiom::AssetManager::deserializeManifest(manifestPath);
    } else {
        Axiom::AX_LOG_INFO("Creating a new project asset manifest");
    }

    transitionTo<EditorLayer>();
}
