#include "EditorLayer.h"

#include "Project.h"
#include "UI/InspectorUI.h"

#include <cstddef>
#include <cstdint>
#include <memory>

EditorLayer::EditorLayer() : Axiom::Layer("EditorLayer") {
}

void EditorLayer::onAttach() {
    Axiom::AX_LOG_INFO("EditorLayer attached");
    viewportSize = Axiom::Locator::getRenderer()->getCurrentRenderTargetSize();

    mainUiContext = {
        .renderer = Axiom::Locator::getUIRenderer(),
        .dpiScale = Axiom::Locator::getWindow()->getWindowDPI() / 96.0f,
        .layer = 0,
    };

    uiRoot = std::make_shared<Axiom::UICanvas>();

    auto mainLayout = std::make_shared<Axiom::UIHorizontalBox>();
    mainLayout->setID("MainHBox");
    uiRoot->addChild(mainLayout);

    auto leftPanel = std::make_shared<Axiom::UIPanel>();
    leftPanel->setID("LeftPanel");
    leftPanel->setFixedSize({320.0f, -1.0f});
    leftPanel->setHorizontalAlignment(Axiom::UIAlignment::Start);
    leftPanel->setPadding({8.0f, 8.0f, 8.0f, 8.0f});
    mainLayout->addChild(leftPanel);

    auto leftVBox = std::make_shared<Axiom::UIVerticalBox>();
    leftVBox->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    leftVBox->setVerticalAlignment(Axiom::UIAlignment::Fill);
    leftPanel->addChild(leftVBox);

    hierarchyPanel = std::make_shared<Axiom::UIVerticalBox>();
    hierarchyPanel->setID("HierarchyPanel");
    hierarchyPanel->setPadding({4.0f, 4.0f, 4.0f, 4.0f});
    hierarchyPanel->setVerticalAlignment(Axiom::UIAlignment::Fill);
    leftVBox->addChild(hierarchyPanel);

    profilerPanel = std::make_shared<Axiom::UIVerticalBox>();
    profilerPanel->setID("ProfilerPanel");
    profilerPanel->setPadding({4.0f, 4.0f, 4.0f, 4.0f});
    profilerPanel->setVerticalAlignment(Axiom::UIAlignment::End);
    leftVBox->addChild(profilerPanel);

    auto viewportPanel = std::make_shared<Axiom::UIPanel>();
    viewportPanel->setID("ViewportPanel");
    viewportPanel->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    viewportPanel->setVerticalAlignment(Axiom::UIAlignment::Fill);
    viewportPanel->setPadding({0.0f, 0.0f, 0.0f, 0.0f});
    viewportPanel->setBackgroundColor(uiRoot->getTheme()->windowBackgroundColor);
    mainLayout->addChild(viewportPanel);

    viewportImage = std::make_shared<Axiom::UIImage>();
    viewportImage->setID("Viewport");
    viewportImage->setFixedSize({640.0f, 360.0f});
    viewportImage->setVerticalAlignment(Axiom::UIAlignment::Start);
    viewportImage->setHorizontalAlignment(Axiom::UIAlignment::Center);
    viewportPanel->addChild(viewportImage);

    auto rightPanel = std::make_shared<Axiom::UIPanel>();
    rightPanel->setID("RightPanel");
    rightPanel->setFixedSize({320.0f, -1.0f});
    rightPanel->setHorizontalAlignment(Axiom::UIAlignment::End);
    rightPanel->setPadding({8.0f, 8.0f, 8.0f, 8.0f});
    mainLayout->addChild(rightPanel);

    inspectorPanel = std::make_shared<Axiom::UIVerticalBox>();
    inspectorPanel->setID("InspectorPanel");
    inspectorPanel->setPadding({4.0f, 4.0f, 4.0f, 4.0f});
    rightPanel->addChild(inspectorPanel);

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
    editorCamera->setPerspective(45.0f, static_cast<float>(viewportSize.x()) / static_cast<float>(viewportSize.y()), 0.1f, 3000.0f);

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
    uiRoot->arrange(mainUiContext, Math::Vec2(0, 0), Math::Vec2(winWidth, winHeight));
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
            auto bounds = hierarchyPanel->getArrangedPosition();
            auto size = hierarchyPanel->getArrangedSize();
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
    if (selectedEntity) {
        renderContext.shouldDrawGizmos = true;
        renderContext.gizmosPosition = selectedEntity.getComponent<Axiom::TransformComponent>().position;
    }

    Axiom::Locator::getRenderer()->getFRP()->render(renderGraph, renderContext);

    viewportImage->setTexture(renderTarget);
}

