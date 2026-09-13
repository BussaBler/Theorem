#include "EditorLayer.h"

#include "ECS/Components/TagComponent.h"
#include "Math/Color.h"
#include "Project.h"
#include "UI/InspectorUI.h"
#include "UI/UIPanel.h"

#include <cstddef>
#include <cstdint>
#include <memory>

EditorLayer::EditorLayer() : Axiom::Layer("EditorLayer") {
}

void EditorLayer::onAttach() {
    Axiom::AX_LOG_INFO("EditorLayer attached");
    Axiom::Locator::getWindow()->setSize(1280, 720);
    viewportSize = Axiom::Locator::getRenderer()->getCurrentRenderTargetSize();

    mainUiContext = {
        .renderer = Axiom::Locator::getUIRenderer(),
        .theme = &Axiom::UITheme::getDefault(),
        .layer = 0,
    };

    uiRoot = std::make_shared<Axiom::UICanvas>();

    auto rootBackGround = std::make_shared<Axiom::UIPanel>();
    rootBackGround->setId("RootBackground");
    rootBackGround->setBackgroundColor(mainUiContext.theme->windowBackgroundColor);
    uiRoot->addSlot(rootBackGround);

    auto mainLayout = std::make_shared<Axiom::UIHorizontalBox>();
    mainLayout->setId("MainHBox");
    rootBackGround->addSlot(mainLayout);

    leftPanel = std::make_shared<Axiom::UIPanel>();
    leftPanel->setId("LeftPanel");
    leftPanel->setBackgroundColor(mainUiContext.theme->panelBackgroundColor);
    mainLayout->addSlot(leftPanel)
        .setMargin(mainUiContext.theme->containerPadding)
        .setFixedSize({320.0f, -1.0f})
        .setHorizontalAlignment(Axiom::UIAlignment::Start);

    auto leftVBox = std::make_shared<Axiom::UIVerticalBox>();
    leftPanel->addSlot(leftVBox).setHorizontalAlignment(Axiom::UIAlignment::Fill).setVerticalAlignment(Axiom::UIAlignment::Fill);

    hierarchyPanel = std::make_shared<Axiom::UIVerticalBox>();
    hierarchyPanel->setId("HierarchyPanel");
    leftVBox->addSlot(hierarchyPanel).setMargin(mainUiContext.theme->itemMargin).setVerticalAlignment(Axiom::UIAlignment::Fill);

    profilerPanel = std::make_shared<Axiom::UIVerticalBox>();
    profilerPanel->setId("ProfilerPanel");
    leftVBox->addSlot(profilerPanel).setMargin(mainUiContext.theme->itemMargin).setVerticalAlignment(Axiom::UIAlignment::End);

    auto viewportPanel = std::make_shared<Axiom::UIPanel>();
    viewportPanel->setId("ViewportPanel");
    viewportPanel->setBackgroundColor(mainUiContext.theme->windowBackgroundColor);
    mainLayout->addSlot(viewportPanel)
        .setMargin({0.0f, 0.0f, 0.0f, 0.0f})
        .setHorizontalAlignment(Axiom::UIAlignment::Fill)
        .setVerticalAlignment(Axiom::UIAlignment::Fill);

    viewportImage = std::make_shared<Axiom::UIImage>();
    viewportImage->setId("Viewport");
    viewportPanel->addSlot(viewportImage)
        .setFixedSize({640.0f, 360.0f})
        .setHorizontalAlignment(Axiom::UIAlignment::Center)
        .setVerticalAlignment(Axiom::UIAlignment::Start);

    auto rightPanel = std::make_shared<Axiom::UIPanel>();
    rightPanel->setId("RightPanel");
    rightPanel->setBackgroundColor(mainUiContext.theme->panelBackgroundColor);
    mainLayout->addSlot(rightPanel)
        .setMargin(mainUiContext.theme->containerPadding)
        .setFixedSize({320.0f, -1.0f})
        .setHorizontalAlignment(Axiom::UIAlignment::End);

    inspectorPanel = std::make_shared<Axiom::UIVerticalBox>();
    inspectorPanel->setId("InspectorPanel");
    rightPanel->addSlot(inspectorPanel).setMargin(mainUiContext.theme->itemMargin);

    scene = std::make_shared<Axiom::Scene>();
    Axiom::SceneSerializer sceneSerializer(scene.get());
    ProjectConfig projectConfig = Project::getActive()->getConfig();

    if (Axiom::FileSystem::exists(projectConfig.startScene)) {
        if (!sceneSerializer.deserialize(projectConfig.startScene)) {
            Axiom::AX_LOG_ERROR("Failed to deserialize the project scene");
        }
    } else {
        // TODO: create a default scene
    }

    Axiom::UUID textureHandle = Axiom::AssetManager::importAsset("Redstone Block", "project://Assets/Textures/redstone_block.png", Axiom::AssetType::Texture);
    textureAsset = Axiom::AssetManager::getAsset<Axiom::TextureAsset>(textureHandle);

    Axiom::Texture::CreateInfo createInfo = {
        .width = 1920,
        .height = 1080,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = Axiom::Format::B8G8R8A8Unorm,
        .usage = Axiom::TextureUsage::ColorAttachment | Axiom::TextureUsage::Sampled,
        .aspect = Axiom::TextureAspect::Color,
        .initialState = Axiom::TextureState::Undefined,
        .memoryUsage = Axiom::MemoryUsage::GPUOnly,
    };
    Axiom::Texture::CreateInfo depthCreateInfo = {
        .width = 1920,
        .height = 1080,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = Axiom::Format::D32sFloat,
        .usage = Axiom::TextureUsage::DepthStencilAttachment,
        .aspect = Axiom::TextureAspect::Depth,
        .initialState = Axiom::TextureState::Undefined,
        .memoryUsage = Axiom::MemoryUsage::GPUOnly,
    };
    uint32_t frameCount = Axiom::Locator::getRenderer()->getFrameCount();

    sceneTextures.resize(frameCount);
    depthTextures.resize(frameCount);

    for (uint32_t i = 0; i < frameCount; ++i) {
        sceneTextures[i] = Axiom::Locator::getRenderer()->createTexture(createInfo);
        depthTextures[i] = Axiom::Locator::getRenderer()->createTexture(depthCreateInfo);
    }

    editorCamera = std::make_unique<EditorCamera>(Math::Vec3(0.0f, 5.0f, 10.0f), -25.0f);
    editorCamera->setPerspective(45.0f, static_cast<float>(1920) / static_cast<float>(1080), 0.1f, 3000.0f);

    refreshHierarchyPanel();
}

