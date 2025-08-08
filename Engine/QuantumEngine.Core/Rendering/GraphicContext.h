#pragma once
#include "../BasicTypes.h"
#include "Core/Matrix4.h"
#include "Core/Vector3.h"
#include "Core/Camera/Camera.h"

namespace QuantumEngine {
	class Mesh;
	class GameEntity;
	struct SceneLightData;
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
		virtual void RegisterLight(const SceneLightData& lights) = 0;
		void SetCamera(const ref<Camera>& camera) { 
			m_camera = camera; 
			m_camData.projectionMatrix = camera->ProjectionMatrix();
			m_camData.inverseProjectionMatrix = camera->InverseProjectionMatrix();
		}
	protected:
		struct CameraGPU {
		public:
			Matrix4 projectionMatrix;
			Matrix4 inverseProjectionMatrix;
			Matrix4 viewMatrix;
			Vector3 position;
		};
		ref<Camera> m_camera;
		CameraGPU m_camData;
	};
}