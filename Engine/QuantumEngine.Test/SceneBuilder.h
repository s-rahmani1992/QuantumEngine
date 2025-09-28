#pragma once
#include <BasicTypes.h>
#include <string>

namespace QuantumEngine {
	class Scene;

	namespace Rendering {
		class GPUDeviceManager;
	}

	namespace Platform {
		class GraphicWindow;
	}
}

namespace Render = QuantumEngine::Rendering;
namespace Platform = QuantumEngine::Platform;

class SceneBuilder
{
public:
	static bool BuildLightScene(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& error);
};

