#pragma once
#include "Axiom.h"

class EditorCamera : public Axiom::Camera {
  public:
    EditorCamera(const Math::Vec3& position = Math::Vec3::zero(), float pitch = 0.0f, float yaw = 0.0f);
    ~EditorCamera() override = default;

    void onUpdate(float deltaTime);

    inline const Math::Mat4& getView() const { return view; }
    inline const Math::Vec3& getPosition() const { return position; }
    inline const Math::Vec3& getForward() const { return forward; }
    inline const Math::Vec3& getUp() const { return up; }
    inline const Math::Vec3& getRight() const { return right; }

  private:
    void updateVectors();
    void updateView();

  private:
    Math::Mat4 view = Math::Mat4::identity();
    Math::Vec3 position = Math::Vec3::zero();
    Math::Vec3 forward = Math::Vec3(0.0f, 0.0f, -1.0f);
    Math::Vec3 up = Math::Vec3(0.0f, 1.0f, 0.0f);
    Math::Vec3 right = Math::Vec3(1.0f, 0.0f, 0.0f);
    float pitch = 0.0f;
    float yaw = 0.0f;
    Math::Vec2 lastMousePos = Math::Vec2::zero();
};