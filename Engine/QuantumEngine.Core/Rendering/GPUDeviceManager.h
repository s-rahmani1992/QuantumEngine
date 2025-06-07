#pragma once
#include "../BasicTypes.h"
#include"../Platform/GraphicWindow.h"

namespace QuantumEngine::Rendering {
	using namespace Platform;

	class GraphicContext;

	class GPUDeviceManager {
	public:
		virtual bool Initialize() = 0;
		virtual ref<GraphicContext> CreateContextForWindows(ref<GraphicWindow>& window) = 0;
	};
}