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
    uiRoot = std::make_shared<Axiom::UICanvas>();

    auto background = std::make_shared<Axiom::UIPanel>();
    background->setBackgroundColor(uiRoot->getTheme()->windowBackgroundColor);
    uiRoot->addChild(background);

    auto hubWindow = std::make_shared<Axiom::UIVerticalBox>();
    hubWindow->setHorizontalAlignment(Axiom::UIAlignment::Center);
    hubWindow->setVerticalAlignment(Axiom::UIAlignment::Center);
    hubWindow->setFixedSize({500.0f, -1.0f});
    background->addChild(hubWindow);

    auto titleText = std::make_shared<Axiom::UIText>("Theorem Engine");
    titleText->setHorizontalAlignment(Axiom::UIAlignment::Center);
    titleText->setMargin({0.0f, 0.0f, 0.0f, 24.0f});
    hubWindow->addChild(titleText);

    projectListPanel = std::make_shared<Axiom::UIVerticalBox>();
    projectListPanel->setPadding({16.0f, 16.0f, 16.0f, 16.0f});
    projectListPanel->setMargin({0.0f, 0.0f, 0.0f, 16.0f});
    hubWindow->addChild(projectListPanel);

    auto recentLabel = std::make_shared<Axiom::UIText>("Recent Projects");
    recentLabel->setColor(uiRoot->getTheme()->textMutedColor);
    recentLabel->setMargin({0.0f, 0.0f, 0.0f, 12.0f});
    projectListPanel->addChild(recentLabel);

    for (const auto& project : recentProjects) {
        auto projectBtn = std::make_shared<Axiom::UIButton>(project.name);
        projectBtn->setMargin({0.0f, 0.0f, 0.0f, 4.0f});
        projectBtn->setOnClick([this, project]() { Axiom::AX_LOG_INFO("OPEN PROJECT"); });
        projectListPanel->addChild(projectBtn);
    }

    auto browseBtn = std::make_shared<Axiom::UIButton>("Browse files...");
    browseBtn->setPadding({12.0f, 8.0f, 12.0f, 8.0f});
    browseBtn->setNormalColor(uiRoot->getTheme()->accentColor);
    browseBtn->setOnClick([this]() {
        std::optional<std::filesystem::path> selectedFolder = Axiom::FileDialogs::openFolder("Select a Project Folder");
        if (selectedFolder.has_value()) {
            openProject(selectedFolder.value());
        }
    });
    hubWindow->addChild(browseBtn);
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
