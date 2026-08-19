#pragma once

#include <Axiom.h>
#include <filesystem>
#include <memory>
#include <string>

struct ProjectConfig {
    std::string name = "untitled";
    std::string startScene = "project://Scenes/main.scene";
    std::string assetDir = "project://Assets";
};

class Project {
  public:
    static inline std::shared_ptr<Project> getActive() { return activeProject; }

    static std::shared_ptr<Project> load(const std::filesystem::path& projectPath);
    static std::shared_ptr<Project> createNew(const std::string& name, const std::filesystem::path& projectPath);
    static void saveActive(const std::filesystem::path& filePath);

    inline ProjectConfig& getConfig() { return config; }

  private:
    ProjectConfig config;
    static std::shared_ptr<Project> activeProject;
};
