#pragma once
#include "../pch.h"
#include <vector>
#include <BasicTypes.h>

using namespace Microsoft::WRL;

namespace QuantumEngine {
	class Transform;
	struct Matrix4;

	namespace Rendering::DX12 {
		class DX12MeshController;
	}
}

namespace QuantumEngine::Rendering::DX12::RayTracing {
	struct EntityBLASDesc {
		ref<DX12MeshController> mesh;
		ref<Transform> transform;
	};

	class RTAccelarationStructure {
	public:
		bool Initialize(const ComPtr<ID3D12GraphicsCommandList7>& commandList, const std::vector<EntityBLASDesc>& entities);
		void UpdateTransforms(const ComPtr<ID3D12GraphicsCommandList7>& commandList, Matrix4& viewMatrix);
		inline ComPtr<ID3D12DescriptorHeap> GetDescriptor() { return m_tlasHeap; }
	private:
		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instanceDescs;
		std::vector<ref<Transform>> m_transforms;
		ComPtr<ID3D12Resource2> m_topLevelAccelerationStructure;
		ComPtr<ID3D12DescriptorHeap> m_tlasHeap;
		ComPtr<ID3D12Resource2> m_tlasUpload;
		ComPtr<ID3D12Resource2> m_rtScratchBuffer;
	};
}