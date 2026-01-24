#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include <vector>
#include <string>
#include "HLSLRayTracingProgram.h"

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering {
	class Material;
}

namespace QuantumEngine::Rendering::DX12::RayTracing {
	class DX12RayTracingMaterial {
	public:
		DX12RayTracingMaterial(const ref<Material>& material, const ref<RayTracing::HLSLRayTracingProgram>& program);
		~DX12RayTracingMaterial();
		UInt32 GetNonGlobalResourceCounts();

		UInt32 LinkMaterialToDescriptorHeap(const ComPtr<ID3D12DescriptorHeap>& heap, UInt32 offset);
		UInt32 LinkUserMaterials(const ComPtr<ID3D12DescriptorHeap>& heap, UInt32 offset);
		UInt32 LinkInternalMaterials(const ComPtr<ID3D12DescriptorHeap>& heap, UInt32 offset);

		void SetDescriptorHandle(const std::string& fieldName, const D3D12_GPU_DESCRIPTOR_HANDLE& handle);
		void SetCBV(const std::string& name, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbv);
		void SetSRV(const std::string& name,const ComPtr<ID3D12Resource2>& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srv);
		
		void CopyVariableData(Byte* dst);
		void BindParameters(ComPtr<ID3D12GraphicsCommandList7>& commandList);
		void UpdateModifiedParameters();

		inline ref<HLSLRayTracingProgram> GetProgram() const { return m_program; }

		template<typename T> 
		void SetInternalConstantValue(std::string fieldName, T value);

	private:
		
		struct constantBufferData {
			UInt32 rootParamIndex;
			std::string fieldName;
			Byte* dataLocation;
			UInt32 locationOffset;
			UInt32 size;
		};

		struct HeapData {
			UInt32 rootParamIndex;
			std::string fieldName;
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
			UInt32 locationOffset;
		};

		ref<Material> m_material;
		ref<HLSLRayTracingProgram> m_program;
		ComPtr<ID3D12Device10> m_device;

		std::vector<constantBufferData> m_constantRegisterValues;
		std::vector<HeapData> m_heapValues;

		std::vector<Byte*> m_linkedLocations;

		Byte* m_internalData;
	};


	template<typename T>
	inline void DX12RayTracingMaterial::SetInternalConstantValue(std::string fieldName, T value)
	{
		// GetRootConstants() should return a container of root-constant groups
		auto& rootConstants = m_program->GetReflectionData()->GetRootConstants();

		for (auto& rt : rootConstants) {
			// find the variable in this root-constant group
			auto varIt = std::find_if(rt.rootConstants.begin(), rt.rootConstants.end(),
				[&fieldName](const RootConstantVariableData& vData) {
					return vData.name == fieldName;
				});

			if (varIt != rt.rootConstants.end()) {
				// find the constant buffer entry that matches this root-constant group's name
				auto cbIt = std::find_if(m_constantRegisterValues.begin(), m_constantRegisterValues.end(),
					[&rt](const constantBufferData& cb) {
						return rt.name == cb.fieldName;
					});

				// Validate iterator and dataLocation before writing
				if (cbIt != m_constantRegisterValues.end() && cbIt->dataLocation != nullptr) {
					std::memcpy(cbIt->dataLocation, &value, sizeof(T));
				}
				return;
			}
		}
	}
}
