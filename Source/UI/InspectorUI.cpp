#include "InspectorUI.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

std::shared_ptr<Axiom::UIElement> InspectorUI::createFieldUI(Axiom::Entity entity, uint8_t componentId, const Axiom::FieldInfo& field,
                                                             const Axiom::UITheme* theme) {
    auto horizontalBox = std::make_shared<Axiom::UIHorizontalBox>();

    std::string displayName = "";
    for (size_t i = 0; i < field.name.size(); i++) {
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
    horizontalBox->addSlot(label).setFixedSize({120.0f, -1.0f}).setVerticalAlignment(Axiom::UIAlignment::Start);

    switch (field.type) {
    case Axiom::FieldType::Float:
        buildFloatUI(horizontalBox, entity, componentId, field);
        break;
    case Axiom::FieldType::Int:
        buildIntUI(horizontalBox, entity, componentId, field);
        break;
    case Axiom::FieldType::Bool:
        buildBoolUI(horizontalBox, entity, componentId, field);
        break;
    case Axiom::FieldType::String:
        buildStringUI(horizontalBox, entity, componentId, field);
        break;
    case Axiom::FieldType::Vec2:
        buildVec2UI(horizontalBox, entity, componentId, field);
        break;
    case Axiom::FieldType::Vec3:
        buildVec3UI(horizontalBox, entity, componentId, field);
        break;
    case Axiom::FieldType::Vec4:
        buildVec4UI(horizontalBox, entity, componentId, field);
        break;
    case Axiom::FieldType::Color:
        buildColorUI(horizontalBox, entity, componentId, field, theme);
        break;
    case Axiom::FieldType::AssetHandle:
        buildAssetHandleUI(horizontalBox, entity, componentId, field, theme);
        break;
    case Axiom::FieldType::Enum:
        buildEnumUI(horizontalBox, entity, componentId, field);
        break;
    default:
        break;
    }

    return horizontalBox;
}

void InspectorUI::buildFloatUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, uint8_t componentId, const Axiom::FieldInfo& field) {
    auto drag = std::make_shared<Axiom::UIScalarField<float>>();
    drag->setValueGetter([entity, componentId, offset = field.offset]() -> float {
        void* compData = entity.getComponentData(componentId);
        if (!compData) {
            return 0.0f;
        }
        return *reinterpret_cast<float*>(static_cast<char*>(compData) + offset);
    });
    drag->setValueSetter([entity, componentId, offset = field.offset](float newValue) mutable {
        void* compData = entity.getComponentData(componentId);
        if (compData) {
            *reinterpret_cast<float*>(static_cast<char*>(compData) + offset) = newValue;
        }
    });
    box->addSlot(drag).setHorizontalAlignment(Axiom::UIAlignment::Fill);
}

void InspectorUI::buildIntUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, uint8_t componentId, const Axiom::FieldInfo& field) {
    auto drag = std::make_shared<Axiom::UIScalarField<int>>();
    drag->setValueGetter([entity, componentId, offset = field.offset] -> int {
        void* compData = entity.getComponentData(componentId);
        if (!compData) {
            return 0;
        }
        return *reinterpret_cast<int*>(static_cast<char*>(compData) + offset);
    });
    drag->setValueSetter([entity, componentId, offset = field.offset](int newValue) mutable {
        void* compData = entity.getComponentData(componentId);
        if (compData) {
            *reinterpret_cast<int*>(static_cast<char*>(compData) + offset) = newValue;
        }
    });
    box->addSlot(drag).setHorizontalAlignment(Axiom::UIAlignment::Fill);
}

void InspectorUI::buildBoolUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, uint8_t componentId, const Axiom::FieldInfo& field) {
    auto checkBox = std::make_shared<Axiom::UICheckbox>();
    checkBox->setValueGetter([entity, componentId, offset = field.offset]() -> bool {
        void* compData = entity.getComponentData(componentId);
        if (!compData) {
            return false;
        }
        return *reinterpret_cast<bool*>(static_cast<char*>(compData) + offset);
    });
    checkBox->setValueSetter([entity, componentId, offset = field.offset](bool newValue) mutable {
        void* compData = entity.getComponentData(componentId);
        if (compData) {
            *reinterpret_cast<bool*>(static_cast<char*>(compData) + offset) = newValue;
        }
    });
    box->addSlot(checkBox).setHorizontalAlignment(Axiom::UIAlignment::Start);
}

