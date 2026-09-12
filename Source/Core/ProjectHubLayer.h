#pragma once

#include <Axiom.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class ProjectHubLayer : public Axiom::Layer {
  private:
    struct ProjectData {
        std::string name;
        std::filesystem::path path;
        std::string lastModified;
    };

    struct RecentProject {
        std::string name;
        std::filesystem::path path;
    };

  public:
    ProjectHubLayer();
    ~ProjectHubLayer() = default;

    void onAttach() override;
    void onDetach() override;
    void onUpdate() override;
    void onUIRender() override;
    void onEvent(Axiom::Event& event) override;
    void onRender(Axiom::RenderGraph& renderGraph) override;

  private:
    void buildUI();
    void loadRecentProjects();
    void addToRecentProjects(const std::string& name, const std::filesystem::path& filePath);
    void openProject(const std::filesystem::path& projectPath);

  private:
    Axiom::UIContext hubUiContext;
    std::shared_ptr<Axiom::UICanvas> uiRoot;
    std::shared_ptr<Axiom::UIVerticalBox> projectListPanel;

    std::vector<RecentProject> recentProjects;
};
