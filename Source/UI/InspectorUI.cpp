#include "InspectorUI.h"

#include "Math/Color.h"
#include "UI/Elements/UICheckbox.h"
#include "UI/Elements/UIElement.h"
#include "UI/Elements/UIScalarField.h"
#include "UI/Elements/UITextInput.h"

#include <functional>
#include <memory>
#include <string>

std::shared_ptr<Axiom::UIElement> InspectorUI::createFieldUI(Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field,
                                                             const std::shared_ptr<Axiom::UITheme>& theme) {
    auto horizontalBox = std::make_shared<Axiom::UIHorizontalBox>();
    horizontalBox->setMargin({0.0f, 0.0f, 0.0f, 4.0f});

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
    label->setFixedSize({120.0f, -1.0f});
    label->setVerticalAlignment(Axiom::UIAlignment::Start);
    label->setColor(theme->textMutedColor);
    horizontalBox->addChild(label);

    switch (field.type) {
    case Axiom::FieldType::Float:
        buildFloatUI(horizontalBox, entity, compTypeIndex, field);
        break;
    case Axiom::FieldType::Int:
        buildIntUI(horizontalBox, entity, compTypeIndex, field);
        break;
    case Axiom::FieldType::Bool:
        buildBoolUI(horizontalBox, entity, compTypeIndex, field);
        break;
    case Axiom::FieldType::String:
        buildStringUI(horizontalBox, entity, compTypeIndex, field);
        break;
    case Axiom::FieldType::Vec2:
        buildVec2UI(horizontalBox, entity, compTypeIndex, field);
        break;
    case Axiom::FieldType::Vec3:
        buildVec3UI(horizontalBox, entity, compTypeIndex, field);
        break;
    case Axiom::FieldType::Vec4:
        buildVec4UI(horizontalBox, entity, compTypeIndex, field);
        break;
    case Axiom::FieldType::Color:
        buildColorUI(horizontalBox, entity, compTypeIndex, field, theme);
        break;
    case Axiom::FieldType::AssetHandle:
        buildAssetHandleUI(horizontalBox, entity, compTypeIndex, field, theme);
        break;
    case Axiom::FieldType::Enum:
        buildEnumUI(horizontalBox, entity, compTypeIndex, field);
        break;
    default:
        break;
    }

    return horizontalBox;
}

void InspectorUI::buildFloatUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex,
                               const Axiom::FieldInfo& field) {
    auto drag = std::make_shared<Axiom::UIScalarField<float>>();
    drag->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    drag->setValueGetter([entity, compTypeIndex, offset = field.offset]() -> float {
        void* compData = entity.getComponentData(compTypeIndex);
        if (!compData) {
            return 0.0f;
        }
        return *reinterpret_cast<float*>(static_cast<char*>(compData) + offset);
    });
    drag->setValueSetter([entity, compTypeIndex, offset = field.offset](float newValue) mutable {
        void* compData = entity.getComponentData(compTypeIndex);
        if (compData) {
            *reinterpret_cast<float*>(static_cast<char*>(compData) + offset) = newValue;
        }
    });
    box->addChild(drag);
}

void InspectorUI::buildIntUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field) {
    auto drag = std::make_shared<Axiom::UIScalarField<int>>();
    drag->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    drag->setValueGetter([entity, compTypeIndex, offset = field.offset] -> int {
        void* compData = entity.getComponentData(compTypeIndex);
        if (!compData) {
            return 0;
        }
        return *reinterpret_cast<int*>(static_cast<char*>(compData) + offset);
    });
    drag->setValueSetter([entity, compTypeIndex, offset = field.offset](int newValue) mutable {
        void* compData = entity.getComponentData(compTypeIndex);
        if (compData) {
            *reinterpret_cast<int*>(static_cast<char*>(compData) + offset) = newValue;
        }
    });
    box->addChild(drag);
}

void InspectorUI::buildBoolUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field) {
    auto checkBox = std::make_shared<Axiom::UICheckbox>();
    checkBox->setHorizontalAlignment(Axiom::UIAlignment::Start);
    checkBox->setValueGetter([entity, compTypeIndex, offset = field.offset]() -> bool {
        void* compData = entity.getComponentData(compTypeIndex);
        if (!compData) {
            return false;
        }
        return *reinterpret_cast<bool*>(static_cast<char*>(compData) + offset);
    });
    checkBox->setValueSetter([entity, compTypeIndex, offset = field.offset](bool newValue) mutable {
        void* compData = entity.getComponentData(compTypeIndex);
        if (compData) {
            *reinterpret_cast<bool*>(static_cast<char*>(compData) + offset) = newValue;
        }
    });
    box->addChild(checkBox);
}

