#pragma once
#include "EditorCamera.h"
#include "UI/Elements/UIButton.h"
#include "UI/Elements/UICanvas.h"
#include <Axiom.h>
#include <cstdint>
#include <memory>
#include <unordered_map>

class EditorLayer : public Axiom::Layer {
  public:
    EditorLayer();
    ~EditorLayer() = default;

    void onAttach() override;
    void onDetach() override;
    void onUpdate() override;
    void onUIRender() override;
    void onEvent(Axiom::Event& event) override;
    void onRender(Axiom::RenderGraph& commandBuffer) override;

  private:
    void refreshHierarchyPanel();
    void refreshInspectorPanel();
    void spawnHierarchyContextMenu(Math::Vec2 spawnPos);
    void refreshProfilerPanel();

    void buildAssetPickerPopUp(Axiom::Entity entity, std::type_index compTypeIndex, std::shared_ptr<Axiom::UIButton> assetButton, const Axiom::FieldInfo& field);

  private:
    Math::uVec2 viewportSize{0, 0};
    std::shared_ptr<Axiom::TextureAsset> textureAsset;
    std::shared_ptr<Axiom::Scene> scene;
    std::vector<std::shared_ptr<Axiom::Texture>> sceneTextures;
    std::vector<std::shared_ptr<Axiom::Texture>> depthTextures;
    std::unique_ptr<EditorCamera> editorCamera;

    Axiom::UIContext mainUiContext;
    std::shared_ptr<Axiom::UICanvas> uiRoot;
    std::shared_ptr<Axiom::UIImage> viewportImage;
    std::shared_ptr<Axiom::UIVerticalBox> hierarchyPanel;
    std::unordered_map<uint32_t, std::shared_ptr<Axiom::UIButton>> hierarchyButtons;
    std::shared_ptr<Axiom::UIVerticalBox> inspectorPanel;
    std::shared_ptr<Axiom::UIVerticalBox> profilerPanel;
    std::shared_ptr<Axiom::UIPanel> contextMenu = nullptr;
    std::shared_ptr<Axiom::UIPanel> assetPickerMenu = nullptr;
    bool shouldRefreshHierarchy = false;
    bool shouldRefreshInspector = false;

    Axiom::Entity selectedEntity = Axiom::Entity();
};
