#include "EditorCamera.h"

EditorCamera::EditorCamera(const Math::Vec3& position, float pitch, float yaw)
    : position(position), pitch(pitch * Math::DEG_TO_RAD), yaw(yaw * Math::DEG_TO_RAD) {
    updateVectors();
    updateView();
}

void EditorCamera::onUpdate(float deltaTime) {
    Math::Vec2 currentMousePos = Axiom::Input::getMousePosition();
    Math::Vec2 mouseDelta = (currentMousePos - lastMousePos) * 0.005f;
    lastMousePos = currentMousePos;

    if (Axiom::Input::isMouseButtonPressed(Axiom::KeyCode::RightButton)) {
        yaw += mouseDelta.x();
        pitch += mouseDelta.y();

        pitch = Math::clamp(pitch, -89.0f * Math::DEG_TO_RAD, 89.0f * Math::DEG_TO_RAD);

        updateVectors();

        float speed = 1.0f * deltaTime;

        if (Axiom::Input::isKeyPressed(Axiom::KeyCode::W)) {
            position += forward * speed;
        }
        if (Axiom::Input::isKeyPressed(Axiom::KeyCode::S)) {
            position -= forward * speed;
        }
        if (Axiom::Input::isKeyPressed(Axiom::KeyCode::A)) {
            position -= right * speed;
        }
        if (Axiom::Input::isKeyPressed(Axiom::KeyCode::D)) {
            position += right * speed;
        }
        if (Axiom::Input::isKeyPressed(Axiom::KeyCode::E)) {
            position += Math::Vec3(0.0f, 1.0f, 0.0f) * speed;
        }
        if (Axiom::Input::isKeyPressed(Axiom::KeyCode::Q)) {
            position -= Math::Vec3(0.0f, 1.0f, 0.0f) * speed;
        }

        updateView();
    }
}

void EditorCamera::updateVectors() {
    Math::Vec3 front = Math::Vec3::zero();
    front.x() = Math::sin(yaw) * Math::cos(pitch);
    front.y() = Math::sin(pitch);
    front.z() = -Math::cos(yaw) * Math::cos(pitch);
    forward = Math::normalize(front);

    right = Math::normalize(Math::cross(forward, Math::Vec3(0.0f, 1.0f, 0.0f)));
    up = Math::normalize(Math::cross(right, forward));
}

void EditorCamera::updateView() {
    view = Math::Mat4::lookAt(position, position + forward, Math::Vec3(0.0f, 1.0f, 0.0f));
}
