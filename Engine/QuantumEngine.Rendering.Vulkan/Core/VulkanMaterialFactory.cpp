#include "vulkan-pch.h"
#include "VulkanMaterialFactory.h"
#include "Core/SPIRVShaderProgram.h"
#include "Rendering/Material.h"


ref<QuantumEngine::Rendering::Material> QuantumEngine::Rendering::Vulkan::VulkanMaterialFactory::CreateMaterial(const ref<ShaderProgram>& program)
{
	ref<SPIRVShaderProgram> rasterProgram = std::dynamic_pointer_cast<SPIRVShaderProgram>(program);
	
	if (rasterProgram == nullptr)
		return std::make_shared<Material>(program);

	auto matReflection = rasterProgram->GetReflection().CreateMaterialReflection();
	return std::make_shared<Material>(program, &matReflection);
}
