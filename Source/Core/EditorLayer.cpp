#include "EditorLayer.h"

#include "Asset/Asset.h"
#include "Asset/AssetManager.h"
#include "Core/Locator.h"
#include "Renderer/ForwardRenderPipeline.h"
#include "Renderer/RenderGraph.h"
#include "Renderer/RenderPipeline.h"
#include "Scene/Components/TransformComponent.h"

#include <memory>

EditorLayer::EditorLayer() : Layer("EditorLayer") {
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
    Axiom::SceneSerializer deserializer(scene.get());
    if (!deserializer.deserialize("Assets/Scenes/scene.json")) {
        Axiom::AX_LOG_ERROR("Failed to deserialize the scene");
    }

    Axiom::UUID textureHandle = Axiom::AssetManager::importAsset("Redstone Block", "Assets/Textures/redstone_block.png", Axiom::AssetType::Texture);
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
    Axiom::SceneSerializer serializer(scene.get());
    serializer.serialize("Assets/Scenes/scene.json");
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
    if (shouldDeleteContextMenu) {
        contextMenu = nullptr;
        shouldDeleteContextMenu = false;
    }
    if (shouldDeleteAssetPicker) {
        assetPickerMenu = nullptr;
        shouldDeleteAssetPicker = false;
    }
    refreshProfilerPanel();
    uiRoot->arrange(mainUiContext, Math::Vec2(0, 0), Math::Vec2(winWidth, winHeight));
}

void EditorLayer::onUIRender() {
    mainUiContext.renderer->pushScissorRect({{0, 0}, {Axiom::Locator::getWindow()->getWidth(), Axiom::Locator::getWindow()->getHeight()}}, mainUiContext.layer);
    uiRoot->onRender(mainUiContext, Math::Rect({0, 0}, {Axiom::Locator::getWindow()->getWidth(), Axiom::Locator::getWindow()->getHeight()}));

    if (contextMenu) {
        mainUiContext.layer = 1;
        mainUiContext.renderer->pushScissorRect({{0, 0}, {Axiom::Locator::getWindow()->getWidth(), Axiom::Locator::getWindow()->getHeight()}},
                                                mainUiContext.layer);
        contextMenu->onRender(mainUiContext, Math::Rect({0, 0}, {Axiom::Locator::getWindow()->getWidth(), Axiom::Locator::getWindow()->getHeight()}));
        mainUiContext.renderer->popScissorRect(mainUiContext.layer);
        mainUiContext.layer = 0;
    }
    if (assetPickerMenu) {
        mainUiContext.layer = 1;
        mainUiContext.renderer->pushScissorRect({{0, 0}, {Axiom::Locator::getWindow()->getWidth(), Axiom::Locator::getWindow()->getHeight()}},
                                                mainUiContext.layer);
        assetPickerMenu->onRender(mainUiContext, Math::Rect({0, 0}, {Axiom::Locator::getWindow()->getWidth(), Axiom::Locator::getWindow()->getHeight()}));
        mainUiContext.renderer->popScissorRect(mainUiContext.layer);
        mainUiContext.layer = 0;
    }
    mainUiContext.renderer->popScissorRect(mainUiContext.layer);
}

