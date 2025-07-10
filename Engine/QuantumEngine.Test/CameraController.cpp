#include "CameraController.h"
#include "Platform/CommonWin.h"
#include "Core/Transform.h"
#include <string>

CameraController::CameraController(ref<QuantumEngine::Camera>& camera)
	:m_camera(camera), m_moveSpeed(2.0f), m_rotateSpeed(10)
{
	POINT mousePos;
	GetCursorPos(&mousePos);
	xPos = mousePos.x;
	yPos = mousePos.y;
}

void CameraController::Update(Float deltaTime)
{
	// TODO Replace it with event-based input system
	if ((GetKeyState(VK_RBUTTON) & 0x80) == 0) {
		POINT mousePos;
		GetCursorPos(&mousePos);
		xPos = mousePos.x;
		yPos = mousePos.y;
		return;
	}

	POINT mousePos;
	GetCursorPos(&mousePos);
	Float angle_x = m_rotateSpeed * deltaTime * (mousePos.x - xPos);
	Float angle_y = m_rotateSpeed * deltaTime * (mousePos.y - yPos);
	m_camera->GetTransform()->RotateAround(m_camera->GetTransform()->Up(), -angle_x);
	m_camera->GetTransform()->RotateAround(m_camera->GetTransform()->Right(), -angle_y);
	xPos = mousePos.x;
	yPos = mousePos.y;

	if (GetKeyState(VK_UP) & 0x80 || GetKeyState('W') & 0x80) {
		m_camera->GetTransform()->MoveForward(m_moveSpeed * deltaTime);
	}

	if (GetKeyState(VK_DOWN) & 0x80 || GetKeyState('S') & 0x80) {
		m_camera->GetTransform()->MoveForward(-m_moveSpeed * deltaTime);
	}

	if (GetKeyState(VK_LEFT) & 0x80 || GetKeyState('A') & 0x80) {
		m_camera->GetTransform()->MoveRight(-m_moveSpeed * deltaTime);
	}

	if (GetKeyState(VK_RIGHT) & 0x80 || GetKeyState('D') & 0x80) {
		m_camera->GetTransform()->MoveRight(m_moveSpeed * deltaTime);
	}
}
