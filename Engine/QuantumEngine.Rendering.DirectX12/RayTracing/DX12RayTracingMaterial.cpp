#include "pch.h"
#include "DX12RayTracingMaterial.h"
#include "Rendering/Material.h"
#include "HLSLRayTracingProgram.h"
#include <DX12Utilities.h>
#include "Core/Texture2D.h"
#include "DX12Texture2DController.h"

QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::DX12RayTracingMaterial(const ref<Material>& material, const ref<RayTracing::HLSLRayTracingProgram>& program)
	: m_material(material), m_program(program)
{
	program->GetRootSignature()->GetDevice(IID_PPV_ARGS(&m_device));
	auto reflectionData = program->GetReflectionData();
	auto& rootConstantList = reflectionData->GetRootConstants();

	auto textureFields = material->GetTextureFields();
	auto& resourceVariableList = reflectionData->GetResourceVariables();
	m_heapValues = std::vector<HeapData>(resourceVariableList.size());

	UInt32 offset = 0;
	UInt32 fieldIndex = 0;
	UInt32 internalSize = 0;

	for(UInt32 i = 0; i < reflectionData->GetRootParameterCount(); i++) {
		auto cbData = reflectionData->GetRootConstantByRootIndex(i);

		if (cbData != nullptr) {
			m_constantRegisterValues.push_back(constantBufferData{
			.rootParamIndex = i,
			.fieldName = cbData->name,
			.dataLocation = material->GetValueLocation(cbData->rootConstants[0].name) - cbData->rootConstants[0].variableDesc.StartOffset / 4,
			.locationOffset = offset,
			.size = cbData->registerData.Num32BitValues,
				});

			offset += cbData->registerData.Num32BitValues * 4;

			if (cbData->name[0] == '_')
				internalSize += (cbData->registerData.Num32BitValues * 4);

			continue;
		}

		auto rcData = reflectionData->GetResourceVariableByRootIndex(i);

		auto t = textureFields->find(rcData->name);

		if(t != textureFields->end()) {
			(*t).second.fieldIndex = fieldIndex;
		}

		m_heapValues[fieldIndex] = HeapData{
			.rootParamIndex = rcData->rootParameterIndex,
			.fieldName = rcData->name,
			.locationOffset = offset,
		};

		offset += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
		fieldIndex++;
	}

	m_internalData = new Byte[internalSize];
	UInt32 internalOffset = 0;

	for (auto& constRegister : m_constantRegisterValues) {
		if (constRegister.fieldName[0] == '_') {
			constRegister.dataLocation = m_internalData + internalOffset;
			internalOffset += (constRegister.size * 4);
		}
	}
}

QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::~DX12RayTracingMaterial()
{
	delete[] m_internalData;
}

UInt32 QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::GetNonGlobalResourceCounts()
{
	UInt32 count = 0;
	auto& resourceVariables = std::dynamic_pointer_cast<HLSLRayTracingProgram>(m_material->GetProgram())->GetReflectionData()->GetResourceVariables();
	
	for(auto& rc : resourceVariables) {
		if(rc.name == HLSL_RT_TLAS_SCENE_NAME ||
			rc.name == HLSL_RT_OUTPUT_TEXTURE_NAME ||
			rc.name == HLSL_CAMERA_DATA_NAME ||
			rc.name == HLSL_LIGHT_DATA_NAME) 
		{
			continue;
		}
		count++;
	}

	return count;
}

UInt32 QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::LinkMaterialToDescriptorHeap(const ComPtr<ID3D12DescriptorHeap>& heap, UInt32 offset)
{
	auto cpuHandleStart = heap->GetCPUDescriptorHandleForHeapStart();
	auto gpuHandleStart = heap->GetGPUDescriptorHandleForHeapStart();
	auto incSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto& resourceVariables = std::dynamic_pointer_cast<HLSLRayTracingProgram>(m_material->GetProgram())->GetReflectionData()->GetResourceVariables();
	
	cpuHandleStart.ptr += offset * incSize;
	gpuHandleStart.ptr += offset * incSize;
	UInt32 boundResourceCount = 0;

	for(auto& heapData : m_heapValues) {
		if(heapData.fieldName == HLSL_RT_TLAS_SCENE_NAME ||
			heapData.fieldName == HLSL_RT_OUTPUT_TEXTURE_NAME ||
			heapData.fieldName == HLSL_CAMERA_DATA_NAME ||
			heapData.fieldName == HLSL_LIGHT_DATA_NAME) 
		{
			continue;
		}

		heapData.cpuHandle = cpuHandleStart;
		heapData.gpuHandle = gpuHandleStart;
		cpuHandleStart.ptr += incSize;
		gpuHandleStart.ptr += incSize;
		boundResourceCount++;
	}

	return boundResourceCount;
}

void QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::SetDescriptorHandle(const std::string& fieldName, const D3D12_GPU_DESCRIPTOR_HANDLE& handle)
{
	auto it = std::find_if(
		m_heapValues.begin(),
		m_heapValues.end(),
		[&fieldName, this](const HeapData& binding) {
			auto resVar = m_program->GetReflectionData()->GetResourceVariableByRootIndex(binding.rootParamIndex);
			return resVar != nullptr && resVar->name == fieldName;
		});

	if(it != m_heapValues.end())
		(*it).gpuHandle = handle;
}

