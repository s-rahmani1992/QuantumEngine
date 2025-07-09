#pragma once
#include "Core/Camera/Camera.h"

class CameraController
{
public:
	CameraController(ref<QuantumEngine::Camera>& camera);
	void Update(Float deltaTime);
private:
	ref<QuantumEngine::Camera> m_camera;
};

