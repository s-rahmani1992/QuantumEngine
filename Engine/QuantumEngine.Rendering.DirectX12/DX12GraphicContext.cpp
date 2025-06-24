#include "pch.h"
#include "DX12GraphicContext.h"
#include "DX12CommandExecuter.h"
#include "Platform/GraphicWindow.h"
#include "DX12AssetManager.h"
#include "DX12MeshController.h"
#include "Core/GameEntity.h"
#include "Core/Mesh.h"
#include "Rendering/ShaderProgram.h"
#include "HLSLShader.h"
#include "HLSLShaderProgram.h"
#include "HLSLMaterial.h"


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

	return true;
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::Render()
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
	float clearColor[] = { 0.1f, 0.7f, 0.3f, 1.0f };
	m_commandList->ClearRenderTargetView(m_rtvHandles[m_current_buffer_index], clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &m_rtvHandles[m_current_buffer_index], false, nullptr);
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
		viewPort.MinDepth = 1.0f;
		viewPort.MaxDepth = 0.0f;
		m_commandList->RSSetViewports(1, &viewPort);

		//Rect Scissor
		RECT scissorRect{};
		scissorRect.left = scissorRect.top = 0;
		scissorRect.right = m_window->GetWidth();
		scissorRect.bottom = m_window->GetHeight();
		m_commandList->RSSetScissorRects(1, &scissorRect);

		entity.material->RegisterValues(m_commandList);
		
		//m_commandList->DrawInstanced(entity.meshController->GetMesh()->GetVertexCount(), 1, 0, 0);
		int h = entity.meshController->GetMesh()->GetIndexCount();
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
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

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
	pipelineStateDesc.DepthStencilState.DepthEnable = FALSE;
	pipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	pipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
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
		});
}

QuantumEngine::Rendering::DX12::DX12GraphicContext::DX12GraphicContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& commandExecuter, ref<QuantumEngine::Platform::GraphicWindow>& window)
	:m_bufferCount(bufferCount), m_commandExecuter(commandExecuter), m_window(window),
	 m_buffers(std::vector<ComPtr<ID3D12Resource2>>(bufferCount)),
	 m_rtvHandles(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>(bufferCount))
{
}