UInt32 QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::LinkUserMaterials(const ComPtr<ID3D12DescriptorHeap>& heap, UInt32 offset)
{
	auto cpuHandleStart = heap->GetCPUDescriptorHandleForHeapStart();
	auto gpuHandleStart = heap->GetGPUDescriptorHandleForHeapStart();
	auto incSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandleStart.ptr += offset * incSize;
	gpuHandleStart.ptr += offset * incSize;
	UInt32 boundResourceCount = 0;
	auto textureFields = m_material->GetTextureFields();

	for (auto& heapData : m_heapValues) {
		if (heapData.fieldName[0] == '_')
			continue;

		auto t = textureFields->find(heapData.fieldName);
		auto gpuTexture = std::dynamic_pointer_cast<DX12Texture2DController>((*t).second.texture->GetGPUHandle());
		
		m_device->CopyDescriptorsSimple(1, cpuHandleStart, gpuTexture->GetShaderView()->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		heapData.cpuHandle = cpuHandleStart;
		heapData.gpuHandle = gpuHandleStart;
		cpuHandleStart.ptr += incSize;
		gpuHandleStart.ptr += incSize;
		boundResourceCount++;
	}

	return boundResourceCount;
}

UInt32 QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::LinkInternalMaterials(const ComPtr<ID3D12DescriptorHeap>& heap, UInt32 offset)
{
	auto cpuHandleStart = heap->GetCPUDescriptorHandleForHeapStart();
	auto gpuHandleStart = heap->GetGPUDescriptorHandleForHeapStart();
	auto incSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandleStart.ptr += offset * incSize;
	gpuHandleStart.ptr += offset * incSize;
	UInt32 boundResourceCount = 0;

	for (auto& heapData : m_heapValues) {
		if (heapData.fieldName[0] != '_' ||
			heapData.fieldName == HLSL_RT_TLAS_SCENE_NAME ||
			heapData.fieldName == HLSL_RT_OUTPUT_TEXTURE_NAME ||
			heapData.fieldName == HLSL_CAMERA_DATA_NAME ||
			heapData.fieldName == HLSL_LIGHT_DATA_NAME)
		{
			continue;
		}

		heapData.cpuHandle = cpuHandleStart;
		heapData.gpuHandle = gpuHandleStart;
		cpuHandleStart.ptr += incSize;
		gpuHandleStart.ptr += incSize;
		boundResourceCount++;
	}

	return boundResourceCount;
}

void QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::SetCBV(const std::string& name, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbv)
{
	auto it = std::find_if(m_heapValues.begin(), m_heapValues.end(), [name](const HeapData& h) {
		return h.fieldName == name;
		});

	if(it != m_heapValues.end())
		m_device->CreateConstantBufferView(&cbv, (*it).cpuHandle);
}

void QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::SetSRV(const std::string& name, const ComPtr<ID3D12Resource2>& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srv)
{
	auto it = std::find_if(m_heapValues.begin(), m_heapValues.end(), [name](const HeapData& h) {
		return h.fieldName == name;
		});

	if (it != m_heapValues.end())
		m_device->CreateShaderResourceView(resource.Get(), &srv, (*it).cpuHandle);
}

void QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::CopyVariableData(Byte* dst)
{
	auto reflectionData = m_program->GetReflectionData();
	auto& rootConstantList = reflectionData->GetRootConstants();
	auto& resourceVariableList = reflectionData->GetResourceVariables();

	UInt32 offset = 0;
	UInt32 fieldIndex = 0;
	UInt32 internalSize = 0;
	Byte* data = dst;

	for (UInt32 i = 0; i < reflectionData->GetRootParameterCount(); i++) {
		auto cbData = std::find_if(m_constantRegisterValues.begin(), m_constantRegisterValues.end(), [i](const constantBufferData& cb) {
			return cb.rootParamIndex == i;
			});

		if (cbData != m_constantRegisterValues.end()) {
			std::memcpy(data, (*cbData).dataLocation, 4 * (*cbData).size);
			data += 4 * (*cbData).size;
			continue;
		}

		auto rcData = std::find_if(m_heapValues.begin(), m_heapValues.end(), [i](const HeapData& hpData) {
			return hpData.rootParamIndex == i;
			});

		D3D12_GPU_DESCRIPTOR_HANDLE* gpuPtr = (D3D12_GPU_DESCRIPTOR_HANDLE*)data;

		*gpuPtr = (*rcData).gpuHandle;
		data += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
	}
}

void QuantumEngine::Rendering::DX12::RayTracing::DX12RayTracingMaterial::BindParameters(ComPtr<ID3D12GraphicsCommandList7>& commandList)
{
	for (auto& rField : m_constantRegisterValues)
		commandList->SetComputeRoot32BitConstants(rField.rootParamIndex, rField.size, rField.dataLocation, 0);

	for (auto& heapField : m_heapValues) {
		commandList->SetComputeRootDescriptorTable(heapField.rootParamIndex, heapField.gpuHandle);
	}
}