void InspectorUI::buildStringUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, uint8_t componentId, const Axiom::FieldInfo& field) {
    auto textInput = std::make_shared<Axiom::UITextInput>();
    textInput->setValueGetter([entity, componentId, offset = field.offset]() -> std::string {
        void* compData = entity.getComponentData(componentId);
        if (!compData) {
            return "";
        }
        return *reinterpret_cast<std::string*>(static_cast<char*>(compData) + offset);
    });
    textInput->setValueSetter([entity, componentId, offset = field.offset](const std::string& newValue) {
        void* compData = entity.getComponentData(componentId);
        if (compData) {
            *reinterpret_cast<std::string*>(static_cast<char*>(compData) + offset) = newValue;
        }
    });
    box->addSlot(textInput);
}

void InspectorUI::buildVec2UI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, uint8_t componentId, const Axiom::FieldInfo& field) {
    auto createAxis = [&](Axiom::Color color, auto getter, auto setter) {
        auto drag = std::make_shared<Axiom::UIScalarField<float>>();
        drag->setValueGetter(getter);
        drag->setValueSetter(setter);
        drag->setNormalColor(color);
        box->addSlot(drag).setHorizontalAlignment(Axiom::UIAlignment::Fill);
    };

    createAxis(
        Axiom::Color(0.9f, 0.1f, 0.1f),
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec2*>(static_cast<char*>(compData) + offset)->x();
        },
        [entity, componentId, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData)
                reinterpret_cast<Math::Vec2*>(static_cast<char*>(compData) + offset)->x() = v;
        });

    createAxis(
        Axiom::Color(0.1f, 0.9f, 0.1f),
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec2*>(static_cast<char*>(compData) + offset)->y();
        },
        [entity, componentId, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData)
                reinterpret_cast<Math::Vec2*>(static_cast<char*>(compData) + offset)->y() = v;
        });
}

void InspectorUI::buildVec3UI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, uint8_t componentId, const Axiom::FieldInfo& field) {
    auto createAxis = [&](Axiom::Color color, auto getter, auto setter) {
        auto drag = std::make_shared<Axiom::UIScalarField<float>>();
        drag->setValueGetter(getter);
        drag->setValueSetter(setter);
        drag->setNormalColor(color);
        box->addSlot(drag).setHorizontalAlignment(Axiom::UIAlignment::Fill);
    };

    createAxis(
        Axiom::Color(0.9f, 0.1f, 0.1f),
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec3*>(static_cast<char*>(compData) + offset)->x();
        },
        [entity, componentId, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData)
                reinterpret_cast<Math::Vec3*>(static_cast<char*>(compData) + offset)->x() = v;
        });

    createAxis(
        Axiom::Color(0.1f, 0.9f, 0.1f),
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec3*>(static_cast<char*>(compData) + offset)->y();
        },
        [entity, componentId, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData)
                reinterpret_cast<Math::Vec3*>(static_cast<char*>(compData) + offset)->y() = v;
        });

    createAxis(
        Axiom::Color(0.1f, 0.1f, 0.9f),
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec3*>(static_cast<char*>(compData) + offset)->z();
        },
        [entity, componentId, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData)
                reinterpret_cast<Math::Vec3*>(static_cast<char*>(compData) + offset)->z() = v;
        });
}

void InspectorUI::buildVec4UI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, uint8_t componentId, const Axiom::FieldInfo& field) {
    auto createAxis = [&](Axiom::Color color, auto getter, auto setter) {
        auto drag = std::make_shared<Axiom::UIScalarField<float>>();
        drag->setValueGetter(getter);
        drag->setValueSetter(setter);
        drag->setNormalColor(color);
        box->addSlot(drag).setHorizontalAlignment(Axiom::UIAlignment::Fill);
    };

    createAxis(
        Axiom::Color(0.9f, 0.1f, 0.1f),
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->x();
        },
        [entity, componentId, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData)
                reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->x() = v;
        });

    createAxis(
        Axiom::Color(0.1f, 0.9f, 0.1f),
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->y();
        },
        [entity, componentId, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData)
                reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->y() = v;
        });

    createAxis(
        Axiom::Color(0.1f, 0.1f, 0.9f),
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->z();
        },
        [entity, componentId, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData)
                reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->z() = v;
        });

    createAxis(
        Axiom::Color(0.9f, 0.9f, 0.1f),
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->w();
        },
        [entity, componentId, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData)
                reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->w() = v;
        });
}

