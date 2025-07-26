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

	return true;
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::Render()
{
	// Reset Commands
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);
	
	UpdateTLAS();
	m_commandList->SetComputeRootSignature((m_rtProgram->GetReflectionData()->rootSignature).Get());
	m_rtMaterial->SetVector3("camPosition", m_camera->GetTransform()->Position());
	m_rtMaterial->SetMatrix("projectMatrix", m_camera->GetTransform()->Matrix()* m_camera->InverseProjectionMatrix());
	m_rtMaterial->RegisterComputeValues(m_commandList);
	
	// Run Ray Tracing Pipeline
	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width = m_window->GetWidth();
	raytraceDesc.Height = m_window->GetHeight();
	raytraceDesc.Depth = 1;

	// RayGen is the first entry in the shader-table
	raytraceDesc.RayGenerationShaderRecord.StartAddress =
		m_shaderTableBuffer->GetGPUVirtualAddress();
	raytraceDesc.RayGenerationShaderRecord.SizeInBytes = m_mShaderTableEntrySize;

	// Miss is the second entry in the shader-table
	const size_t missOffset = 1 * m_mShaderTableEntrySize;
	raytraceDesc.MissShaderTable.StartAddress = m_shaderTableBuffer->GetGPUVirtualAddress() + missOffset;
	raytraceDesc.MissShaderTable.StrideInBytes = m_mShaderTableEntrySize;
	raytraceDesc.MissShaderTable.SizeInBytes = m_mShaderTableEntrySize * 1; // Only a s single miss-entry 

	// Hit is the fourth entry in the shader-table
	const size_t hitOffset = 2 * m_mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.StartAddress = m_shaderTableBuffer->GetGPUVirtualAddress() + hitOffset;
	raytraceDesc.HitGroupTable.StrideInBytes = m_mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.SizeInBytes = m_mShaderTableEntrySize * 1;

	// Bind the empty root signature
	//mpCmdList->SetComputeRootSignature(mpEmptyRootSig.Get());

	// Dispatch
	m_commandList->SetPipelineState1(m_rtStateObject.Get());
	m_commandList->DispatchRays(&raytraceDesc);

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
	//draw

	for (auto& entity : m_entities) {
		//Pipeline
		m_commandList->SetGraphicsRootSignature(entity.rootSignature.Get());
		m_commandList->SetPipelineState(entity.pipeline.Get());
		//Input Assembler
		m_commandList->IASetVertexBuffers(0, 1, entity.meshController->GetVertexView());
		m_commandList->IASetIndexBuffer(entity.meshController->GetIndexView());
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

		entity.material->RegisterValues(m_commandList);
		entity.material->SetMatrix("worldMatrix", entity.transform->Matrix());
		
		m_commandList->DrawIndexedInstanced(entity.meshController->GetMesh()->GetIndexCount(), 1, 0, 0, 0);
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

void QuantumEngine::Rendering::DX12::DX12GraphicContext::RegisterAssetManager(const ref<GPUAssetManager>& assetManager)
{
	m_assetManager = std::dynamic_pointer_cast<DX12AssetManager>(assetManager);
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::AddGameEntity(ref<GameEntity>& gameEntity)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc;
	//Input part
	auto meshController = m_assetManager->GetMeshController(gameEntity->GetMesh());
	pipelineStateDesc.InputLayout = *(meshController->GetLayoutDesc());
	pipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//Shader Part
	auto vertexShader = std::dynamic_pointer_cast<HLSLShader>(gameEntity->GetMaterial()->GetProgram()->GetShader(VERTEX_SHADER));
	auto pixelShader = gameEntity->GetMaterial()->GetProgram()->GetShader(PIXEL_SHADER);
	pipelineStateDesc.VS.BytecodeLength = vertexShader->GetCodeSize();
	pipelineStateDesc.VS.pShaderBytecode = vertexShader->GetByteCode();
	pipelineStateDesc.PS.BytecodeLength = pixelShader->GetCodeSize();
	pipelineStateDesc.PS.pShaderBytecode = pixelShader->GetByteCode();
	pipelineStateDesc.DS.pShaderBytecode = nullptr;
	pipelineStateDesc.DS.BytecodeLength = 0;
	pipelineStateDesc.HS.pShaderBytecode = nullptr;
	pipelineStateDesc.HS.BytecodeLength = 0;
	pipelineStateDesc.GS.pShaderBytecode = nullptr;
	pipelineStateDesc.GS.BytecodeLength = 0;

	//Root Signature

	auto program = std::dynamic_pointer_cast<HLSLShaderProgram>(gameEntity->GetMaterial()->GetProgram())->GetReflectionData();
	pipelineStateDesc.pRootSignature = program->rootSignature.Get();

	//Rasterizer
	pipelineStateDesc.RasterizerState;
	pipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	pipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	pipelineStateDesc.RasterizerState.FrontCounterClockwise = FALSE;
	pipelineStateDesc.RasterizerState.DepthBias = 0;
	pipelineStateDesc.RasterizerState.DepthBiasClamp = 0.0f;
	pipelineStateDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
	pipelineStateDesc.RasterizerState.DepthClipEnable = FALSE;
	pipelineStateDesc.RasterizerState.MultisampleEnable = FALSE;
	pipelineStateDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	pipelineStateDesc.RasterizerState.ForcedSampleCount = 0;
	pipelineStateDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	//Stream Output
	pipelineStateDesc.StreamOutput.NumEntries = 0;
	pipelineStateDesc.StreamOutput.NumStrides = 0;
	pipelineStateDesc.StreamOutput.pBufferStrides = nullptr;
	pipelineStateDesc.StreamOutput.pSODeclaration = nullptr;
	pipelineStateDesc.StreamOutput.RasterizedStream = 0;

	//Flags and Cache
	pipelineStateDesc.CachedPSO.CachedBlobSizeInBytes = 0;
	pipelineStateDesc.CachedPSO.pCachedBlob = nullptr;
	pipelineStateDesc.NodeMask = 0;
	pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	//RTVs
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStateDesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;

	//DSV
	pipelineStateDesc.DSVFormat = m_depthFormat;

	//Blend
	pipelineStateDesc.BlendState.AlphaToCoverageEnable = FALSE;
	pipelineStateDesc.BlendState.IndependentBlendEnable = FALSE;

	//Blending for Color RTV
	pipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
	pipelineStateDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
	pipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
	pipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	pipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	pipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	pipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	pipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pipelineStateDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	pipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//Depth buffer
	pipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	pipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	pipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	pipelineStateDesc.DepthStencilState.StencilEnable = FALSE;
	pipelineStateDesc.DepthStencilState.StencilReadMask = 0;
	pipelineStateDesc.DepthStencilState.StencilWriteMask = 0;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pipelineStateDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	//Sampling
	pipelineStateDesc.SampleMask = 0xFFFFFFFF;
	pipelineStateDesc.SampleDesc.Count = 1;
	pipelineStateDesc.SampleDesc.Quality = 0;

	//Creating pipeline
	ComPtr<ID3D12PipelineState> pso;
	m_device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pso));
	m_entities.push_back(DX12GameEntityGPU{
		.pipeline = pso,
		.meshController = meshController,
		.rootSignature = program->rootSignature,
		.material = std::dynamic_pointer_cast<HLSLMaterial>(gameEntity->GetMaterial()),
		.transform = gameEntity->GetTransform(),
		.rtMaterial = std::dynamic_pointer_cast<HLSLMaterial>(gameEntity->GetRTMaterial()),
		});
}