void EditorLayer::onDetach() {
    ProjectConfig projectConfig = Project::getActive()->getConfig();
    Axiom::SceneSerializer sceneSerializer(scene.get());
    sceneSerializer.serialize(projectConfig.startScene);
    Axiom::AssetManager::serializeManifest("project://ProjectManifest.json", "project://");
    std::string projectFileName = "project://" + projectConfig.name + ".theorem";
    Project::saveActive(projectFileName);

    Axiom::AX_LOG_INFO("EditorLayer detached");
}

void EditorLayer::onUpdate() {
    editorCamera->onUpdate(0.125f);
    scene->onUpdate(0.125f);
    float winWidth = Axiom::Locator::getWindow()->getWidth();
    float winHeight = Axiom::Locator::getWindow()->getHeight();
    if (shouldRefreshHierarchy) {
        refreshHierarchyPanel();
        shouldRefreshHierarchy = false;
    }
    if (shouldRefreshInspector) {
        refreshInspectorPanel();
        shouldRefreshInspector = false;
    }
    refreshProfilerPanel();
    uiRoot->updateLayout(mainUiContext, Math::Vec2(0, 0), Math::Vec2(winWidth, winHeight));
}

void EditorLayer::onUIRender() {
    mainUiContext.renderer->pushScissorRect({{0, 0}, {Axiom::Locator::getWindow()->getWidth(), Axiom::Locator::getWindow()->getHeight()}}, mainUiContext.layer);
    uiRoot->onRender(mainUiContext, Math::Rect({0, 0}, {Axiom::Locator::getWindow()->getWidth(), Axiom::Locator::getWindow()->getHeight()}));
    mainUiContext.renderer->popScissorRect(mainUiContext.layer);
}

void EditorLayer::onEvent(Axiom::Event& event) {
    Axiom::EventDispatcher dispatcher(event);

    dispatcher.dispatch<Axiom::MouseButtonPressedEvent>([this](const Axiom::MouseButtonPressedEvent& e) {
        if (e.getMouseButton() == Axiom::KeyCode::RightButton) {
            auto bounds = leftPanel->getArrangedPosition();
            auto size = leftPanel->getArrangedSize();
            float mouseX = e.getMouseX();
            float mouseY = e.getMouseY();

            if (mouseX >= bounds.x() && mouseX <= bounds.x() + size.x() && mouseY >= bounds.y() && mouseY <= bounds.y() + size.y()) {
                spawnHierarchyContextMenu(Math::Vec2(mouseX, mouseY));
                return true;
            }
        }
        return false;
    });

    if (!event.isHandled()) {
        event.handled = uiRoot->onEvent(event);
    }
}

