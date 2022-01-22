#include "Camera.h"
#include "Window.h"
#include <algorithm>

void Camera::UpdateCamera(float dt) {
	Matrix4 rotation = Matrix4::Rotation(yaw, Vector3(0, 1, 0));
	Vector3 forward = rotation * Vector3(0, 0, -1);
	Vector3 right = rotation * Vector3(1, 0, 0);

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_F)) {
		freeLook = true;
	}

	if (freeLook == false) {
		if (pitch > -10) {
			pitch -= 2.0f * dt;
		}
		else {
			yaw -= 8 * dt;
		}
	}
	else {
		pitch -= (Window::GetMouse()->GetRelativePosition().y);
		yaw -= (Window::GetMouse()->GetRelativePosition().x);
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_W)) {
			position += forward * (speed * dt);
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_S)) {
			position -= forward * (speed * dt);
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_A)) {
			position -= right * (speed * dt);
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_D)) {
			position += right * (speed * dt);
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_SHIFT)) {
			position.y -= (speed * dt);
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_SPACE)) {
			position.y += (speed * dt);
		}
	}
	pitch = std::min(pitch, 90.0f);
	pitch = std::max(pitch, -90.0f);
	if (yaw < 0) {
		yaw += 360.0f;
	}
	if (yaw > 360.0f) {
		yaw -= 360.0f;
	}
}

Matrix4 Camera::BuildViewMatrix() {
	return Matrix4::Rotation(-pitch, Vector3(1, 0, 0)) * Matrix4::Rotation(-yaw, Vector3(0, 1, 0)) * Matrix4::Translation(-position);
}