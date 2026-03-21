#include "EntityPositionController.h"
#include "Platform/CommonWin.h"

EntityPositionController::EntityPositionController(ref<QuantumEngine::Transform>& transform, float speed)
	:m_transform(transform), m_speed(speed)
{
}

void EntityPositionController::Update(Float deltaTime)
{
	if (GetKeyState('U') & 0x80) {
		m_transform->Translate((deltaTime * m_speed) * Vector3(0.0f, 0.0f, -1.0f));
	}

	if (GetKeyState('J') & 0x80) {
		m_transform->Translate((deltaTime * m_speed) * Vector3(0.0f, 0.0f, 1.0f));
	}

	if (GetKeyState('H') & 0x80) {
		m_transform->Translate((deltaTime * m_speed) * Vector3(1.0f, 0.0f, 0.0f));
	}

	if (GetKeyState('K') & 0x80) {
		m_transform->Translate((deltaTime * m_speed) * Vector3(-1.0f, 0.0f, 0.0f));
	}
}