void InspectorUI::buildStringUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex,
                                const Axiom::FieldInfo& field) {
    auto textInput = std::make_shared<Axiom::UITextInput>();
    textInput->setValueGetter([entity, compTypeIndex, offset = field.offset]() -> std::string {
        void* compData = entity.getComponentData(compTypeIndex);
        if (!compData) {
            return "";
        }
        return *reinterpret_cast<std::string*>(static_cast<char*>(compData) + offset);
    });
    textInput->setValueSetter([entity, compTypeIndex, offset = field.offset](const std::string& newValue) {
        void* compData = entity.getComponentData(compTypeIndex);
        if (compData) {
            *reinterpret_cast<std::string*>(static_cast<char*>(compData) + offset) = newValue;
        }
    });
    box->addChild(textInput);
}

void InspectorUI::buildVec2UI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field) {
    auto createAxis = [&](Axiom::Color color, auto getter, auto setter) {
        auto drag = std::make_shared<Axiom::UIScalarField<float>>();
        drag->setHorizontalAlignment(Axiom::UIAlignment::Fill);
        drag->setValueGetter(getter);
        drag->setValueSetter(setter);
        drag->setNormalColor(color);
        box->addChild(drag);
    };

    createAxis(
        Axiom::Color(0.9f, 0.1f, 0.1f),
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec2*>(static_cast<char*>(compData) + offset)->x();
        },
        [entity, compTypeIndex, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData)
                reinterpret_cast<Math::Vec2*>(static_cast<char*>(compData) + offset)->x() = v;
        });

    createAxis(
        Axiom::Color(0.1f, 0.9f, 0.1f),
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec2*>(static_cast<char*>(compData) + offset)->y();
        },
        [entity, compTypeIndex, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData)
                reinterpret_cast<Math::Vec2*>(static_cast<char*>(compData) + offset)->y() = v;
        });
}

void InspectorUI::buildVec3UI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field) {
    auto createAxis = [&](Axiom::Color color, auto getter, auto setter) {
        auto drag = std::make_shared<Axiom::UIScalarField<float>>();
        drag->setHorizontalAlignment(Axiom::UIAlignment::Fill);
        drag->setValueGetter(getter);
        drag->setValueSetter(setter);
        drag->setNormalColor(color);
        box->addChild(drag);
    };

    createAxis(
        Axiom::Color(0.9f, 0.1f, 0.1f),
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec3*>(static_cast<char*>(compData) + offset)->x();
        },
        [entity, compTypeIndex, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData)
                reinterpret_cast<Math::Vec3*>(static_cast<char*>(compData) + offset)->x() = v;
        });

    createAxis(
        Axiom::Color(0.1f, 0.9f, 0.1f),
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec3*>(static_cast<char*>(compData) + offset)->y();
        },
        [entity, compTypeIndex, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData)
                reinterpret_cast<Math::Vec3*>(static_cast<char*>(compData) + offset)->y() = v;
        });

    createAxis(
        Axiom::Color(0.1f, 0.1f, 0.9f),
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec3*>(static_cast<char*>(compData) + offset)->z();
        },
        [entity, compTypeIndex, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData)
                reinterpret_cast<Math::Vec3*>(static_cast<char*>(compData) + offset)->z() = v;
        });
}

void InspectorUI::buildVec4UI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field) {
    auto createAxis = [&](Axiom::Color color, auto getter, auto setter) {
        auto drag = std::make_shared<Axiom::UIScalarField<float>>();
        drag->setHorizontalAlignment(Axiom::UIAlignment::Fill);
        drag->setValueGetter(getter);
        drag->setValueSetter(setter);
        drag->setNormalColor(color);
        box->addChild(drag);
    };

    createAxis(
        Axiom::Color(0.9f, 0.1f, 0.1f),
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->x();
        },
        [entity, compTypeIndex, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData)
                reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->x() = v;
        });

    createAxis(
        Axiom::Color(0.1f, 0.9f, 0.1f),
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->y();
        },
        [entity, compTypeIndex, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData)
                reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->y() = v;
        });

    createAxis(
        Axiom::Color(0.1f, 0.1f, 0.9f),
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->z();
        },
        [entity, compTypeIndex, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData)
                reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->z() = v;
        });

    createAxis(
        Axiom::Color(0.9f, 0.9f, 0.1f),
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->w();
        },
        [entity, compTypeIndex, offset = field.offset](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData)
                reinterpret_cast<Math::Vec4*>(static_cast<char*>(compData) + offset)->w() = v;
        });
}