void InspectorUI::buildColorUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, uint8_t componentId, const Axiom::FieldInfo& field,
                               const Axiom::UITheme* theme) {
    auto colorPreview = std::make_shared<Axiom::UIButton>();

    void* initialData = entity.getComponentData(componentId);
    if (initialData) {
        Axiom::Color initialColor = *reinterpret_cast<Axiom::Color*>(static_cast<char*>(initialData) + field.offset);
        colorPreview->setNormalColor(initialColor);
        colorPreview->setHoverColor(initialColor);
        colorPreview->setActiveColor(initialColor);
    }

    box->addSlot(colorPreview).setFixedSize({theme->defaultRowHeight, theme->defaultRowHeight}).setHorizontalAlignment(Axiom::UIAlignment::Start);

    auto updatePreview = [colorPreview, entity, componentId, offset = field.offset]() {
        void* compData = entity.getComponentData(componentId);
        if (compData) {
            Axiom::Color c = *reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset);
            colorPreview->setNormalColor(c);
            colorPreview->setHoverColor(c);
            colorPreview->setActiveColor(c);
        }
    };

    auto createChannelSlider = [&](auto getter, auto setter) {
        auto slider = std::make_shared<Axiom::UIScalarField<float>>();
        slider->setValueGetter(getter);
        slider->setValueSetter(setter);
        slider->setNormalColor(Axiom::Color(0.5f, 0.5f, 0.5f));
        slider->setLimits(0.0f, 1.0f);
        box->addSlot(slider).setHorizontalAlignment(Axiom::UIAlignment::Fill);
    };

    createChannelSlider(
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->r();
        },
        [entity, componentId, offset = field.offset, updatePreview](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData) {
                reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->r() = v;
                updatePreview();
            }
        });

    createChannelSlider(
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->g();
        },
        [entity, componentId, offset = field.offset, updatePreview](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData) {
                reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->g() = v;
                updatePreview();
            }
        });

    createChannelSlider(
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->b();
        },
        [entity, componentId, offset = field.offset, updatePreview](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData) {
                reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->b() = v;
                updatePreview();
            }
        });

    createChannelSlider(
        [entity, componentId, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(componentId);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->a();
        },
        [entity, componentId, offset = field.offset, updatePreview](float v) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData) {
                reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->a() = v;
                updatePreview();
            }
        });
}

