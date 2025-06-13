#pragma 
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Mesh;
}

namespace QuantumEngine::Rendering {
	class GPUAssetManager {
	public:
		virtual void UploadMeshToGPU(const ref<Mesh>& mesh) = 0;
	};
}