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
    void openProject(const std::filesystem::path& projectPath);

  private:
    Axiom::UIContext hubUiContext;
    std::shared_ptr<Axiom::UIContainer> uiRoot;
    std::shared_ptr<Axiom::UIVerticalBox> projectListPanel;

    std::vector<ProjectData> recentProjects;
};
