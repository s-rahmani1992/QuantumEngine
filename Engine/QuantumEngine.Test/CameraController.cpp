#include "CameraController.h"
#include "Platform/CommonWin.h"
#include "Core/Transform.h"

CameraController::CameraController(ref<QuantumEngine::Camera>& camera)
	:m_camera(camera)
{
}

void CameraController::Update(Float deltaTime)
{
	if (GetKeyState(VK_UP) & 0x80 || GetKeyState('W') & 0x80) {
		m_camera->GetTransform()->MoveForward(1.0 * deltaTime);
	}

	if (GetKeyState(VK_DOWN) & 0x80 || GetKeyState('S') & 0x80) {
		m_camera->GetTransform()->MoveForward(-1.0 * deltaTime);
	}

	if (GetKeyState(VK_LEFT) & 0x80 || GetKeyState('A') & 0x80) {
		m_camera->GetTransform()->MoveRight(-1.0 * deltaTime);
	}

	if (GetKeyState(VK_RIGHT) & 0x80 || GetKeyState('D') & 0x80) {
		m_camera->GetTransform()->MoveRight(1.0 * deltaTime);
	}
}
