#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Mesh;
	class GameEntity;
}

namespace QuantumEngine::Rendering {
	class GPUAssetManager;

	class GraphicContext {
	public:
		virtual void Render() = 0;
		virtual void RegisterAssetManager(const ref<GPUAssetManager>& mesh) = 0;
		virtual void AddGameEntity(ref<GameEntity>& gameEntity) = 0;
		virtual bool PrepareRayTracingData() = 0;
	};
}