void EditorLayer::onRender(Axiom::RenderGraph& renderGraph) {
    uint32_t currentFrameIndex = Axiom::Locator::getRenderer()->getCurrentFrameIndex();
    std::shared_ptr<Axiom::Texture> renderTarget = sceneTextures[currentFrameIndex];
    std::shared_ptr<Axiom::Texture> depthTexture = depthTextures[currentFrameIndex];

    Axiom::RenderContext renderContext = {.targetScene = scene.get(),
                                          .viewMatrix = editorCamera->getView(),
                                          .projectionMatrix = editorCamera->getProjection(),
                                          .cameraPosition = editorCamera->getPosition(),
                                          .renderTarget = renderTarget.get(),
                                          .depthTarget = depthTexture.get(),
                                          .shouldDrawSkybox = true,
                                          .shouldDrawGizmos = false,
                                          .shouldDrawWorldGrid = true};

    if (selectedEntity && selectedEntity.hasComponent<Axiom::TransformComponent>()) {
        renderContext.shouldDrawGizmos = true;
        renderContext.gizmosPosition = selectedEntity.getComponent<Axiom::TransformComponent>().position;
    }

    Axiom::Locator::getRenderer()->getFRP()->render(renderGraph, renderContext);

    viewportImage->setTexture(renderTarget);
}

void EditorLayer::refreshHierarchyPanel() {
    hierarchyPanel->clearSlots();
    hierarchyButtons.clear();

    auto headerRow = std::make_shared<Axiom::UIHorizontalBox>();

    auto headerText = std::make_shared<Axiom::UIText>("Hierarchy");
    headerRow->addSlot(headerText).setHorizontalAlignment(Axiom::UIAlignment::Fill);
    hierarchyPanel->addSlot(headerRow).setVerticalAlignment(Axiom::UIAlignment::Start);

    Axiom::View entityView = scene->view<Axiom::TagComponent>();
    for (uint32_t entityId : entityView) {
        Axiom::Entity entity = scene->getEntity(entityId);
        auto& tagComponent = entity.getComponent<Axiom::TagComponent>();

        auto row = std::make_shared<Axiom::UIHorizontalBox>();

        auto entityButton = std::make_shared<Axiom::UIButton>();
        entityButton->setText(tagComponent.tag);
        entityButton->setId("Entity_" + std::to_string(entityId));

        if (selectedEntity == entity) {
            entityButton->setNormalColor(mainUiContext.theme->accentColor);
        }

        hierarchyButtons[entityId] = entityButton;

        entityButton->setOnClick([this, entity, entityId]() {
            if (selectedEntity && hierarchyButtons.count(selectedEntity.getId())) {
                hierarchyButtons[selectedEntity.getId()]->setNormalColor(mainUiContext.theme->controlNormalColor);
            }

            selectedEntity = entity;
            hierarchyButtons[entityId]->setNormalColor(mainUiContext.theme->accentColor);
            shouldRefreshInspector = true;
        });

        row->addSlot(entityButton)
            .setMargin(mainUiContext.theme->itemMargin)
            .setFixedSize({-1.0f, mainUiContext.theme->defaultRowHeight})
            .setVerticalAlignment(Axiom::UIAlignment::Start);

        auto deleteBtn = std::make_shared<Axiom::UIButton>();
        deleteBtn->setText("X");
        deleteBtn->setNormalColor(Axiom::Color::red());
        deleteBtn->setOnClick([this, entity]() {
            if (selectedEntity == entity) {
                selectedEntity = {};
                inspectorPanel->clearSlots();
            }
            scene->deleteEntity(entity);
            shouldRefreshHierarchy = true;
        });

        row->addSlot(deleteBtn)
            .setFixedSize({mainUiContext.theme->defaultRowHeight, mainUiContext.theme->defaultRowHeight})
            .setVerticalAlignment(Axiom::UIAlignment::Start);

        hierarchyPanel->addSlot(row).setVerticalAlignment(Axiom::UIAlignment::Start);
    }
}