void EditorLayer::refreshHierarchyPanel() {
    hierarchyPanel->clearChildren();
    hierarchyButtons.clear();

    auto headerRow = std::make_shared<Axiom::UIHorizontalBox>();
    headerRow->setVerticalAlignment(Axiom::UIAlignment::Start);
    headerRow->setMargin({0.0f, 0.0f, 0.0f, 8.0f});

    auto headerText = std::make_shared<Axiom::UIText>("Hierarchy");
    headerText->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    headerRow->addChild(headerText);
    hierarchyPanel->addChild(headerRow);

    Axiom::View entityView = scene->view();
    for (uint32_t entityId : entityView) {
        Axiom::Entity entity = scene->getEntity(entityId);
        auto& tag = entity.getComponent<Axiom::TagComponent>();

        auto row = std::make_shared<Axiom::UIHorizontalBox>();
        row->setVerticalAlignment(Axiom::UIAlignment::Start);
        row->setMargin({0.0f, 0.0f, 0.0f, 4.0f});

        auto entityButton = std::make_shared<Axiom::UIButton>(tag.tag);
        entityButton->setID("Entity_" + std::to_string(entityId));
        entityButton->setVerticalAlignment(Axiom::UIAlignment::Start);
        entityButton->setPadding({4.0f, 4.0f, 4.0f, 4.0f});
        entityButton->setMargin({0.0f, 0.0f, 4.0f, 0.0f});
        entityButton->setFixedSize({-1.0f, 24.0f});

        if (selectedEntity == entity) {
            entityButton->setNormalColor(uiRoot->getTheme()->accentColor);
        }

        hierarchyButtons[entityId] = entityButton;

        entityButton->setOnClick([this, entity, entityId]() {
            if (selectedEntity && hierarchyButtons.count(selectedEntity.getId())) {
                hierarchyButtons[selectedEntity.getId()]->setNormalColor(uiRoot->getTheme()->controlNormalColor);
            }

            selectedEntity = entity;
            hierarchyButtons[entityId]->setNormalColor(uiRoot->getTheme()->accentColor);
            shouldRefreshInspector = true;
        });
        row->addChild(entityButton);

        auto deleteBtn = std::make_shared<Axiom::UIButton>("X");
        deleteBtn->setFixedSize({24.0f, 24.0f});
        deleteBtn->setVerticalAlignment(Axiom::UIAlignment::Start);
        deleteBtn->setNormalColor(uiRoot->getTheme()->errorColor);
        deleteBtn->setOnClick([this, entity]() {
            if (selectedEntity == entity) {
                selectedEntity = {};
                inspectorPanel->clearChildren();
            }
            scene->destroyEntity(entity);
            shouldRefreshHierarchy = true;
        });
        row->addChild(deleteBtn);

        hierarchyPanel->addChild(row);
    }
}

