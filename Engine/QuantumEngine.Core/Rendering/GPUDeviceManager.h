#pragma once

namespace QuantumEngine{
	namespace Rendering {
		class GPUDeviceManager {
		public:
			virtual bool Initialize() = 0;
		};
	}
}