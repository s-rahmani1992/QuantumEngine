#include "pch.h"
#include "Dx12RayTracingPipeline.h"
#include "RayTracing/RTAccelarationStructure.h"
#include "DX12GraphicContext.h"
#include "Core/GameEntity.h"
#include "HLSLMaterial.h"
#include "Core/Mesh.h"
#include "DX12Utilities.h"
#include <set>
#include "HLSLShaderProgram.h"
#include "HLSLShader.h"
#include "DX12MeshController.h"

bool QuantumEngine::Rendering::DX12::DX12RayTracingPipeline::Initialize(const ComPtr<ID3D12GraphicsCommandList7>& commandList, const std::vector<DX12EntityGPUData>& entities, UInt32 width, UInt32 height, const ref<HLSLMaterial>& rtMaterial)
{
	commandList->GetDevice(IID_PPV_ARGS(&m_device));

	// Initialize TLAS
	m_TLASController = std::make_shared<RayTracing::RTAccelarationStructure>();
	std::vector<RayTracing::EntityBLASDesc> rtEntities;

	UInt32 rtHeapsize = 0;

	for (auto& entity : entities) {
		auto rtMaterial = std::dynamic_pointer_cast<HLSLMaterial>(entity.gameEntity->GetRTMaterial());

		if (rtMaterial == nullptr)
			continue;

		rtHeapsize += rtMaterial->GetBoundResourceCount();
		rtEntities.push_back(RayTracing::EntityBLASDesc{
			.mesh = std::dynamic_pointer_cast<DX12MeshController>(entity.gameEntity->GetMesh()->GetGPUHandle()),
			.transform = entity.gameEntity->GetTransform(),
			});

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		};

		ComPtr<ID3D12DescriptorHeap> transformHeap;
		if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&transformHeap)))) {
			return false;
		}

		D3D12_CONSTANT_BUFFER_VIEW_DESC transformViewDesc;
		transformViewDesc.BufferLocation = entity.transformResource->GetGPUVirtualAddress();
		transformViewDesc.SizeInBytes = CONSTANT_BUFFER_ALIGHT(sizeof(TransformGPU));

		m_device->CreateConstantBufferView(&transformViewDesc, transformHeap->GetCPUDescriptorHandleForHeapStart());

		auto meshController = std::dynamic_pointer_cast<DX12MeshController>(entity.gameEntity->GetMesh()->GetGPUHandle());

		rtMaterial->SetDescriptorHeap(HLSL_OBJECT_TRANSFORM_DATA_NAME, transformHeap);
		rtMaterial->SetDescriptorHeap("g_indices", meshController->GetIndexHeap());
		rtMaterial->SetDescriptorHeap("g_vertices",meshController->GetVertexHeap());
	}

	if (m_TLASController->Initialize(commandList, rtEntities) == false)
		return false;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};

	// Ray Tracing Output Buffer
	// Create the output resource. The dimensions should match the swap-chain
	D3D12_RESOURCE_DESC outputDesc = {};
	outputDesc.DepthOrArraySize = 1;
	outputDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	outputDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB formats can't be used with UAVs. We will convert to sRGB ourselves in the shader
	outputDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	outputDesc.Width = width;
	outputDesc.Height = height;
	outputDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	outputDesc.MipLevels = 1;
	outputDesc.SampleDesc.Count = 1;

	if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
		&outputDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_outputBuffer)
	))) {
		return false;
	}

	if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_outputHeap)))) {
		return false;
	}

	D3D12_RESOURCE_BARRIER outputEndRTBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_outputBuffer.Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_COMMON,
		.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		},
	};
	commandList->ResourceBarrier(1, &outputEndRTBarrier);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavOutputDesc = {};
	uavOutputDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	m_device->CreateUnorderedAccessView(m_outputBuffer.Get(), nullptr,
		&uavOutputDesc, m_outputHeap->GetCPUDescriptorHandleForHeapStart()
	);

	// Ray Tracing Pipeline
	this->m_rtMaterial = rtMaterial;
	m_rtProgram = std::dynamic_pointer_cast<HLSLShaderProgram>(m_rtMaterial->GetProgram());
	std::set<ref< HLSLShader>> shaders;

	m_rtMaterial->SetDescriptorHeap("gRtScene", m_TLASController->GetDescriptor());
	m_rtMaterial->SetDescriptorHeap("gOutput", m_outputHeap);
	rtHeapsize += m_rtMaterial->GetBoundResourceCount();

	for (auto& entity : entities) {
		auto rtMaterial = std::dynamic_pointer_cast<HLSLMaterial>(entity.gameEntity->GetRTMaterial());

		if (rtMaterial == nullptr)
			continue;

		rtMaterial->SetDescriptorHeap("gRtScene", m_TLASController->GetDescriptor());
	}

	ref<HLSLShader> libShader = std::dynamic_pointer_cast<HLSLShader>(m_rtProgram->GetShader(LIB_SHADER));
	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	subobjects.reserve(6 + 20 * entities.size());
	// Create DXIL Sub Object

	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
		.pDesc = libShader->GetDXIL(),
		});

	// Global Root Signature 
	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
		.pDesc = m_rtProgram->GetReflectionData()->rootSignature.GetAddressOf(),
		});

	D3D12_STATE_SUBOBJECT* rootPtr = subobjects.data() + subobjects.size() - 1;

	// Shader Config
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig;
	shaderConfig.MaxPayloadSizeInBytes = 6 * sizeof(Float);
	shaderConfig.MaxAttributeSizeInBytes = 2 * sizeof(Float);

	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,
		.pDesc = &shaderConfig,
		});

	D3D12_STATE_SUBOBJECT* shaderConfigPtr = subobjects.data() + subobjects.size() - 1;

	// Pipeline Config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
	pipelineConfig.MaxTraceRecursionDepth = 3;

	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,
		.pDesc = &pipelineConfig,
		});

	D3D12_STATE_SUBOBJECT* pipelineConfigPtr = subobjects.data() + subobjects.size() - 1;

	LPCWSTR g = GLOBAL_HIT_GROUP_NAME;
	LPCWSTR missGen[2] = { libShader->GetRayGenExport().c_str(), libShader->GetMissExport().c_str() };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION rootExportDesc;
	rootExportDesc.pSubobjectToAssociate = rootPtr;
	rootExportDesc.NumExports = 2;
	rootExportDesc.pExports = missGen;
	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
		.pDesc = &rootExportDesc,
		});

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigExportDesc;
	shaderConfigExportDesc.pSubobjectToAssociate = shaderConfigPtr;
	shaderConfigExportDesc.NumExports = 2;
	shaderConfigExportDesc.pExports = missGen;
	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
		.pDesc = &shaderConfigExportDesc,
		});

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION pipelineConfigExportDesc;
	pipelineConfigExportDesc.pSubobjectToAssociate = pipelineConfigPtr;
	pipelineConfigExportDesc.NumExports = 2;
	pipelineConfigExportDesc.pExports = missGen;
	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
		.pDesc = &pipelineConfigExportDesc,
		});

	//Create Local RootSignature state objects for each entity
	int entityIndex = 0;

	struct LocalRTData {
		std::wstring hitExportName;
		std::wstring missExportName;
		D3D12_HIT_GROUP_DESC hitDesc;
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION hitExportDesc;
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigExportDesc;
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION pipelineConfigExportDesc;
		LPCWSTR hitExportPtr[2];
	};

	std::vector<LocalRTData> entityRTDatas;
	entityRTDatas.reserve(entities.size() + 1); // Prevent vector from reallocation. So the pointer to entry is always valid

	for (auto& entity : entities) {
		auto rtMaterial = std::dynamic_pointer_cast<HLSLMaterial>(entity.gameEntity->GetRTMaterial());

		if (rtMaterial == nullptr)
			continue;

		entityRTDatas.push_back(LocalRTData{});
		LocalRTData& rtRef = entityRTDatas.back();
		ref<HLSLShader> localLibShader = std::dynamic_pointer_cast<HLSLShader>(rtMaterial->GetProgram()->GetShader(LIB_SHADER));

		if (shaders.emplace(localLibShader).second) {
			subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
			.pDesc = localLibShader->GetDXIL(),
				});
		}

		// Create HitGroup for local material
		rtRef.hitExportName = L"Entity_" + std::to_wstring(entityIndex);
		rtRef.missExportName = localLibShader->GetMissExport();
		rtRef.hitExportPtr[0] = rtRef.hitExportName.c_str();
		rtRef.hitDesc = localLibShader->GetHitGroup(rtRef.hitExportName);
		UInt32 numExport = 1;

		if (rtRef.missExportName.empty() == false) {
			rtRef.hitExportPtr[1] = rtRef.missExportName.c_str();
			numExport = 2;
		}

		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
			.pDesc = &rtRef.hitDesc,
			});

		subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,
		.pDesc = std::dynamic_pointer_cast<HLSLShaderProgram>(rtMaterial->GetProgram())->GetReflectionData()->rootSignature.GetAddressOf(),
			});

		D3D12_STATE_SUBOBJECT* localRootPtr = subobjects.data() + subobjects.size() - 1;

		rtRef.hitExportDesc.pSubobjectToAssociate = localRootPtr;
		rtRef.hitExportDesc.NumExports = numExport;
		rtRef.hitExportDesc.pExports = rtRef.hitExportPtr;
		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
			.pDesc = &rtRef.hitExportDesc,
			});

		rtRef.shaderConfigExportDesc.pSubobjectToAssociate = shaderConfigPtr;
		rtRef.shaderConfigExportDesc.NumExports = numExport;
		rtRef.shaderConfigExportDesc.pExports = rtRef.hitExportPtr;
		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
			.pDesc = &rtRef.shaderConfigExportDesc,
			});

		rtRef.pipelineConfigExportDesc.pSubobjectToAssociate = pipelineConfigPtr;
		rtRef.pipelineConfigExportDesc.NumExports = numExport;
		rtRef.pipelineConfigExportDesc.pExports = rtRef.hitExportPtr;
		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
			.pDesc = &rtRef.pipelineConfigExportDesc,
			});

		entityIndex++;
	}

	// Create the State Object
	D3D12_STATE_OBJECT_DESC stateObjectDesc;
	stateObjectDesc.NumSubobjects = subobjects.size();
	stateObjectDesc.pSubobjects = subobjects.data();
	stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	if (FAILED(m_device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&m_rtStateObject)))) {
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC rtHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = rtHeapsize,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	};

	m_device->CreateDescriptorHeap(&rtHeapDesc, IID_PPV_ARGS(&m_rtHeap));
	UInt32 offset = 0;

	m_rtMaterial->BindDescriptor(m_rtHeap, offset);
	offset += m_rtMaterial->GetBoundResourceCount();

	for (auto& entity : entities) {
		auto rtMaterial = std::dynamic_pointer_cast<HLSLMaterial>(entity.gameEntity->GetRTMaterial());

		if (rtMaterial == nullptr)
			continue;

		rtMaterial->BindDescriptor(m_rtHeap, offset);
		offset += rtMaterial->GetBoundResourceCount();
	}

	// Create ShaderTable
	UInt32 rayGenResordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + m_rtProgram->GetReflectionData()->totalVariableSize;
	rayGenResordSize = SBT_SHADER_RECORD_ALIGHT(rayGenResordSize);
	UInt32 rayGenSectionSize = SBT_SECTION_ALIGHT(rayGenResordSize);

	UInt32 rtMissEntityCount = 0;
	UInt32 missResordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + m_rtProgram->GetReflectionData()->totalVariableSize;
	missResordSize = SBT_SHADER_RECORD_ALIGHT(missResordSize);

	UInt32 hitRecordSize = 0;
	UInt32 rtEntityCount = 0;

	for (auto& entity : entities) {
		auto rtMaterial = std::dynamic_pointer_cast<HLSLMaterial>(entity.gameEntity->GetRTMaterial());

		if (rtMaterial == nullptr)
			continue;

		auto localRTProgram = std::dynamic_pointer_cast<HLSLShaderProgram>(rtMaterial->GetProgram());
		UInt32 hitGroupSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + localRTProgram->GetReflectionData()->totalVariableSize;
		hitGroupSize = SBT_SHADER_RECORD_ALIGHT(hitGroupSize);

		if (hitGroupSize > hitRecordSize)
			hitRecordSize = hitGroupSize;

		ref<HLSLShader> localLibShader = std::dynamic_pointer_cast<HLSLShader>(rtMaterial->GetProgram()->GetShader(LIB_SHADER));

		if (localLibShader->GetMissExport().empty() == false) {
			if (missResordSize < hitGroupSize)
				missResordSize = hitGroupSize;

			rtMissEntityCount++;

			rtMaterial->SetUInt32("missIndex", rtMissEntityCount);
		}

		rtEntityCount++;
	}
	UInt32 missSectionSize = SBT_SECTION_ALIGHT((rtMissEntityCount + 1) * missResordSize);

	UInt32 hitSectionSize = rtEntityCount * hitRecordSize;
	hitSectionSize = SBT_SECTION_ALIGHT(hitSectionSize);

	ComPtr<ID3D12StateObjectProperties> rtProperties;
	if (FAILED(m_rtStateObject->QueryInterface(IID_PPV_ARGS(&rtProperties))))
		return false;

	D3D12_RESOURCE_DESC shaderTableBufferDesc
	{
	shaderTableBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	shaderTableBufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
	shaderTableBufferDesc.Width = rayGenSectionSize + missSectionSize + hitSectionSize,
	shaderTableBufferDesc.Height = 1,
	shaderTableBufferDesc.DepthOrArraySize = 1,
	shaderTableBufferDesc.MipLevels = 1,
	shaderTableBufferDesc.Format = DXGI_FORMAT_UNKNOWN,
	shaderTableBufferDesc.SampleDesc.Count = 1,
	shaderTableBufferDesc.SampleDesc.Quality = 0,
	shaderTableBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	shaderTableBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE,
	};

	if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonUploadHeapProps, D3D12_HEAP_FLAG_NONE,
		&shaderTableBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_shaderTableBuffer))))
		return false;

	ComPtr<ID3D12DescriptorHeap> shaderTableHeap;
	if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&shaderTableHeap)))) {
		return false;
	}

	Byte* pData;
	m_shaderTableBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData));

	// Entry 0 - ray-gen program ID and descriptor data
	std::memcpy(pData, rtProperties->GetShaderIdentifier(libShader->GetRayGenExport().c_str()),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	m_rtMaterial->CopyVariableData(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 1 - primary ray miss
	pData += rayGenSectionSize;
	Byte* pMissData = pData;
	std::memcpy(pData, rtProperties->GetShaderIdentifier(libShader->GetMissExport().c_str()),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	m_rtMaterial->CopyVariableData(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	pMissData += missResordSize;

	for (auto& entity : entities) {
		auto rtMaterial = std::dynamic_pointer_cast<HLSLMaterial>(entity.gameEntity->GetRTMaterial());

		if (rtMaterial == nullptr)
			continue;

		ref<HLSLShader> localLibShader = std::dynamic_pointer_cast<HLSLShader>(rtMaterial->GetProgram()->GetShader(LIB_SHADER));

		if (localLibShader->GetMissExport().empty())
			continue;

		std::memcpy(pMissData, rtProperties->GetShaderIdentifier(localLibShader->GetMissExport().c_str()),
			D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		rtMaterial->CopyVariableData(pMissData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		pMissData += missResordSize;
	}

	// Entry 2 - hit program
	pData += missSectionSize;
	entityIndex = 0;

	for (auto& entity : entities) {
		auto rtMaterial = std::dynamic_pointer_cast<HLSLMaterial>(entity.gameEntity->GetRTMaterial());

		if (rtMaterial == nullptr)
			continue;

		std::memcpy(pData, rtProperties->GetShaderIdentifier((L"Entity_" + std::to_wstring(entityIndex)).c_str()),
			D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		rtMaterial->CopyVariableData(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		pData += hitRecordSize;
		entityIndex++;
	}

	// Unmap
	m_shaderTableBuffer->Unmap(0, nullptr);

	m_raytraceDesc.Width = width;
	m_raytraceDesc.Height = height;
	m_raytraceDesc.Depth = 1;

	D3D12_GPU_VIRTUAL_ADDRESS startAddress = m_shaderTableBuffer->GetGPUVirtualAddress();
	// RayGen is the first entry in the shader-table
	m_raytraceDesc.RayGenerationShaderRecord.StartAddress = startAddress;
	m_raytraceDesc.RayGenerationShaderRecord.SizeInBytes = rayGenResordSize;

	// Miss is the second entry in the shader-table
	startAddress += rayGenSectionSize;
	m_raytraceDesc.MissShaderTable.StartAddress = startAddress;
	m_raytraceDesc.MissShaderTable.StrideInBytes = missResordSize;
	m_raytraceDesc.MissShaderTable.SizeInBytes = missResordSize; // Only a s single miss-entry 

	// Hit is the fourth entry in the shader-table
	startAddress += missSectionSize;
	m_raytraceDesc.HitGroupTable.StartAddress = startAddress;
	m_raytraceDesc.HitGroupTable.StrideInBytes = hitRecordSize;
	m_raytraceDesc.HitGroupTable.SizeInBytes = rtEntityCount * hitRecordSize;

	m_raytraceDesc.CallableShaderTable.StartAddress = 0;
	m_raytraceDesc.CallableShaderTable.StrideInBytes = 0;
	m_raytraceDesc.CallableShaderTable.SizeInBytes = 0;

	return true;

}

void QuantumEngine::Rendering::DX12::DX12RayTracingPipeline::RenderCommand(ComPtr<ID3D12GraphicsCommandList7>& commandList, const ref<Camera>& camera)
{
	Matrix4 v = camera->ViewMatrix();
	m_TLASController->UpdateTransforms(commandList, v);

	commandList->SetComputeRootSignature((m_rtProgram->GetReflectionData()->rootSignature).Get());
	commandList->SetPipelineState1(m_rtStateObject.Get());

	commandList->SetDescriptorHeaps(1, m_rtHeap.GetAddressOf());
	m_rtMaterial->RegisterComputeValues(commandList);
	// Run Ray Tracing Pipeline
	commandList->DispatchRays(&m_raytraceDesc);
}
