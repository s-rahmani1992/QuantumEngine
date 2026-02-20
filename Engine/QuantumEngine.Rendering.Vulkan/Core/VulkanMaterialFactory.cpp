#include "vulkan-pch.h"
#include "VulkanMaterialFactory.h"
#include "Rasterization/SPIRVRasterizationProgram.h"
#include "Rendering/Material.h"


ref<QuantumEngine::Rendering::Material> QuantumEngine::Rendering::Vulkan::VulkanMaterialFactory::CreateMaterial(const ref<ShaderProgram>& program)
{
	ref<Rasterization::SPIRVRasterizationProgram> rasterProgram = std::dynamic_pointer_cast<Rasterization::SPIRVRasterizationProgram>(program);
	auto matReflection = rasterProgram->GetReflection().CreateMaterialReflection();
	return std::make_shared<Material>(program, &matReflection);
}
