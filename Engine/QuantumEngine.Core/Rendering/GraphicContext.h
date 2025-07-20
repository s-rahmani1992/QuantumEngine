#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Mesh;
	class GameEntity;
	class Camera;
}

namespace QuantumEngine::Rendering {
	class GPUAssetManager;
	class ShaderProgram;

	class GraphicContext {
	public:
		virtual void Render() = 0;
		virtual void RegisterAssetManager(const ref<GPUAssetManager>& mesh) = 0;
		virtual void AddGameEntity(ref<GameEntity>& gameEntity) = 0;
		virtual bool PrepareRayTracingData(const ref<ShaderProgram>& rtProgram) = 0;
		void SetCamera(const ref<Camera>& camera) { m_camera = camera; }
	protected:
		ref<Camera> m_camera;
	};
}