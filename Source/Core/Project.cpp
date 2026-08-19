#include "Project.h"

#include "Utils/FileSystem.h"
#include "Utils/JSONSerializer.h"

#include <filesystem>
#include <memory>
#include <string>

std::shared_ptr<Project> Project::activeProject = nullptr;

std::shared_ptr<Project> Project::load(const std::filesystem::path& projectPath) {
    if (!Axiom::FileSystem::exists(projectPath)) {
        Axiom::AX_LOG_ERROR("Project file not found: {}", projectPath.generic_string());
        return nullptr;
    }

    std::string projectJsonStr = Axiom::FileSystem::readFileStr(projectPath);
    Axiom::JSONValue projectRoot = Axiom::JSONSerializer::deserialize(projectJsonStr);

    std::shared_ptr<Project> project = std::make_shared<Project>();

    Axiom::JSONValue configNode = projectRoot.getChild("Project");
    project->config.name = configNode.getChild("Name").getString();
    project->config.startScene = configNode.getChild("StartScene").getString();
    project->config.assetDir = configNode.getChild("AssetDirectory").getString();

    activeProject = project;
    return activeProject;
}

std::shared_ptr<Project> Project::createNew(const std::string& name, const std::filesystem::path& projectPath) {
    std::shared_ptr<Project> project = std::make_shared<Project>();
    project->config.name = name;

    activeProject = project;

    std::filesystem::path projectFilePath = projectPath / (name + ".theorem");
    saveActive(projectFilePath);

    return activeProject;
}

void Project::saveActive(const std::filesystem::path& filePath) {
    if (!activeProject) {
        Axiom::AX_LOG_WARN("No active project to save");
        return;
    }

    Axiom::JSONValue projectRoot;
    Axiom::JSONValue configNode;
    Axiom::JSONValue nameNode;
    nameNode.setString(activeProject->config.name);
    Axiom::JSONValue startSceneNode;
    startSceneNode.setString(activeProject->config.startScene);
    Axiom::JSONValue assetDirNode;
    assetDirNode.setString(activeProject->config.assetDir);
    configNode.setChild("Name", nameNode);
    configNode.setChild("StartScene", startSceneNode);
    configNode.setChild("AssetDirectory", assetDirNode);

    projectRoot.setChild("Project", configNode);

    Axiom::FileSystem::writeFile(filePath, Axiom::JSONSerializer::serialize(projectRoot));
}