void InspectorUI::buildAssetHandleUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, uint8_t componentId, const Axiom::FieldInfo& field,
                                     const Axiom::UITheme* theme) {
    auto slotBox = std::make_shared<Axiom::UIHorizontalBox>();

    auto getAssetName = [entity, componentId, offset = field.offset]() -> std::string {
        void* compData = entity.getComponentData(componentId);
        if (!compData)
            return "None";
        Axiom::UUID id = *reinterpret_cast<Axiom::UUID*>(static_cast<char*>(compData) + offset);
        if (id.isValid())
            return Axiom::AssetManager::getMetadata(id).name;
        return "None";
    };

    auto assetButton = std::make_shared<Axiom::UIButton>();
    assetButton->setText(getAssetName());
    assetButton->setOnClick([assetButton, field, entity, componentId, offset = field.offset, getAssetName, theme]() {
        Axiom::UICanvas* canvas = assetButton->getParentCanvas();
        if (!canvas)
            return;

        Math::Vec2 modalSize(340.0f, 250.0f);

        auto modalPanel = std::make_shared<Axiom::UIPanel>();
        modalPanel->setBackgroundColor(theme->panelBackgroundColor);

        auto mainLayout = std::make_shared<Axiom::UIVerticalBox>();

        auto headerBox = std::make_shared<Axiom::UIHorizontalBox>();

        auto titleText = std::make_shared<Axiom::UIText>("Select Asset: " + field.name);

        auto closeBtn = std::make_shared<Axiom::UIButton>();
        closeBtn->setText("X");
        closeBtn->setNormalColor(Axiom::Color(0.6f, 0.2f, 0.2f, 1.0f));
        closeBtn->setOnClick([canvas]() { canvas->closePopup(); });

        headerBox->addSlot(titleText).setAlignment(Axiom::UIAlignment::Start, Axiom::UIAlignment::Center);
        headerBox->addSlot(closeBtn)
            .setFixedSize({theme->defaultRowHeight, theme->defaultRowHeight})
            .setAlignment(Axiom::UIAlignment::End, Axiom::UIAlignment::Center);

        auto scrollBox = std::make_shared<Axiom::UIScrollBox>();

        std::vector<Axiom::UUID> assetsIds = Axiom::AssetManager::getAssetsByType(field.assetType);
        for (const auto& id : assetsIds) {
            std::string assetName = Axiom::AssetManager::getMetadata(id).name;
            auto itemBtn = std::make_shared<Axiom::UIButton>();
            itemBtn->setText(assetName);
            itemBtn->setOnClick([entity, componentId, offset, id, assetButton, getAssetName, canvas]() {
                void* componentData = entity.getComponentData(componentId);
                if (componentData) {
                    *reinterpret_cast<Axiom::UUID*>(static_cast<char*>(componentData) + offset) = id;
                    assetButton->setText(getAssetName());
                }
                canvas->closePopup();
            });

            scrollBox->addSlot(itemBtn).setHorizontalAlignment(Axiom::UIAlignment::Fill).setMargin({2.0f, 2.0f, 2.0f, 2.0f});
        }

        mainLayout->addSlot(headerBox).setMargin(theme->containerPadding).setHorizontalAlignment(Axiom::UIAlignment::Fill);
        mainLayout->addSlot(scrollBox).setAlignment(Axiom::UIAlignment::Fill, Axiom::UIAlignment::Fill);

        modalPanel->addSlot(mainLayout).setAlignment(Axiom::UIAlignment::Fill, Axiom::UIAlignment::Fill);

        Math::Vec2 screenSize = Math::Vec2(Axiom::Locator::getWindow()->getWidth(), Axiom::Locator::getWindow()->getHeight());
        Math::Vec2 modalPos = (screenSize - modalSize) * 0.5f;

        canvas->openPopup(modalPanel, modalPos);
    });

    auto clearButton = std::make_shared<Axiom::UIButton>();
    clearButton->setText("X");
    clearButton->setNormalColor(Axiom::Color::red());
    clearButton->setOnClick([entity, componentId, offset = field.offset, assetButton, getAssetName]() {
        void* compData = entity.getComponentData(componentId);
        if (compData) {
            *reinterpret_cast<Axiom::UUID*>(static_cast<char*>(compData) + offset) = Axiom::UUID();
            assetButton->setText(getAssetName());
        }
    });

    slotBox->addSlot(assetButton).setHorizontalAlignment(Axiom::UIAlignment::Fill).setMargin({0.0f, 0.0f, 4.0f, 0.0f});
    slotBox->addSlot(clearButton).setFixedSize({theme->defaultRowHeight, theme->defaultRowHeight});
    box->addSlot(slotBox).setHorizontalAlignment(Axiom::UIAlignment::Fill);
}

void InspectorUI::buildEnumUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, uint8_t componentId, const Axiom::FieldInfo& field) {
    auto dropdown = std::make_shared<Axiom::UIDropdown>();
    dropdown->setOptions(field.enumOptions);

    dropdown->setOptions(field.enumOptions)
        .setValueGetter([entity, componentId, offset = field.offset]() -> int {
            void* compData = entity.getComponentData(componentId);
            if (compData) {
                return *reinterpret_cast<int*>(static_cast<char*>(compData) + offset);
            }
            return 0;
        })
        .setValueSetter([entity, componentId, offset = field.offset](int index) mutable {
            void* compData = entity.getComponentData(componentId);
            if (compData) {
                *reinterpret_cast<int*>(static_cast<char*>(compData) + offset) = index;
            }
        });

    box->addSlot(dropdown).setHorizontalAlignment(Axiom::UIAlignment::Fill);
}
