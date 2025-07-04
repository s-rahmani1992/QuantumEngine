#pragma 
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Mesh;
	class Texture2D;
}

namespace QuantumEngine::Rendering {
	class GPUAssetManager {
	public:
		virtual void UploadMeshToGPU(const ref<Mesh>& mesh) = 0;
		virtual void UploadTextureToGPU(const ref<Texture2D>& texture) = 0;
	};
}