void EditorLayer::refreshInspectorPanel() {
    inspectorPanel->clearChildren();
    if (!selectedEntity) {
        return;
    }

    if (selectedEntity.hasComponent<Axiom::TagComponent>()) {
        auto tagRow = std::make_shared<Axiom::UIHorizontalBox>();
        tagRow->setMargin({0.0f, 0.0f, 0.0f, 16.0f});
        tagRow->setVerticalAlignment(Axiom::UIAlignment::Start);

        auto label = std::make_shared<Axiom::UIText>("Name:");
        label->setVerticalAlignment(Axiom::UIAlignment::Start);
        label->setHorizontalAlignment(Axiom::UIAlignment::Start);
        label->setMargin({0.0f, 0.0f, 8.0f, 0.0f});
        label->setFixedSize({120.0f, -1.0f});
        tagRow->addChild(label);

        auto nameInput = std::make_shared<Axiom::UITextInput>();
        nameInput->setHorizontalAlignment(Axiom::UIAlignment::Fill);
        nameInput->setVerticalAlignment(Axiom::UIAlignment::Start);

        Axiom::Entity capturedEntity = selectedEntity;
        nameInput->setValueGetter([capturedEntity]() { return capturedEntity.getComponent<Axiom::TagComponent>().tag; });
        nameInput->setValueSetter([this, capturedEntity](const std::string& v) mutable {
            capturedEntity.getComponent<Axiom::TagComponent>().tag = v;
            uint32_t id = capturedEntity.getId();
            if (hierarchyButtons.find(id) != hierarchyButtons.end()) {
                hierarchyButtons[id]->setText(v);
            }
        });
        tagRow->addChild(nameInput);
        inspectorPanel->addChild(tagRow);
    }

    for (const auto& [typeIndex, dataPtr] : selectedEntity.getComponents()) {
        const Axiom::ComponentInfo* componentInfo = Axiom::ComponentReflection::getComponentInfo(typeIndex);

        if (!componentInfo || componentInfo->name == "TagComponent" || componentInfo->name == "Tag") {
            continue;
        }

        auto componentGroup = std::make_shared<Axiom::UICollapsableGroup>(componentInfo->name.substr(0, componentInfo->name.find("Component")));
        componentGroup->setMargin({0.0f, 0.0f, 0.0f, 8.0f});
        componentGroup->setVerticalAlignment(Axiom::UIAlignment::Start);

        for (const auto& field : componentInfo->fields) {
            auto fieldUI = InspectorUI::createFieldUI(selectedEntity, typeIndex, field, uiRoot->getTheme());

            if (fieldUI) {
                auto row = std::make_shared<Axiom::UIHorizontalBox>();
                row->setMargin({4.0f, 4.0f, 4.0f, 4.0f});
                fieldUI->setHorizontalAlignment(Axiom::UIAlignment::Fill);
                row->addChild(fieldUI);
                componentGroup->addChild(row);
            }
        }
        inspectorPanel->addChild(componentGroup);
    }

    auto addComponentGroup = std::make_shared<Axiom::UICollapsableGroup>("+ Add Component");
    addComponentGroup->setMargin({0.0f, 10.0f, 0.0f, 0.0f});
    addComponentGroup->setVerticalAlignment(Axiom::UIAlignment::Start);

    auto createAddButton = [this](const std::string& name, auto checkHas, auto addComp) {
        auto btn = std::make_shared<Axiom::UIButton>(name);
        btn->setHorizontalAlignment(Axiom::UIAlignment::Fill);
        btn->setMargin({4.0f, 4.0f, 4.0f, 4.0f});
        btn->setOnClick([this, checkHas, addComp]() {
            if (!checkHas()) {
                addComp();
                shouldRefreshInspector = true;
            }
        });
        return btn;
    };

    const auto& allComponents = Axiom::ComponentReflection::getRegistry();
    for (const auto& [typeIndex, componentInfo] : allComponents) {
        if (componentInfo.name == "TagComponent" || componentInfo.name == "Tag") {
            continue;
        }

        if (selectedEntity.hasComponent(typeIndex)) {
            continue;
        }

        addComponentGroup->addChild(createAddButton(
            componentInfo.name.substr(0, componentInfo.name.find("Component")), [this, typeIndex]() { return selectedEntity.hasComponent(typeIndex); },
            [this, typeIndex, componentInfo]() { Axiom::ComponentReflection::addComponent(selectedEntity, componentInfo.name, nullptr); }));
    }

    inspectorPanel->addChild(addComponentGroup);
}

