#include "ProjectHubLayer.h"

#include "Core/Locator.h"
#include "Core/Log.h"
#include "EditorLayer.h"
#include "Project.h"
#include "UI/UIElement.h"
#include "UI/UISlot.h"
#include "Utils/FileSystem.h"
#include "Utils/JSONSerializer.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

ProjectHubLayer::ProjectHubLayer() : Axiom::Layer("ProjectHubLayer") {
}

void ProjectHubLayer::onAttach() {
    Axiom::AX_LOG_INFO("Project Hub Layer Attached");
    hubUiContext = {.renderer = Axiom::Locator::getUIRenderer(), .theme = &Axiom::UITheme::getDefault(), .layer = 0};

    loadRecentProjects();
    buildUI();
}

void ProjectHubLayer::onDetach() {
    Axiom::AX_LOG_INFO("Project Hub Layer Detached");
}

void ProjectHubLayer::onUpdate() {
    float winWidth = Axiom::Locator::getWindow()->getWidth();
    float winHeight = Axiom::Locator::getWindow()->getHeight();
    uiRoot->updateLayout(hubUiContext, Math::Vec2::zero(), Math::Vec2(winWidth, winHeight));
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
    background->setBackgroundColor(hubUiContext.theme->windowBackgroundColor);
    uiRoot->addSlot(background);

    auto hubWindow = std::make_shared<Axiom::UIVerticalBox>();
    hubWindow->setPadding(hubUiContext.theme->containerPadding);
    background->addSlot(hubWindow)
        .setFixedSize({360.0f, -1.0f})
        .setHorizontalAlignment(Axiom::UIAlignment::Center)
        .setVerticalAlignment(Axiom::UIAlignment::Center);

    auto titleText = std::make_shared<Axiom::UIText>("Theorem Engine");
    hubWindow->addSlot(titleText).setFixedSize({-1.0f, 30.0f}).setHorizontalAlignment(Axiom::UIAlignment::Center).setMargin({0.0f, 0.0f, 0.0f, 24.0f});

    projectListPanel = std::make_shared<Axiom::UIVerticalBox>();
    hubWindow->addSlot(projectListPanel).setVerticalAlignment(Axiom::UIAlignment::Start);

    auto recentLabel = std::make_shared<Axiom::UIText>("Recent Projects");
    recentLabel->setTextColor(hubUiContext.theme->textMuted);
    projectListPanel->addSlot(recentLabel)
        .setMargin(hubUiContext.theme->itemMargin)
        .setFixedSize({-1.0f, 20.0f})
        .setVerticalAlignment(Axiom::UIAlignment::Start);

    for (const auto& project : recentProjects) {
        auto projectBtn = std::make_shared<Axiom::UIButton>();
        projectBtn->setText(project.name);
        projectBtn->setNormalColor(hubUiContext.theme->accentColor);
        projectBtn->setOnClick([this, project]() { openProject(project.path); });
        projectListPanel->addSlot(projectBtn).setFixedSize({-1.0f, 32.0f}).setVerticalAlignment(Axiom::UIAlignment::Start);
    }

    auto browseBtn = std::make_shared<Axiom::UIButton>();
    browseBtn->setText("Browse files...");
    browseBtn->setNormalColor(hubUiContext.theme->accentColor);
    browseBtn->setOnClick([this]() {
        std::optional<std::filesystem::path> selectedFolder = Axiom::FileDialogs::openFolder("Select a Project Folder");
        if (selectedFolder.has_value()) {
            openProject(selectedFolder.value());
        }
    });
    hubWindow->addSlot(browseBtn).setMargin({0.0f, 40.0f, 0.0f, 0.0f}).setFixedSize({-1.0f, 48.0f}).setVerticalAlignment(Axiom::UIAlignment::Start);
}

void ProjectHubLayer::loadRecentProjects() {
    recentProjects.clear();
    std::filesystem::path recentProjectsPath = "app://RecentProjects.json";

    if (Axiom::FileSystem::exists(recentProjectsPath)) {
        std::string jsonStr = Axiom::FileSystem::readFileStr(recentProjectsPath);
        Axiom::JSONValue root = Axiom::JSONSerializer::deserialize(jsonStr);

        std::vector<Axiom::JSONValue> list = root.getChild("Projects").getArrayElements();
        for (const auto& projNode : list) {
            std::string name = projNode.getChild("Name").getString();
            std::filesystem::path path = projNode.getChild("Path").getString();

            if (!path.empty()) {
                recentProjects.push_back({name, path});
            }
        }
    }
}

void ProjectHubLayer::addToRecentProjects(const std::string& name, const std::filesystem::path& filePath) {
    recentProjects.erase(std::remove_if(recentProjects.begin(), recentProjects.end(), [&](const RecentProject& data) { return data.path == filePath; }),
                         recentProjects.end());

    recentProjects.insert(recentProjects.begin(), {name, filePath});

    if (recentProjects.size() > 5) {
        recentProjects.pop_back();
    }

    Axiom::JSONValue root;
    Axiom::JSONValue projectsArray;

    for (const auto& project : recentProjects) {
        Axiom::JSONValue projectNode;
        Axiom::JSONValue nameNode;
        nameNode.setString(project.name);
        Axiom::JSONValue pathNode;
        pathNode.setString(project.path.string());
        projectNode.setChild("Name", nameNode);
        projectNode.setChild("Path", pathNode);
        projectsArray.addArrayElement(projectNode);
    }

    root.setChild("Projects", projectsArray);
    Axiom::FileSystem::writeFile("app://RecentProjects.json", Axiom::JSONSerializer::serialize(root));
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

    addToRecentProjects(Project::getActive()->getConfig().name, projectPath);

    transitionTo<EditorLayer>();
}
