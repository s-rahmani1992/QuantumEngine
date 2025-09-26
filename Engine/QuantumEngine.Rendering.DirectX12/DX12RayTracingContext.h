#pragma once
#include "DX12GraphicContext.h"

namespace QuantumEngine::Rendering::DX12
{
	class DX12RayTracingContext : public DX12GraphicContext
	{
	public:
		DX12RayTracingContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& m_commandExecuter, ref<QuantumEngine::Platform::GraphicWindow>& window) 
			: DX12GraphicContext(bufferCount, m_commandExecuter, window) {}
		
		virtual bool Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory) override;
		virtual bool PrepareScene(const std::vector<ref<GameEntity>>& gameEntities, const ref<Camera>& camera, const SceneLightData& lights, const ref<ShaderProgram>& rtProgram) override;
		virtual void Render() override;

	private:
		void PrepareRayTracingPipeline(const ref<ShaderProgram>& rtProgram);

	private:
		std::vector<DX12RayTracingGPUData> m_rtEntityData;
		ref<HLSLMaterial> m_rtMaterial;
		ref<DX12RayTracingPipeline> m_rayTracingPipeline;
	};
}


