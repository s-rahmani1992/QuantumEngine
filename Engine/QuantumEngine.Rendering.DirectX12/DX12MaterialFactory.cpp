#include "pch.h"
#include "DX12MaterialFactory.h"
#include "Shader/HLSLRasterizationProgram.h"
#include "Rendering/Material.h"

namespace Render = QuantumEngine::Rendering;

ref<Render::Material> Render::DX12::DX12MaterialFactory::CreateMaterial(const ref<Render::ShaderProgram>& program)
{
	// check if program is HLSL Rasterization Program
	auto hlslProgram = std::dynamic_pointer_cast<Render::DX12::Shader::HLSLRasterizationProgram>(program);

	if(hlslProgram != nullptr) {
		// Extract Reflection Data from HLSL Program
		auto hlslReflection = hlslProgram->GetReflectionData();
		UInt32 fieldIndex = 0;
		MaterialReflection reflectionData;

		// Value Fields are from Root Constants
		for (auto& rootConst : hlslReflection->rootConstants) {
			if (rootConst.name[0] == '_') {// Skip internal root constants
				fieldIndex++;
				continue;
			}

			for(auto& rootVar : rootConst.rootConstants) {
				UInt32 varSize = rootVar.variableDesc.Size;
				MaterialValueFieldInfo valueFieldInfo{
					.name = rootVar.name,
					.fieldIndex = fieldIndex,
					.size = varSize,
				};
				reflectionData.valueFields.push_back(valueFieldInfo);
				fieldIndex++;
			}
		}

		// Texture Fields
		for (auto& resVar : hlslReflection->resourceVariables) {
			if (resVar.name[0] == '_') {// Skip internal variables
				fieldIndex++;
				continue;
			}

			// Only textures are considered for material texture fields
			if (resVar.resourceData.Type == D3D_SIT_TEXTURE) {
				MaterialTextureFieldInfo textureFieldInfo{
					.name = resVar.name,
					.fieldIndex = fieldIndex,
				};
				reflectionData.textureFields.push_back(textureFieldInfo);
				fieldIndex++;
			}
		}
		return std::make_shared<Render::Material>(program, &reflectionData);
	}

	return nullptr;
}
