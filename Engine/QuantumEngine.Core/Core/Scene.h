#pragma once
#include "../BasicTypes.h"
#include "GameEntity.h"
#include "Camera/Camera.h"
#include "Light/Lights.h"
#include "Texture2D.h"
#include "../Rendering/ShaderProgram.h"
#include <vector>

namespace QuantumEngine {
	class Scene {
	public:
		ref<Camera> mainCamera;
		SceneLightData lightData;
		std::vector<ref<GameEntity>> entities;
		ref<Rendering::ShaderProgram> rtGlobalProgram;
	};
}