bool QuantumEngine::Rendering::DX12::DX12GraphicContext::PrepareRayTracingData(const ref<ShaderProgram>& rtProgram)
{
	// Initialize TLAS
	m_TLASController = std::make_shared<RayTracing::RTAccelarationStructure>();
	std::vector<RayTracing::EntityBLASDesc> rtEntities;

	for (auto& entity : m_entities) {
		rtEntities.push_back(RayTracing::EntityBLASDesc{
			.mesh = entity.meshController,
			.transform = entity.transform,
		});
	}

	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);

	if (m_TLASController->Initialize(m_commandList, rtEntities) == false)
		return false;

	m_commandExecuter->ExecuteAndWait(m_commandList.Get());

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	};

	// Ray Tracing Output Buffer
	// Create the output resource. The dimensions should match the swap-chain
	D3D12_RESOURCE_DESC outputDesc = {};
	outputDesc.DepthOrArraySize = 1;
	outputDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	outputDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB formats can't be used with UAVs. We will convert to sRGB ourselves in the shader
	outputDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	outputDesc.Width = m_window->GetWidth();
	outputDesc.Height = m_window->GetHeight();
	outputDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	outputDesc.MipLevels = 1;
	outputDesc.SampleDesc.Count = 1;

	if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
		&outputDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&m_outputBuffer)
	))) {
		return false;
	}

	if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_outputHeap)))) {
		return false;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavOutputDesc = {};
	uavOutputDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	m_device->CreateUnorderedAccessView( m_outputBuffer.Get(), nullptr, 
		&uavOutputDesc, m_outputHeap->GetCPUDescriptorHandleForHeapStart()
	);

	// Ray Tracing Pipeline
	std::set<ref< HLSLShader>> shaders;
	m_rtProgram = std::dynamic_pointer_cast<HLSLShaderProgram>(rtProgram);
	m_rtMaterial = std::make_shared<DX12::HLSLMaterial>(m_rtProgram);
	m_rtMaterial->Initialize();
	ref<HLSLShader> libShader = std::dynamic_pointer_cast<HLSLShader>(m_rtProgram->GetShader(LIB_SHADER));
	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	subobjects.reserve(6 + 20 * m_entities.size());
	// Create DXIL Sub Object
	
	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
		.pDesc = libShader->GetDXIL(),
		});

	// Global Hit Group
	/*std::wstring globe(GLOBAL_HIT_GROUP_NAME);
	D3D12_HIT_GROUP_DESC globalHitDesc = libShader->GetHitGroup(globe);
	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
		.pDesc = &globalHitDesc,
		});*/

	// Global Root Signature 
	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
		.pDesc = m_rtProgram->GetReflectionData()->rootSignature.GetAddressOf(),
		});

	D3D12_STATE_SUBOBJECT* rootPtr = subobjects.data() + subobjects.size() - 1;

	// Shader Config
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig;
	shaderConfig.MaxPayloadSizeInBytes = 3 * sizeof(Float);
	shaderConfig.MaxAttributeSizeInBytes = 2 * sizeof(Float);

	subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,
		.pDesc = &shaderConfig,
		});

	D3D12_STATE_SUBOBJECT* shaderConfigPtr = subobjects.data() + subobjects.size() - 1;

	// Pipeline Config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
	pipelineConfig.MaxTraceRecursionDepth = 1;

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
		D3D12_HIT_GROUP_DESC hitDesc;
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION hitExportDesc;
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigExportDesc;
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION pipelineConfigExportDesc;
	};

	std::vector<LocalRTData> entityRTDatas;
	entityRTDatas.reserve(m_entities.size() + 1); // Prevent vector from reallocation. So the pointer to entry is always valid

	for (auto& entity : m_entities) {
		if (entity.rtMaterial == nullptr)
			continue;

		entityRTDatas.push_back(LocalRTData{});
		LocalRTData& rtRef = entityRTDatas.back();
		ref<HLSLShader> localLibShader = std::dynamic_pointer_cast<HLSLShader>(entity.rtMaterial->GetProgram()->GetShader(LIB_SHADER));
		
		if (shaders.emplace(localLibShader).second) {
			subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
			.pDesc = localLibShader->GetDXIL(),
				});
		}

		// Create HitGroup for local material
		rtRef.hitExportName = L"Entity_" + std::to_wstring(entityIndex);
		rtRef.hitDesc = localLibShader->GetHitGroup(rtRef.hitExportName);

		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
			.pDesc = &rtRef.hitDesc,
			});

		subobjects.push_back(D3D12_STATE_SUBOBJECT{
		.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,
		.pDesc = std::dynamic_pointer_cast<HLSLShaderProgram>(entity.rtMaterial->GetProgram())->GetReflectionData()->rootSignature.GetAddressOf(),
			});

		LPCWSTR b = rtRef.hitExportName.c_str();
		rtRef.hitExportDesc.pSubobjectToAssociate = subobjects.data() + subobjects.size() - 1;
		rtRef.hitExportDesc.NumExports = 1;
		rtRef.hitExportDesc.pExports = &b;
		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
			.pDesc = &rtRef.hitExportDesc,
		});

		rtRef.shaderConfigExportDesc.pSubobjectToAssociate = shaderConfigPtr;
		rtRef.shaderConfigExportDesc.NumExports = 1;
		rtRef.shaderConfigExportDesc.pExports = &b;
		subobjects.push_back(D3D12_STATE_SUBOBJECT{
			.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
			.pDesc = &rtRef.shaderConfigExportDesc,
			});

		rtRef.pipelineConfigExportDesc.pSubobjectToAssociate = pipelineConfigPtr;
		rtRef.pipelineConfigExportDesc.NumExports = 1;
		rtRef.pipelineConfigExportDesc.pExports = &b;
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

	// Create ShaderTable
	m_mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	m_mShaderTableEntrySize += 8; // The hit shader constant-buffer descriptor
	m_mShaderTableEntrySize = (((m_mShaderTableEntrySize + D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1) / D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT) * D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	const UInt32 shaderTableSize = m_mShaderTableEntrySize * 3;

	ComPtr<ID3D12StateObjectProperties> rtProperties;
	if (FAILED(m_rtStateObject->QueryInterface(IID_PPV_ARGS(&rtProperties))))
		return false;

	D3D12_RESOURCE_DESC shaderTableBufferDesc
	{
	shaderTableBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	shaderTableBufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
	shaderTableBufferDesc.Width = shaderTableSize,
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
	std::memcpy(pData, rtProperties->GetShaderIdentifier(RAYGEN_NAME),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 1 - primary ray miss
	pData += m_mShaderTableEntrySize;
	std::memcpy(pData, rtProperties->GetShaderIdentifier(MISS_NAME),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 2 - hit program
	pData += m_mShaderTableEntrySize;
	std::memcpy(pData, rtProperties->GetShaderIdentifier(GLOBAL_HIT_GROUP_NAME),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	/*const uint64_t heapStartHit = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr;
	*reinterpret_cast<uint64_t*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStartHit;*/

	// Unmap
	m_shaderTableBuffer->Unmap(0, nullptr);

	m_rtMaterial->SetDescriptorHeap("gRtScene", m_TLASController->GetDescriptor());
	m_rtMaterial->SetDescriptorHeap("gOutput", m_outputHeap);

	return true;
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::UpdateTLAS()
{
	Matrix4 v = m_camera->ViewMatrix();
	m_TLASController->UpdateTransforms(m_commandList, v);
}

QuantumEngine::Rendering::DX12::DX12GraphicContext::DX12GraphicContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& commandExecuter, ref<QuantumEngine::Platform::GraphicWindow>& window)
	:m_bufferCount(bufferCount), m_commandExecuter(commandExecuter), m_window(window),
	 m_buffers(std::vector<ComPtr<ID3D12Resource2>>(bufferCount)),
	 m_rtvHandles(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>(bufferCount))
{
}
