#pragma once
#include "../BasicTypes.h"
#include"../Platform/GraphicWindow.h"

namespace QuantumEngine::Rendering {
	using namespace Platform;

	class GraphicContext;
	class GPUAssetManager;
	class ShaderRegistery;

	class GPUDeviceManager {
	public:
		virtual bool Initialize() = 0;
		virtual ref<GraphicContext> CreateHybridContextForWindows(ref<GraphicWindow>& window) = 0;
		virtual ref<GraphicContext> CreateRayTracingContextForWindows(ref<GraphicWindow>& window) = 0;
		virtual ref<GPUAssetManager> CreateAssetManager() = 0;
		virtual ref<ShaderRegistery> CreateShaderRegistery() = 0;
	};
}