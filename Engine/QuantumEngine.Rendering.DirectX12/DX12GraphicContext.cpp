#include "pch.h"
#include "DX12GraphicContext.h"
#include "DX12CommandExecuter.h"
#include "Platform/GraphicWindow.h"
#include "DX12AssetManager.h"
#include "DX12MeshController.h"
#include "Core/GameEntity.h"
#include "Core/Mesh.h"
#include "Core/Transform.h"
#include "Core/Camera/Camera.h"
#include "Rendering/ShaderProgram.h"
#include "HLSLShader.h"
#include "HLSLShaderProgram.h"
#include "HLSLMaterial.h"
#include <vector>
#include "RayTracing/RTAccelarationStructure.h"
#include "DX12Utilities.h"
#include <set>
#include "DX12GameEntityPipeline.h"
#include "Dx12RayTracingPipeline.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/RayTracingComponent.h"

bool QuantumEngine::Rendering::DX12::DX12GraphicContext::Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory)
{
	m_device = device;

	//command allocator
	if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator))))
		return false;

	//Command List
	if (FAILED(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_commandList))))
		return false;

	//Create Swap Chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc1
	{
	.Width = m_window->GetWidth(),
	.Height = m_window->GetHeight(),
	.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	.Stereo = false,
	.SampleDesc = DXGI_SAMPLE_DESC
		{
		.Count = 1,
		.Quality = 0,
		},
	.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT,
	.BufferCount = m_bufferCount,
	.Scaling = DXGI_SCALING_STRETCH,
	.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
	.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
	.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
	};

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc{ 0 };
	swapChainFullScreenDesc.Windowed = true;

	ComPtr<IDXGISwapChain1> swapChain1;

	if (FAILED(factory->CreateSwapChainForHwnd(m_commandExecuter->GetQueue().Get(), m_window->GetHandle(), &swapChainDesc1, &swapChainFullScreenDesc, nullptr, &swapChain1))) {
		return false;
	}

	if (FAILED(swapChain1.As(&m_swapChain)))
		return false;

	// Create Heap for Render Target view
	D3D12_DESCRIPTOR_HEAP_DESC rtv_desc_heap_desc{
	.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
	.NumDescriptors = m_bufferCount,
	.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	.NodeMask = 0,
	};
	if (FAILED(device->CreateDescriptorHeap(&rtv_desc_heap_desc, IID_PPV_ARGS(&m_rtvDescriptorHeap))))
		return false;

	auto firstHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto handleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (size_t i = 0; i < m_bufferCount; i++) {
		m_rtvHandles[i] = firstHandle;
		m_rtvHandles[i].ptr += i * handleIncrementSize;
	}

	//create buffers and render target views
	for (size_t i = 0; i < m_bufferCount; i++) {
		m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_buffers[i]));
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
		{
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D = D3D12_TEX2D_RTV
			{
				.MipSlice = 0,
				.PlaneSlice = 0
			},
		};
		device->CreateRenderTargetView(m_buffers[i].Get(), &rtv_desc, m_rtvHandles[i]);
	}

	// Create Depth Buffer
	D3D12_RESOURCE_DESC depthResourceDesc;
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Alignment = 0;
	depthResourceDesc.Width = m_window->GetWidth();
	depthResourceDesc.Height = m_window->GetHeight();
	depthResourceDesc.DepthOrArraySize = 1;
	depthResourceDesc.MipLevels = 1;
	depthResourceDesc.Format = m_depthFormat;
	depthResourceDesc.SampleDesc.Count = 1;
	depthResourceDesc.SampleDesc.Quality = 0;
	depthResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES depthHeapProps
	{
	.Type = D3D12_HEAP_TYPE_DEFAULT,
	.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	.CreationNodeMask = 0,
	.VisibleNodeMask = 0,
	};

	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = m_depthFormat;
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.DepthStencil.Stencil = 0;

	if (FAILED(device->CreateCommittedResource(&depthHeapProps, D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue,
		IID_PPV_ARGS(&m_depthStencilBuffer)))) 
		return false;

	D3D12_DESCRIPTOR_HEAP_DESC depthHeapDesc{
	.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
	.NumDescriptors = 1,
	.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	.NodeMask = 0,
	};

	if (FAILED(device->CreateDescriptorHeap(&depthHeapDesc, IID_PPV_ARGS(&m_depthStencilvHeap))))
		return false;

	D3D12_DEPTH_STENCIL_VIEW_DESC depthViewDesc{
		.Format = m_depthFormat,
		.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		.Flags = D3D12_DSV_FLAG_NONE,
		.Texture2D = D3D12_TEX2D_DSV{.MipSlice = 0},
	};

	device->CreateDepthStencilView(
		m_depthStencilBuffer.Get(),
		&depthViewDesc,
		m_depthStencilvHeap->GetCPUDescriptorHandleForHeapStart());

	auto cameraResourceDesc = ResourceUtilities::GetCommonBufferResourceDesc(CONSTANT_BUFFER_ALIGHT(sizeof(CameraGPU)), D3D12_RESOURCE_FLAG_NONE);

	if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonUploadHeapProps, D3D12_HEAP_FLAG_NONE,
		&cameraResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_cameraBuffer))))
	{
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};

	if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cameraHeap)))) {
		return false;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC transformViewDesc;
	transformViewDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
	transformViewDesc.SizeInBytes = cameraResourceDesc.Width;

	m_device->CreateConstantBufferView(&transformViewDesc, m_cameraHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::Render()
{
	m_camData.inverseProjectionMatrix = m_camera->GetTransform()->Matrix() * m_camera->InverseProjectionMatrix();
	m_camData.viewMatrix = m_camera->ViewMatrix();
	m_camData.position = m_camera->GetTransform()->Position();

	void* camData;
	m_cameraBuffer->Map(0, nullptr, &camData);
	std::memcpy(camData, &m_camData, sizeof(CameraGPU));
	m_cameraBuffer->Unmap(0, nullptr);

	for (auto& entity : m_entityGPUData) {
		m_transformData.modelMatrix = entity.gameEntity->GetTransform()->Matrix();
		m_transformData.modelViewMatrix = m_camData.viewMatrix * m_transformData.modelMatrix;
		m_transformData.rotationMatrix = entity.gameEntity->GetTransform()->RotateMatrix();
		void* data;
		entity.transformResource->Map(0, nullptr, &data);
		std::memcpy(data, &m_transformData, sizeof(TransformGPU));
		entity.transformResource->Unmap(0, nullptr);
	}

	//RenderRasterization();
	RenderRayTracing();
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::RegisterAssetManager(const ref<GPUAssetManager>& assetManager)
{
	m_assetManager = std::dynamic_pointer_cast<DX12AssetManager>(assetManager);
}

bool QuantumEngine::Rendering::DX12::DX12GraphicContext::PrepareRayTracingData(const ref<ShaderProgram>& rtProgram)
{
	for(auto& entity : m_entityGPUData) {
		auto rtMaterial = std::dynamic_pointer_cast<HLSLMaterial>(entity.gameEntity->GetRayTracingComponent()->GetRTMaterial());
		if (rtMaterial == nullptr)
			continue;
		rtMaterial->SetDescriptorHeap(HLSL_CAMERA_DATA_NAME, m_cameraHeap);
		rtMaterial->SetDescriptorHeap(HLSL_LIGHT_DATA_NAME, m_lightManager.GetDescriptor());
	}
	m_rtMaterial = std::make_shared<HLSLMaterial>(std::dynamic_pointer_cast<HLSLShaderProgram>(rtProgram));
	m_rtMaterial->Initialize(true);

	m_rayTracingPipeline = std::make_shared<DX12RayTracingPipeline>();

	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);
	m_rtMaterial->SetDescriptorHeap(HLSL_CAMERA_DATA_NAME, m_cameraHeap);
	m_rtMaterial->SetDescriptorHeap(HLSL_LIGHT_DATA_NAME, m_lightManager.GetDescriptor());
	m_rayTracingPipeline->Initialize(m_commandList, m_entityGPUData, m_window->GetWidth(), m_window->GetHeight(), m_rtMaterial);
	
	m_commandExecuter->ExecuteAndWait(m_commandList.Get());

	return true;
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::RegisterLight(const SceneLightData& lights)
{
	m_lightManager.Initialize(lights, m_device);
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::PrepareGameEntities(const std::vector<ref<GameEntity>>& gameEntities)
{
	m_entityGPUData.reserve(gameEntities.size());
	m_rasterizationPipelines.reserve(gameEntities.size());
	auto transformResourceDesc = ResourceUtilities::GetCommonBufferResourceDesc(CONSTANT_BUFFER_ALIGHT(sizeof(TransformGPU)), D3D12_RESOURCE_FLAG_NONE);

	// Create Transform Resources and Rasterization Heap 
	for(auto& gameEntity : gameEntities) {
		ComPtr<ID3D12Resource2> transformResource;

		if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonUploadHeapProps, D3D12_HEAP_FLAG_NONE,
			&transformResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&transformResource))))
		{
			return;
		}

		m_entityGPUData.push_back(DX12EntityGPUData{
			.gameEntity = gameEntity,
			.transformResource = transformResource,
			});
	}

	UInt32 rasterHeapSize = m_entityGPUData.size() + 1 + 1; // entity transforms + camera + light
	std::set<ref<HLSLMaterial>> usedMaterials;
	for (auto& entityGpu : m_entityGPUData) {
		auto material = std::dynamic_pointer_cast<HLSLMaterial>(entityGpu.gameEntity->GetRenderer()->GetMaterial());
		if (usedMaterials.emplace(material).second == false)
			continue;
		rasterHeapSize += material->GetBoundResourceCount();
	}

	D3D12_DESCRIPTOR_HEAP_DESC rtHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = rasterHeapSize,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	};

	m_device->CreateDescriptorHeap(&rtHeapDesc, IID_PPV_ARGS(&m_rasterHeap));

	// Create views for camera and lights
	auto incrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto firstHandle = m_rasterHeap->GetCPUDescriptorHandleForHeapStart();
	auto gpuHandle = m_rasterHeap->GetGPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cameraViewDesc;
	cameraViewDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
	cameraViewDesc.SizeInBytes = CONSTANT_BUFFER_ALIGHT(sizeof(CameraGPU));
	m_device->CreateConstantBufferView(&cameraViewDesc, firstHandle);
	m_cameraHandle = gpuHandle;
	
	firstHandle.ptr += incrementSize;
	gpuHandle.ptr += incrementSize;
	D3D12_CONSTANT_BUFFER_VIEW_DESC lightViewDesc;
	lightViewDesc.BufferLocation = m_lightManager.GetResource()->GetGPUVirtualAddress();
	lightViewDesc.SizeInBytes = m_lightManager.GetResource()->GetDesc().Width;
	m_device->CreateConstantBufferView(&lightViewDesc, firstHandle);
	m_lightHandle = gpuHandle;

	// Populate Transform View and Pipelines
	firstHandle.ptr += incrementSize;
	gpuHandle.ptr += incrementSize;

	for (auto& entityGpu : m_entityGPUData) {
		D3D12_CONSTANT_BUFFER_VIEW_DESC transformViewDesc;
		transformViewDesc.BufferLocation = entityGpu.transformResource->GetGPUVirtualAddress();
		transformViewDesc.SizeInBytes = transformResourceDesc.Width;
		m_device->CreateConstantBufferView(&transformViewDesc, firstHandle);

		auto meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(entityGpu.gameEntity->GetRenderer());
		if (meshRenderer != nullptr) {
			m_meshRendererData.push_back(DX12MeshRendererGPUData{
				.meshRenderer = meshRenderer,
				.transformResource = entityGpu.transformResource,
				.transformHandle = gpuHandle,
				});
		}

		firstHandle.ptr += incrementSize;
		gpuHandle.ptr += incrementSize;
	}

	UInt32 offset = 2 + gameEntities.size();

	for(auto& mat : usedMaterials) {
		mat->BindDescriptor(m_rasterHeap, offset);
		offset += mat->GetBoundResourceCount();
	}

	for(auto& meshRender : m_meshRendererData) {
		auto pipeline = std::make_shared<DX12GameEntityPipeline>();
		if (pipeline->Initialize(m_device, meshRender, m_depthFormat) == false) {
			return;
		}

		m_rasterizationPipelines.push_back(pipeline);
	}
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::RenderRasterization()
{
	// Reset Commands
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);

	//Set Render Target
	auto m_current_buffer_index = m_swapChain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER beginBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_buffers[m_current_buffer_index].Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
		.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
		},
	};
	m_commandList->ResourceBarrier(1, &beginBarrier);

	m_commandList->ClearDepthStencilView(m_depthStencilvHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	float clearColor[] = { 0.1f, 0.7f, 0.3f, 1.0f };
	auto dsvHandle = m_depthStencilvHeap->GetCPUDescriptorHandleForHeapStart();
	m_commandList->ClearRenderTargetView(m_rtvHandles[m_current_buffer_index], clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &m_rtvHandles[m_current_buffer_index], false, &dsvHandle);
	//Viewport
	D3D12_VIEWPORT viewPort{};
	viewPort.Height = m_window->GetHeight();
	viewPort.Width = m_window->GetWidth();
	viewPort.TopLeftX = viewPort.TopLeftY = 0;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	m_commandList->RSSetViewports(1, &viewPort);

	//Rect Scissor
	RECT scissorRect{};
	scissorRect.left = scissorRect.top = 0;
	scissorRect.right = m_window->GetWidth();
	scissorRect.bottom = m_window->GetHeight();
	m_commandList->RSSetScissorRects(1, &scissorRect);
	
	//draw
	m_commandList->SetDescriptorHeaps(1, m_rasterHeap.GetAddressOf());

	for (auto& pipeline : m_rasterizationPipelines) {
		pipeline->Render(m_commandList, m_cameraHandle, m_lightHandle);
	}

	//draw

	D3D12_RESOURCE_BARRIER endBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_buffers[m_current_buffer_index].Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
		.StateAfter = D3D12_RESOURCE_STATE_PRESENT,
		},
	};
	m_commandList->ResourceBarrier(1, &endBarrier);

	m_commandExecuter->ExecuteAndWait(m_commandList.Get());
	m_swapChain->Present(1, 0);
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::RenderRayTracing()
{
	// Reset Commands
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);

	m_rayTracingPipeline->RenderCommand(m_commandList, m_camera);

	//Set Render Target
	auto m_current_buffer_index = m_swapChain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER copyRTBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_buffers[m_current_buffer_index].Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
		.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST,
		},
	};

	m_commandList->ResourceBarrier(1, &copyRTBarrier);

	D3D12_RESOURCE_BARRIER outputRTBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_rayTracingPipeline->GetOutputBuffer().Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE,
		},
	};

	m_commandList->ResourceBarrier(1, &outputRTBarrier);
	m_commandList->CopyResource(m_buffers[m_current_buffer_index].Get(), m_rayTracingPipeline->GetOutputBuffer().Get());

	float clearColor[] = { 0.1f, 0.7f, 0.3f, 1.0f };
	auto dsvHandle = m_depthStencilvHeap->GetCPUDescriptorHandleForHeapStart();
	
	//draw

	D3D12_RESOURCE_BARRIER endBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_buffers[m_current_buffer_index].Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
		.StateAfter = D3D12_RESOURCE_STATE_PRESENT,
		},
	};
	m_commandList->ResourceBarrier(1, &endBarrier);

	D3D12_RESOURCE_BARRIER outputEndRTBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_rayTracingPipeline->GetOutputBuffer().Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE,
		.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		},
	};
	m_commandList->ResourceBarrier(1, &outputEndRTBarrier);
	m_commandExecuter->ExecuteAndWait(m_commandList.Get());
	m_swapChain->Present(1, 0);
}

QuantumEngine::Rendering::DX12::DX12GraphicContext::DX12GraphicContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& commandExecuter, ref<QuantumEngine::Platform::GraphicWindow>& window)
	:m_bufferCount(bufferCount), m_commandExecuter(commandExecuter), m_window(window),
	 m_buffers(std::vector<ComPtr<ID3D12Resource2>>(bufferCount)),
	 m_rtvHandles(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>(bufferCount))
{
}