void EditorLayer::spawnHierarchyContextMenu(Math::Vec2 spawnPos) {
    contextMenu = std::make_shared<Axiom::UIPanel>();
    contextMenu->setBackgroundColor(Axiom::Color::transparent());

    auto createBtn = std::make_shared<Axiom::UIButton>("Create Empty Entity");
    createBtn->setPadding({10.0f, 5.0f, 10.0f, 5.0f});
    createBtn->setOnClick([this]() {
        Axiom::Entity newEntity = scene->createEntity();
        newEntity.addComponent<Axiom::TagComponent>({"New Entity"});
        selectedEntity = newEntity;

        shouldRefreshHierarchy = true;
        shouldRefreshInspector = true;
        uiRoot->closePopup();
    });

    contextMenu->addChild(createBtn);
    uiRoot->openPopup(contextMenu, spawnPos);
}

void EditorLayer::refreshProfilerPanel() {
    const auto& profiles = Axiom::Profiler::getProfiles();

    if (profilerPanel->getChildren().size() != profiles.size() + 1) {
        profilerPanel->clearChildren();

        auto headerRow = std::make_shared<Axiom::UIHorizontalBox>();
        headerRow->setVerticalAlignment(Axiom::UIAlignment::Start);
        headerRow->setMargin({0.0f, 0.0f, 0.0f, 10.0f});

        auto headerText = std::make_shared<Axiom::UIText>("Profiler");
        headerText->setHorizontalAlignment(Axiom::UIAlignment::Fill);
        headerRow->addChild(headerText);
        profilerPanel->addChild(headerRow);

        for (size_t i = 0; i < profiles.size(); i++) {
            auto label = std::make_shared<Axiom::UIText>("");
            label->setVerticalAlignment(Axiom::UIAlignment::Start);
            profilerPanel->addChild(label);
        }
    }

    auto children = profilerPanel->getChildren();
    for (size_t i = 0; i < profiles.size(); ++i) {
        auto ms = std::chrono::duration_cast<std::chrono::duration<double>>(profiles[i].duration);
        auto labelText = std::format("{}: {:.4f} ms", profiles[i].name, ms.count());
        auto textWidget = std::static_pointer_cast<Axiom::UIText>(children[i + 1]);
        textWidget->setText(labelText);
    }
}

void EditorLayer::buildAssetPickerPopUp(Axiom::Entity entity, std::type_index compTypeIndex, std::shared_ptr<Axiom::UIButton> assetButton,
                                        const Axiom::FieldInfo& field) {
    assetPickerMenu = std::make_shared<Axiom::UIPanel>();
    assetPickerMenu->setBackgroundColor(uiRoot->getTheme()->panelBackgroundColor);
    assetPickerMenu->setPadding({4.0f, 4.0f, 4.0f, 4.0f});

    auto vBox = std::make_shared<Axiom::UIVerticalBox>();
    assetPickerMenu->addChild(vBox);

    std::vector<Axiom::UUID> availableAssets = Axiom::AssetManager::getAssetsByType(field.assetType);

    if (availableAssets.empty()) {
        auto emptyLabel = std::make_shared<Axiom::UIText>("No assets found.");
        emptyLabel->setColor(uiRoot->getTheme()->textMutedColor);
        vBox->addChild(emptyLabel);
    } else {
        for (Axiom::UUID assetID : availableAssets) {
            std::string assetName = Axiom::AssetManager::getMetadata(assetID).name;

            auto btn = std::make_shared<Axiom::UIButton>(assetName);
            btn->setHorizontalAlignment(Axiom::UIAlignment::Fill);
            btn->setMargin({0.0f, 0.0f, 0.0f, 2.0f});

            btn->setOnClick([this, entity, compTypeIndex, offset = field.offset, assetButton, assetID, assetName]() mutable {
                void* compData = entity.getComponentData(compTypeIndex);
                if (compData) {
                    *reinterpret_cast<Axiom::UUID*>(static_cast<char*>(compData) + offset) = assetID;
                    assetButton->setText(assetName);
                }
            });

            vBox->addChild(btn);
        }
    }

    Math::Vec2 spawnPos = assetButton->getArrangedPosition() + Math::Vec2(0.0f, assetButton->getArrangedSize().y() + 4.0f);
    assetPickerMenu->arrange(mainUiContext, spawnPos, assetPickerMenu->getDesiredSize(mainUiContext));
}
