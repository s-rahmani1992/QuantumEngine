#pragma once
#include "../BasicTypes.h"
#include"../Platform/GraphicWindow.h"

namespace QuantumEngine {
	class Mesh;
}

namespace QuantumEngine::Rendering {
	using namespace Platform;

	class GraphicContext;
	class GPUAssetManager;
	class Shader;
	class ShaderProgram;

	class GPUDeviceManager {
	public:
		virtual bool Initialize() = 0;
		virtual ref<GraphicContext> CreateContextForWindows(ref<GraphicWindow>& window) = 0;
		virtual ref<GPUAssetManager> CreateAssetManager() = 0;
		virtual ref<ShaderProgram> CreateShaderProgram(const std::initializer_list<ref<Shader>>& shaders) = 0;
	};
}