void InspectorUI::buildColorUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field,
                               const std::shared_ptr<Axiom::UITheme>& theme) {
    auto colorPreview = std::make_shared<Axiom::UIButton>("");
    colorPreview->setHorizontalAlignment(Axiom::UIAlignment::Start);
    colorPreview->setFixedSize({20.0f, 20.0f});

    void* initialData = entity.getComponentData(compTypeIndex);
    if (initialData) {
        Axiom::Color initialColor = *reinterpret_cast<Axiom::Color*>(static_cast<char*>(initialData) + field.offset);
        colorPreview->setNormalColor(initialColor);
        colorPreview->setHoverColor(initialColor);
        colorPreview->setActiveColor(initialColor);
    }

    box->addChild(colorPreview);

    auto updatePreview = [colorPreview, entity, compTypeIndex, offset = field.offset]() {
        void* compData = entity.getComponentData(compTypeIndex);
        if (compData) {
            Axiom::Color c = *reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset);
            colorPreview->setNormalColor(c);
            colorPreview->setHoverColor(c);
            colorPreview->setActiveColor(c);
        }
    };

    auto createChannelSlider = [&](auto getter, auto setter) {
        auto slider = std::make_shared<Axiom::UIScalarField<float>>();
        slider->setHorizontalAlignment(Axiom::UIAlignment::Fill);
        slider->setValueGetter(getter);
        slider->setValueSetter(setter);
        slider->setNormalColor(Axiom::Color(0.5f, 0.5f, 0.5f));
        slider->setLimits(0.0f, 1.0f);
        box->addChild(slider);
    };

    createChannelSlider(
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->r();
        },
        [entity, compTypeIndex, offset = field.offset, updatePreview](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData) {
                reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->r() = v;
                updatePreview();
            }
        });

    createChannelSlider(
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->g();
        },
        [entity, compTypeIndex, offset = field.offset, updatePreview](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData) {
                reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->g() = v;
                updatePreview();
            }
        });

    createChannelSlider(
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->b();
        },
        [entity, compTypeIndex, offset = field.offset, updatePreview](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData) {
                reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->b() = v;
                updatePreview();
            }
        });

    createChannelSlider(
        [entity, compTypeIndex, offset = field.offset]() -> float {
            void* compData = entity.getComponentData(compTypeIndex);
            if (!compData)
                return 0.0f;
            return reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->a();
        },
        [entity, compTypeIndex, offset = field.offset, updatePreview](float v) mutable {
            void* compData = entity.getComponentData(compTypeIndex);
            if (compData) {
                reinterpret_cast<Axiom::Color*>(static_cast<char*>(compData) + offset)->a() = v;
                updatePreview();
            }
        });
}

void InspectorUI::buildAssetHandleUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex,
                                     const Axiom::FieldInfo& field, const std::shared_ptr<Axiom::UITheme>& theme) {
    auto slotBox = std::make_shared<Axiom::UIHorizontalBox>();
    slotBox->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    slotBox->setPadding({4.0f, 4.0f, 4.0f, 4.0f});

    auto getAssetName = [entity, compTypeIndex, offset = field.offset]() -> std::string {
        void* compData = entity.getComponentData(compTypeIndex);
        if (!compData)
            return "None";
        Axiom::UUID id = *reinterpret_cast<Axiom::UUID*>(static_cast<char*>(compData) + offset);
        if (id.isValid())
            return Axiom::AssetManager::getMetadata(id).name;
        return "None";
    };

    auto assetButton = std::make_shared<Axiom::UIButton>(getAssetName());
    assetButton->setHorizontalAlignment(Axiom::UIAlignment::Fill);
    assetButton->setMargin({0.0f, 0.0f, 4.0f, 0.0f});

    auto clearButton = std::make_shared<Axiom::UIButton>("X");
    clearButton->setFixedSize({24.0f, 24.0f});
    clearButton->setNormalColor(theme->errorColor);
    clearButton->setOnClick([entity, compTypeIndex, offset = field.offset, assetButton, getAssetName]() {
        void* compData = entity.getComponentData(compTypeIndex);
        if (compData) {
            *reinterpret_cast<Axiom::UUID*>(static_cast<char*>(compData) + offset) = Axiom::UUID();
            assetButton->setText(getAssetName());
        }
    });

    slotBox->addChild(assetButton);
    slotBox->addChild(clearButton);
    box->addChild(slotBox);
}

void InspectorUI::buildEnumUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field) {
    auto dropdown = std::make_shared<Axiom::UIDropdown>(field.enumOptions);
    dropdown->setHorizontalAlignment(Axiom::UIAlignment::Fill);

    void* initialData = entity.getComponentData(compTypeIndex);
    if (initialData) {
        dropdown->setSelectedIndex(*reinterpret_cast<int*>(static_cast<char*>(initialData) + field.offset));
    }

    dropdown->setOnSelectionChanged([entity, compTypeIndex, offset = field.offset](int index, const std::string&) mutable {
        void* compData = entity.getComponentData(compTypeIndex);
        if (compData) {
            *reinterpret_cast<int*>(static_cast<char*>(compData) + offset) = index;
        }
    });

    box->addChild(dropdown);
}
