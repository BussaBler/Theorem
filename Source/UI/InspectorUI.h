#pragma once

#include <Axiom.h>
#include <functional>
#include <memory>

class InspectorUI {
  public:
    using OnValueChangedCallback = std::function<void()>;
    using OnRequestAssetCallback = std::function<void(Axiom::UUID*, std::shared_ptr<Axiom::UIButton>, const Axiom::FieldInfo&)>;

    static std::shared_ptr<Axiom::UIElement> createFieldUI(Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field,
                                                           const std::shared_ptr<Axiom::UITheme>& theme);

  private:
    static void buildFloatUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field);
    static void buildIntUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field);
    static void buildBoolUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field);
    static void buildStringUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field);
    static void buildVec2UI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field);
    static void buildVec3UI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field);
    static void buildVec4UI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field);
    static void buildColorUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field,
                             const std::shared_ptr<Axiom::UITheme>& theme);
    static void buildAssetHandleUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex,
                                   const Axiom::FieldInfo& field, const std::shared_ptr<Axiom::UITheme>& theme);
    static void buildEnumUI(std::shared_ptr<Axiom::UIHorizontalBox> box, Axiom::Entity entity, std::type_index compTypeIndex, const Axiom::FieldInfo& field);
};