void EditorLayer::refreshInspectorPanel() {
    inspectorPanel->clearSlots();
    if (!selectedEntity) {
        return;
    }

    if (selectedEntity.hasComponent<Axiom::TagComponent>()) {
        auto tagRow = std::make_shared<Axiom::UIHorizontalBox>();

        auto label = std::make_shared<Axiom::UIText>("Name:");
        tagRow->addSlot(label).setFixedSize({120.0f, -1.0f}).setHorizontalAlignment(Axiom::UIAlignment::Start).setVerticalAlignment(Axiom::UIAlignment::Start);

        auto nameInput = std::make_shared<Axiom::UITextInput>();

        Axiom::Entity capturedEntity = selectedEntity;
        nameInput->setValueGetter([capturedEntity]() { return capturedEntity.getComponent<Axiom::TagComponent>().tag; });
        nameInput->setValueSetter([this, capturedEntity](const std::string& v) mutable {
            capturedEntity.getComponent<Axiom::TagComponent>().tag = v;
            uint32_t id = capturedEntity.getId();
            if (hierarchyButtons.find(id) != hierarchyButtons.end()) {
                hierarchyButtons[id]->setText(v);
            }
        });
        tagRow->addSlot(nameInput).setHorizontalAlignment(Axiom::UIAlignment::Fill).setVerticalAlignment(Axiom::UIAlignment::Start);
        inspectorPanel->addSlot(tagRow).setVerticalAlignment(Axiom::UIAlignment::Start);
    }

    for (const auto& [componentId, dataPtr] : selectedEntity.getComponents()) {
        const Axiom::ComponentInfo* componentInfo = Axiom::ComponentReflection::getComponentInfo(componentId);

        if (!componentInfo || componentInfo->name == "TagComponent" || componentInfo->name == "Tag") {
            continue;
        }

        auto componentGroup = std::make_shared<Axiom::UICollapsableGroup>(componentInfo->name.substr(0, componentInfo->name.find("Component")));

        for (const auto& field : componentInfo->fields) {
            auto fieldUI = InspectorUI::createFieldUI(selectedEntity, componentId, field, mainUiContext.theme);

            if (fieldUI) {
                auto row = std::make_shared<Axiom::UIHorizontalBox>();
                row->addSlot(fieldUI).setHorizontalAlignment(Axiom::UIAlignment::Fill);
                componentGroup->addSlot(row);
            }
        }
        inspectorPanel->addSlot(componentGroup).setVerticalAlignment(Axiom::UIAlignment::Start);
    }

    auto addComponentGroup = std::make_shared<Axiom::UICollapsableGroup>("+ Add Component");

    auto createAddButton = [this](const std::string& name, auto checkHas, auto addComp) {
        auto btn = std::make_shared<Axiom::UIButton>();
        btn->setText(name);
        btn->setOnClick([this, checkHas, addComp]() {
            if (!checkHas()) {
                addComp();
                shouldRefreshInspector = true;
            }
        });
        return btn;
    };

    const auto& allComponents = Axiom::ComponentReflection::getRegistry();
    for (const auto& [componentId, componentInfo] : allComponents) {
        if (componentInfo.name == "TagComponent" || componentInfo.name == "Tag") {
            continue;
        }

        if (selectedEntity.hasComponent(componentId)) {
            continue;
        }

        addComponentGroup
            ->addSlot(createAddButton(
                componentInfo.name.substr(0, componentInfo.name.find("Component")), [this, componentId]() { return selectedEntity.hasComponent(componentId); },
                [this, componentId, componentInfo]() { Axiom::ComponentReflection::addComponent(selectedEntity, componentInfo.name, nullptr); }))
            .setHorizontalAlignment(Axiom::UIAlignment::Fill);
    }

    inspectorPanel->addSlot(addComponentGroup).setVerticalAlignment(Axiom::UIAlignment::Start);
}

void EditorLayer::spawnHierarchyContextMenu(Math::Vec2 spawnPos) {
    contextMenu = std::make_shared<Axiom::UIPanel>();
    contextMenu->setBackgroundColor(mainUiContext.theme->panelBackgroundColor);

    auto createBtn = std::make_shared<Axiom::UIButton>();
    createBtn->setText("Create Empty Entity");
    createBtn->setOnClick([this]() {
        Axiom::Entity newEntity = scene->newEntity();
        newEntity.addComponent<Axiom::TagComponent>({"New Entity"});
        selectedEntity = newEntity;

        shouldRefreshHierarchy = true;
        shouldRefreshInspector = true;
        uiRoot->closePopup();
    });

    contextMenu->addSlot(createBtn).setMargin(mainUiContext.theme->containerPadding);
    uiRoot->openPopup(contextMenu, spawnPos);
}

void EditorLayer::refreshProfilerPanel() {
    const auto& profiles = Axiom::Profiler::getProfiles();

    if (profilerPanel->getSlots().size() != profiles.size() + 1) {
        profilerPanel->clearSlots();

        auto headerRow = std::make_shared<Axiom::UIHorizontalBox>();

        auto headerText = std::make_shared<Axiom::UIText>("Profiler");
        headerRow->addSlot(headerText).setHorizontalAlignment(Axiom::UIAlignment::Fill);
        profilerPanel->addSlot(headerRow).setVerticalAlignment(Axiom::UIAlignment::Start);

        for (size_t i = 0; i < profiles.size(); i++) {
            auto label = std::make_shared<Axiom::UIText>("");
            profilerPanel->addSlot(label).setVerticalAlignment(Axiom::UIAlignment::Start);
        }
    }

    auto slots = profilerPanel->getSlots();
    for (size_t i = 0; i < profiles.size(); ++i) {
        auto ms = std::chrono::duration_cast<std::chrono::duration<double>>(profiles[i].duration);
        auto labelText = std::format("{}: {:.4f} ms", profiles[i].name, ms.count());
        auto textWidget = std::static_pointer_cast<Axiom::UIText>(slots[i + 1].content);
        textWidget->setText(labelText);
    }
}
