#pragma once
#include "vulkan-pch.h"
#include "Rendering/ShaderRegistery.h"
#include <vector>

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::Vulkan {
	class SPIRVShader; 
	enum Vulkan_Shader_Type;

	class VulkanShaderRegistery : public ShaderRegistery
	{
	public:

		VulkanShaderRegistery(VkDevice device);
		~VulkanShaderRegistery();
		virtual void RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT = false) override;
		virtual ref<ShaderProgram> CompileProgram(const std::wstring& fileName, std::string& error) override;
	
	private:
		ref<SPIRVShader> CompileShaderStage(const DxcBuffer* sourceBuffer, Vulkan_Shader_Type shaderType, std::string& error);

		VkDevice m_device;

		IDxcUtils* m_utils;
		IDxcIncludeHandler* m_includeHandler;
		IDxcCompiler3* m_dxcCompiler;

		std::vector<LPWSTR> m_compileArguments;
		const UInt32 m_mainIndex = 1;
		const UInt32 m_targetIndex = 3;
	};
}