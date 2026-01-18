#include "pch.h"
#include "DX12ShaderRegistery.h"
#include "Rendering/ShaderProgram.h"
#include "HLSLShaderProgram.h"
#include "HLSLShaderImporter.h"
#include <Shader/HLSLRasterizationProgram.h>
#include <Rendering/Shader.h>
#include "HLSLShader.h"
#include <vector>
#include <memory>
#include "Compute/HLSLComputeProgramImporter.h"
#include "Compute/HLSLComputeProgram.h"
#include "RayTracing/HLSLRayTracingProgram.h"

namespace Render = QuantumEngine::Rendering;
namespace HLSL = QuantumEngine::Rendering::DX12::Shader;
namespace Compute = QuantumEngine::Rendering::DX12::Compute;

void QuantumEngine::Rendering::DX12::DX12ShaderRegistery::Initialize(const ComPtr<ID3D12Device10>& device)
{
	m_device = device;

	LPWSTR rootF = new WCHAR[500];
	DWORD size;
	size = GetModuleFileNameW(NULL, rootF, 500);
	std::wstring root = std::wstring(rootF, size);

	const size_t last_slash_idx = root.rfind('\\');
	if (std::string::npos != last_slash_idx)
		root = root.substr(0, last_slash_idx);

	delete[] rootF;
	std::string errorStr;

	ref<Render::Shader> vertexShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\g_buffer.vert.hlsl", DX12::VERTEX_SHADER, errorStr);

	if (vertexShader == nullptr) {
		return;
	}

	ref<Render::Shader> pixelShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\g_buffer.pix.hlsl", DX12::PIXEL_SHADER, errorStr);

	if (pixelShader == nullptr) {
		return;
	}

	CreateAndRegisterShaderProgram("G_Buffer_Program", { vertexShader, pixelShader }, false);

	ref<Render::Shader> rtGBufferShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\g_buffer_rt_global.lib.hlsl", DX12::LIB_SHADER, errorStr);

	if (rtGBufferShader == nullptr) {
		return;
	}

	CreateAndRegisterShaderProgram("G_Buffer_RT_Global_Program", { rtGBufferShader }, true);

	Compute::HLSLComputeProgramImportDesc computeDesc;
	computeDesc.mainFunction = "cs_main";
	computeDesc.shaderModel = "6_6";

	ref<Compute::HLSLComputeProgram> computeProgram = Compute::HLSLComputeProgramImporter::Import(root + L"\\Assets\\Shaders\\curve_mesh_compute.cs.hlsl", computeDesc, errorStr);

	if (computeProgram == nullptr) {
		return;
	}

	if (computeProgram->InitializeRootSignature(m_device)) {
		m_shaderPrograms["Bezier_Curve_Compute_Program"] = computeProgram;
	}
}

ref<QuantumEngine::Rendering::DX12::HLSLShaderProgram> QuantumEngine::Rendering::DX12::DX12ShaderRegistery::GetShaderProgram(const std::string& name)
{
	auto it = m_shaders.find(name);
	if (it != m_shaders.end())
		return (*it).second;
	return nullptr;
}

ref<QuantumEngine::Rendering::ShaderProgram> QuantumEngine::Rendering::DX12::DX12ShaderRegistery::GetGenericShaderProgram(const std::string& name)
{
	auto it = m_shaderPrograms.find(name);
	if (it != m_shaderPrograms.end())
		return (*it).second;
	return nullptr;
}

void QuantumEngine::Rendering::DX12::DX12ShaderRegistery::RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT)
{
	ref<HLSL::HLSLRasterizationProgram> rasterProgram = std::dynamic_pointer_cast<HLSL::HLSLRasterizationProgram>(program);

	if(rasterProgram != nullptr) {
		if (rasterProgram->InitializeRootSignature(m_device))
			m_shaders[name] = std::dynamic_pointer_cast<HLSLShaderProgram>(rasterProgram);
		return;
	}

	ref<RayTracing::HLSLRayTracingProgram> rtProgram = std::dynamic_pointer_cast<RayTracing::HLSLRayTracingProgram>(program);

	if(rtProgram != nullptr) {
		if (rtProgram->InitializeRootSignature(m_device)) 
			m_shaders[name] = std::dynamic_pointer_cast<HLSLShaderProgram>(rtProgram);
		return;
	}

	ref<HLSLShaderProgram> hlslProgram = std::dynamic_pointer_cast<HLSLShaderProgram>(program);

	if (hlslProgram->Initialize(m_device, isRT))
		m_shaders[name] = std::dynamic_pointer_cast<HLSLShaderProgram>(hlslProgram);
}

ref<Render::ShaderProgram> QuantumEngine::Rendering::DX12::DX12ShaderRegistery::CreateAndRegisterShaderProgram(const std::string& name, const std::initializer_list<ref<Render::Shader>>& shaders, bool isRT)
{
	std::vector<ref<HLSLShader>> hlslShaders;

	if(isRT == false) {
		for (auto& shader : shaders) {
			ref<HLSLShader> hlsl = std::dynamic_pointer_cast<HLSLShader>(shader);
			hlslShaders.push_back(hlsl);
		}
		ref<HLSL::HLSLRasterizationProgram> rasterProgram = std::make_shared<Shader::HLSLRasterizationProgram>(hlslShaders);

		if (rasterProgram->InitializeRootSignature(m_device)) {
			m_shaders[name] = std::dynamic_pointer_cast<HLSLShaderProgram>(rasterProgram);
			return rasterProgram;
		}

		return nullptr;
	}
	ref<HLSLShaderProgram> program = std::make_shared<HLSLShaderProgram>(shaders);

	if (program->Initialize(m_device, isRT))
		m_shaders[name] = program;

	return program;
}