void EditorLayer::onEvent(Axiom::Event& event) {
    Axiom::EventDispatcher dispatcher(event);

    dispatcher.dispatch<Axiom::MouseMovedEvent>([this](const Axiom::MouseMovedEvent& e) {
        lastMousePos = {e.getMouseX(), e.getMouseY()};
        return false;
    });

    dispatcher.dispatch<Axiom::MouseButtonPressedEvent>([this](const Axiom::MouseButtonPressedEvent& e) {
        if (e.getMouseButton() == Axiom::KeyCode::RightButton) {
            auto bounds = hierarchyPanel->getArrangedPosition();
            auto size = hierarchyPanel->getArrangedSize();

            if (lastMousePos.x() >= bounds.x() && lastMousePos.x() <= bounds.x() + size.x() && lastMousePos.y() >= bounds.y() &&
                lastMousePos.y() <= bounds.y() + size.y()) {
                spawnHierarchyContextMenu();
                return true;
            }
        }
        if (e.getMouseButton() == Axiom::KeyCode::LeftButton) {
            if (contextMenu) {
                auto bounds = contextMenu->getArrangedPosition();
                auto size = contextMenu->getArrangedSize();
                if (lastMousePos.x() < bounds.x() || lastMousePos.x() > bounds.x() + size.x() || lastMousePos.y() < bounds.y() ||
                    lastMousePos.y() > bounds.y() + size.y()) {
                    shouldDeleteContextMenu = true;
                    return true;
                }
            }
            if (assetPickerMenu) {
                auto bounds = assetPickerMenu->getArrangedPosition();
                auto size = assetPickerMenu->getArrangedSize();
                if (lastMousePos.x() < bounds.x() || lastMousePos.x() > bounds.x() + size.x() || lastMousePos.y() < bounds.y() ||
                    lastMousePos.y() > bounds.y() + size.y()) {
                    shouldDeleteAssetPicker = true;
                    return true;
                }
            }
        }
        return false;
    });

    if (contextMenu) {
        event.handled = contextMenu->onEvent(event);
    } else if (assetPickerMenu) {
        event.handled = assetPickerMenu->onEvent(event);
    } else {
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

        entityButton->setOnClick([this, entity]() {
            selectedEntity = entity;
            shouldRefreshHierarchy = true;
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
            shouldRefreshHierarchy = true;
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
            void* fieldPtr = static_cast<char*>(dataPtr) + field.offset;
            auto fieldUI = createFieldUI(field, fieldPtr);

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

void EditorLayer::spawnHierarchyContextMenu() {
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
        shouldDeleteContextMenu = true;
    });

    contextMenu->addChild(createBtn);
    contextMenu->arrange(mainUiContext, lastMousePos, contextMenu->getDesiredSize(mainUiContext));
}

void EditorLayer::refreshProfilerPanel() {
    profilerPanel->clearChildren();

    auto headerRow = std::make_shared<Axiom::UIHorizontalBox>();
    headerRow->setVerticalAlignment(Axiom::UIAlignment::Start);
    headerRow->setMargin({0.0f, 0.0f, 0.0f, 10.0f});

    auto headerText = std::make_shared<Axiom::UIText>("Profiler");
    headerText->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    headerRow->addChild(headerText);
    profilerPanel->addChild(headerRow);

    const auto& profiles = Axiom::Profiler::getProfiles();
    for (const auto& profile : profiles) {
        auto ms = std::chrono::duration_cast<std::chrono::duration<double>>(profile.duration);
        auto labelText = std::format("{}: {:.4f} ms", profile.name, ms.count());
        auto label = std::make_shared<Axiom::UIText>(labelText);
        label->setVerticalAlignment(Axiom::UIAlignment::Start);
        profilerPanel->addChild(label);
    }
}

std::shared_ptr<Axiom::UIElement> EditorLayer::createFieldUI(const Axiom::FieldInfo& field, void* fieldPtr) {
    auto horizontalBox = std::make_shared<Axiom::UIHorizontalBox>();
    horizontalBox->setMargin({0.0f, 0.0f, 0.0f, 4.0f});

    std::string displayName;
    for (size_t i = 0; i < field.name.length(); ++i) {
        if (i == 0) {
            displayName += std::toupper(field.name[i]);
        } else if (std::isupper(field.name[i])) {
            displayName += " ";
            displayName += field.name[i];
        } else {
            displayName += field.name[i];
        }
    }

    auto label = std::make_shared<Axiom::UIText>(displayName);
    label->setFixedSize({120.0f, -1.0f});
    label->setVerticalAlignment(Axiom::UIAlignment::Start);
    label->setColor(uiRoot->getTheme()->textMutedColor);
    horizontalBox->addChild(label);

    switch (field.type) {
    case Axiom::FieldType::Float:
        buildFloatUI(horizontalBox, field, fieldPtr);
        break;
    case Axiom::FieldType::Int:
        buildIntUI(horizontalBox, field, fieldPtr);
        break;
    case Axiom::FieldType::Bool:
        buildBoolUI(horizontalBox, field, fieldPtr);
        break;
    case Axiom::FieldType::String:
        buildStringUI(horizontalBox, field, fieldPtr);
        break;
    case Axiom::FieldType::Vec2:
        buildVec2UI(horizontalBox, field, fieldPtr);
        break;
    case Axiom::FieldType::Vec3:
        buildVec3UI(horizontalBox, field, fieldPtr);
        break;
    case Axiom::FieldType::Vec4:
        buildVec4UI(horizontalBox, field, fieldPtr);
        break;
    case Axiom::FieldType::Color:
        buildColorUI(horizontalBox, field, fieldPtr);
        break;
    case Axiom::FieldType::AssetHandle:
        buildAssetHandleUI(horizontalBox, field, fieldPtr);
        break;
    case Axiom::FieldType::Enum:
        buildEnumUI(horizontalBox, field, fieldPtr);
        break;
    default:
        break;
    }

    return horizontalBox;
}

void EditorLayer::buildFloatUI(std::shared_ptr<Axiom::UIHorizontalBox> horizontalBox, const Axiom::FieldInfo& field, void* fieldPtr) {
    float* valuePtr = static_cast<float*>(fieldPtr);
    auto drag = std::make_shared<Axiom::UIScalarField<float>>();
    drag->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    drag->setValueGetter([valuePtr]() { return *valuePtr; });
    drag->setValueSetter([valuePtr](float v) { *valuePtr = v; });
    horizontalBox->addChild(drag);
}

void EditorLayer::buildIntUI(std::shared_ptr<Axiom::UIHorizontalBox> horizontalBox, const Axiom::FieldInfo& field, void* fieldPtr) {
    int* valuePtr = static_cast<int*>(fieldPtr);
    auto drag = std::make_shared<Axiom::UIScalarField<int>>();
    drag->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    drag->setValueGetter([valuePtr]() { return *valuePtr; });
    drag->setValueSetter([valuePtr](int v) { *valuePtr = v; });
    horizontalBox->addChild(drag);
}

void EditorLayer::buildBoolUI(std::shared_ptr<Axiom::UIHorizontalBox> horizontalBox, const Axiom::FieldInfo& field, void* fieldPtr) {
    bool* valuePtr = static_cast<bool*>(fieldPtr);
    auto checkbox = std::make_shared<Axiom::UICheckbox>();
    checkbox->setHorizontalAlignment(Axiom::UIAlignment::Start);
    checkbox->setValueGetter([valuePtr]() { return *valuePtr; });
    checkbox->setValueSetter([valuePtr](bool v) { *valuePtr = v; });
    horizontalBox->addChild(checkbox);
}

void EditorLayer::buildStringUI(std::shared_ptr<Axiom::UIHorizontalBox> horizontalBox, const Axiom::FieldInfo& field, void* fieldPtr) {
    std::string* valuePtr = static_cast<std::string*>(fieldPtr);
    auto textInput = std::make_shared<Axiom::UITextInput>();
    textInput->setValueGetter([valuePtr]() { return *valuePtr; });
    textInput->setValueSetter([this, valuePtr](const std::string& v) {
        *valuePtr = v;
        shouldRefreshHierarchy = true;
    });
    horizontalBox->addChild(textInput);
}

void EditorLayer::buildVec2UI(std::shared_ptr<Axiom::UIHorizontalBox> horizontalBox, const Axiom::FieldInfo& field, void* fieldPtr) {
    Math::Vec2* valuePtr = static_cast<Math::Vec2*>(fieldPtr);
    auto createAxis = [&](Axiom::Color color, auto getter, auto setter) {
        auto drag = std::make_shared<Axiom::UIScalarField<float>>();
        drag->setHorizontalAlignment(Axiom::UIAlignment::Fill);
        drag->setValueGetter(getter);
        drag->setValueSetter(setter);
        drag->setNormalColor(color);
        horizontalBox->addChild(drag);
    };

    createAxis(Axiom::Color(0.9f, 0.1f, 0.1f), [valuePtr]() { return valuePtr->x(); }, [valuePtr](float v) { valuePtr->x() = v; });
    createAxis(Axiom::Color(0.1f, 0.9f, 0.1f), [valuePtr]() { return valuePtr->y(); }, [valuePtr](float v) { valuePtr->y() = v; });
}

void EditorLayer::buildVec3UI(std::shared_ptr<Axiom::UIHorizontalBox> horizontalBox, const Axiom::FieldInfo& field, void* fieldPtr) {
    Math::Vec3* valuePtr = static_cast<Math::Vec3*>(fieldPtr);

    auto createAxis = [&](Axiom::Color color, auto getter, auto setter) {
        auto drag = std::make_shared<Axiom::UIScalarField<float>>();
        drag->setHorizontalAlignment(Axiom::UIAlignment::Fill);
        drag->setValueGetter(getter);
        drag->setValueSetter(setter);
        drag->setNormalColor(color);
        horizontalBox->addChild(drag);
    };

    createAxis(Axiom::Color(0.9f, 0.1f, 0.1f), [valuePtr]() { return valuePtr->x(); }, [valuePtr](float v) { valuePtr->x() = v; });
    createAxis(Axiom::Color(0.1f, 0.9f, 0.1f), [valuePtr]() { return valuePtr->y(); }, [valuePtr](float v) { valuePtr->y() = v; });
    createAxis(Axiom::Color(0.1f, 0.1f, 0.9f), [valuePtr]() { return valuePtr->z(); }, [valuePtr](float v) { valuePtr->z() = v; });
}

void EditorLayer::buildVec4UI(std::shared_ptr<Axiom::UIHorizontalBox> horizontalBox, const Axiom::FieldInfo& field, void* fieldPtr) {
    Math::Vec4* valuePtr = static_cast<Math::Vec4*>(fieldPtr);

    auto createAxis = [&](Axiom::Color color, auto getter, auto setter) {
        auto drag = std::make_shared<Axiom::UIScalarField<float>>();
        drag->setHorizontalAlignment(Axiom::UIAlignment::Fill);
        drag->setValueGetter(getter);
        drag->setValueSetter(setter);
        drag->setNormalColor(color);
        horizontalBox->addChild(drag);
    };

    createAxis(Axiom::Color(0.9f, 0.1f, 0.1f), [valuePtr]() { return valuePtr->x(); }, [valuePtr](float v) { valuePtr->x() = v; });
    createAxis(Axiom::Color(0.1f, 0.9f, 0.1f), [valuePtr]() { return valuePtr->y(); }, [valuePtr](float v) { valuePtr->y() = v; });
    createAxis(Axiom::Color(0.1f, 0.1f, 0.9f), [valuePtr]() { return valuePtr->z(); }, [valuePtr](float v) { valuePtr->z() = v; });
    createAxis(Axiom::Color(0.9f, 0.9f, 0.1f), [valuePtr]() { return valuePtr->w(); }, [valuePtr](float v) { valuePtr->w() = v; });
}

void EditorLayer::buildColorUI(std::shared_ptr<Axiom::UIHorizontalBox> horizontalBox, const Axiom::FieldInfo& field, void* fieldPtr) {
    Axiom::Color* valuePtr = static_cast<Axiom::Color*>(fieldPtr);
    // TODO: add a color picker UI element and use it here
    auto colorPreview = std::make_shared<Axiom::UIButton>("");
    colorPreview->setHorizontalAlignment(Axiom::UIAlignment::Start);
    colorPreview->setFixedSize({20.0f, 20.0f});
    colorPreview->setNormalColor(*valuePtr);
    colorPreview->setHoverColor(*valuePtr);
    colorPreview->setActiveColor(*valuePtr);
    horizontalBox->addChild(colorPreview);

    auto createChannelSlider = [&](const std::string& channelName, auto getter, auto setter) {
        auto slider = std::make_shared<Axiom::UIScalarField<float>>();
        slider->setHorizontalAlignment(Axiom::UIAlignment::Fill);
        slider->setValueGetter(getter);
        slider->setValueSetter(setter);
        slider->setNormalColor(Axiom::Color(0.5f, 0.5f, 0.5f));
        slider->setLimits(0.0f, 1.0f);
        horizontalBox->addChild(slider);
    };

    createChannelSlider(
        "R", [valuePtr]() { return valuePtr->r(); },
        [valuePtr, colorPreview](float v) {
            valuePtr->r() = v;
            colorPreview->setNormalColor(*valuePtr);
            colorPreview->setHoverColor(*valuePtr);
            colorPreview->setActiveColor(*valuePtr);
        });
    createChannelSlider(
        "G", [valuePtr]() { return valuePtr->g(); },
        [valuePtr, colorPreview](float v) {
            valuePtr->g() = v;
            colorPreview->setNormalColor(*valuePtr);
            colorPreview->setHoverColor(*valuePtr);
            colorPreview->setActiveColor(*valuePtr);
        });
    createChannelSlider(
        "B", [valuePtr]() { return valuePtr->b(); },
        [valuePtr, colorPreview](float v) {
            valuePtr->b() = v;
            colorPreview->setNormalColor(*valuePtr);
            colorPreview->setHoverColor(*valuePtr);
            colorPreview->setActiveColor(*valuePtr);
        });
    createChannelSlider(
        "A", [valuePtr]() { return valuePtr->a(); },
        [valuePtr, colorPreview](float v) {
            valuePtr->a() = v;
            colorPreview->setNormalColor(*valuePtr);
            colorPreview->setHoverColor(*valuePtr);
            colorPreview->setActiveColor(*valuePtr);
        });
}

void EditorLayer::buildAssetHandleUI(std::shared_ptr<Axiom::UIHorizontalBox> horizontalBox, const Axiom::FieldInfo& field, void* fieldPtr) {
    Axiom::UUID* valuePtr = static_cast<Axiom::UUID*>(fieldPtr);

    auto slotBox = std::make_shared<Axiom::UIHorizontalBox>();
    slotBox->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    slotBox->setPadding({4.0f, 4.0f, 4.0f, 4.0f});

    auto getAssetName = [valuePtr]() -> std::string {
        if (valuePtr->isValid()) {
            return Axiom::AssetManager::getMetadata(*valuePtr).name;
        }
        return "None";
    };

    auto assetButton = std::make_shared<Axiom::UIButton>(getAssetName());
    assetButton->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    assetButton->setMargin({0.0f, 0.0f, 4.0f, 0.0f});
    assetButton->setOnClick([this, valuePtr, assetButton, field]() { buildAssetPickerPopUp(valuePtr, assetButton, field); });

    auto clearButton = std::make_shared<Axiom::UIButton>("X");
    clearButton->setFixedSize({24.0f, 24.0f});
    clearButton->setNormalColor(uiRoot->getTheme()->errorColor);
    clearButton->setOnClick([valuePtr, assetButton, getAssetName]() {
        *valuePtr = Axiom::UUID();
        assetButton->setText(getAssetName());
    });

    slotBox->addChild(assetButton);
    slotBox->addChild(clearButton);
    horizontalBox->addChild(slotBox);
}

void EditorLayer::buildEnumUI(std::shared_ptr<Axiom::UIHorizontalBox> horizontalBox, const Axiom::FieldInfo& field, void* fieldPtr) {
    int* valuePtr = static_cast<int*>(fieldPtr);
    auto dropdown = std::make_shared<Axiom::UIDropdown>(field.enumOptions);
    dropdown->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    dropdown->setSelectedIndex(*valuePtr);
    dropdown->setOnSelectionChanged([this, valuePtr](int index, const std::string& optionText) { *valuePtr = index; });

    horizontalBox->addChild(dropdown);
}

void EditorLayer::buildAssetPickerPopUp(Axiom::UUID* valuePtr, std::shared_ptr<Axiom::UIButton> assetButton, const Axiom::FieldInfo& field) {
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

            btn->setOnClick([this, valuePtr, assetButton, assetID, assetName]() {
                *valuePtr = assetID;
                assetButton->setText(assetName);

                shouldDeleteAssetPicker = true;
                shouldRefreshInspector = true;
            });

            vBox->addChild(btn);
        }
    }

    Math::Vec2 spawnPos = assetButton->getArrangedPosition() + Math::Vec2(0.0f, assetButton->getArrangedSize().y() + 4.0f);
    assetPickerMenu->arrange(mainUiContext, spawnPos, assetPickerMenu->getDesiredSize(mainUiContext));
}
