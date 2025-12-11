#include "pch.h"
#include "HLSLRasterizationProgramImporter.h"
#include "HLSLRasterizationProgram.h"
#include "../HLSLShader.h"
#include <dxcapi.h>
#include <fstream>
#include <vector>
#include <comdef.h>
#include <filesystem>
#include "StringUtilities.h"

using namespace Microsoft::WRL;

ref<QuantumEngine::Rendering::DX12::Shader::HLSLRasterizationProgram> QuantumEngine::Rendering::DX12::Shader::HLSLRasterizationProgramImporter::ImportShader(const std::wstring& hlslFile, const HLSLRasterizationProgramImportDesc& properties, std::string& error)
{
    // Read file into memory
    std::ifstream shaderFile(hlslFile, std::ios::binary | std::ios::ate);
    if (!shaderFile) {
        error = "Failed to compile file at " + WStringToString(hlslFile) + "\nFailed to open shader file.";
        return nullptr;
    }
    std::streamsize size = shaderFile.tellg();
    shaderFile.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!shaderFile.read(buffer.data(), size)) {
        error = "Failed to read shader file.";
        return nullptr;
    }

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = buffer.data();
    sourceBuffer.Size = buffer.size();
    sourceBuffer.Encoding = DXC_CP_ACP;

    // Create Utils
    HRESULT result;

    ComPtr<IDxcUtils> pUtils;
    result = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));

    if (FAILED(result)) {
		error = "Failed to compile file at " + WStringToString(hlslFile) + "\nFailed to create Utils.";
        return nullptr;
    }

    // Create Include Handler
    ComPtr<IDxcIncludeHandler> includeHandler;
    result = pUtils->CreateDefaultIncludeHandler(&includeHandler);

    if (FAILED(result)) {
		error = "Failed to compile file at " + WStringToString(hlslFile) + "\nFailed to create include handler.";
        return nullptr;
    }

    // Create Compiler
    ComPtr<IDxcCompiler3> dxcCompiler;
    result = result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));

    if (FAILED(result)) {
        error = "Failed to compile file at " + WStringToString(hlslFile) + "\nFailed to create Compiler.";
        return nullptr;
    }

    // Compile arguments
    std::vector<LPWSTR> arguments;
    // -E for the entry point (eg. 'main')
    arguments.push_back((WCHAR*)L"-E");
    arguments.push_back((WCHAR*)(L"main"));

    // -T for the target profile (eg. 'ps_6_6')
    arguments.push_back((WCHAR*)L"-T");
    arguments.push_back((WCHAR*)L"6_6");

    auto path = std::filesystem::path(hlslFile);
    std::wstring shaderDir = path.parent_path().c_str(); /* extract directory from fileName */;
    arguments.push_back((WCHAR*)L"-I");
    arguments.push_back((WCHAR*)shaderDir.c_str());

    // Strip reflection data and pdbs (see later)
    arguments.push_back((WCHAR*)L"-Qstrip_debug");
    arguments.push_back((WCHAR*)L"-Qstrip_reflect");
    arguments.push_back((WCHAR*)L"-Od");

    arguments.push_back((WCHAR*)DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
    arguments.push_back((WCHAR*)DXC_ARG_DEBUG); //-Zi

    UInt8 mainIndex = 1;
    UInt8 targetIndex = 3;
    std::wstring mainName;
    std::wstring targetModel;
    ComPtr<IDxcResult> compileResult;
    std::vector<ref<HLSLShader>> shaders;

	// function to compile shader of given type for each stage
	auto compileFunc = [&pUtils, &dxcCompiler ,&compileResult, &arguments, &sourceBuffer, &includeHandler, &shaders](DX12_Shader_Type sType, std::string& error, bool& ok) {
        HRESULT result;
        result = dxcCompiler->Compile(&sourceBuffer, (LPCWSTR*)arguments.data(), arguments.size(), includeHandler.Get(), IID_PPV_ARGS(&compileResult));

        if (FAILED(result)) {
            ok = false;
            return;
        }

        ComPtr<IDxcBlobUtf8> pErrors;

        result = compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);

        if (FAILED(result)) {
            ok = false;
            return;
        }

        if (pErrors && pErrors->GetStringLength() > 0)
        {
            error = std::string(pErrors->GetStringPointer(), pErrors->GetStringLength());
            ok = false;
            return;
        }

        ComPtr<IDxcBlob> pshaderObjectData;
        result = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pshaderObjectData), nullptr);

        if (FAILED(result)) {
            ok = false;
            return;
        }

        // Get Reflection
        ComPtr<IDxcBlob> pReflectionData;
        compileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(pReflectionData.GetAddressOf()), nullptr);
        DxcBuffer reflectionBuffer{
            .Ptr = pReflectionData->GetBufferPointer(),
            .Size = pReflectionData->GetBufferSize(),
            .Encoding = 0,
        };

        ComPtr<ID3D12ShaderReflection> pShaderReflection;
        result = pUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&pShaderReflection));

        auto vertexShader = std::make_shared<HLSLShader>((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), sType, pShaderReflection);
        shaders.push_back(vertexShader);

        ok = true;
        };

	// Compile vertex shader if Exists
	if(properties.vertexMainFunction.empty() == false)
    {
		mainName = CharToString(properties.vertexMainFunction.c_str());
		targetModel = L"vs_" + CharToString(properties.shaderModel.c_str());
        arguments[mainIndex] = (WCHAR*)mainName.c_str();
		arguments[targetIndex] = (WCHAR*)targetModel.c_str();
		bool ok = true;
		compileFunc(DX12_Shader_Type::VERTEX_SHADER, error, ok);

        if(ok == false)
        {
            return nullptr;
		}
	}

	// Compile pixel shader if Exists
    if(properties.pixelMainFunction.empty() == false)
    {
        mainName = CharToString(properties.pixelMainFunction.c_str());
        targetModel = L"ps_" + CharToString(properties.shaderModel.c_str());
        arguments[mainIndex] = (WCHAR*)mainName.c_str();
        arguments[targetIndex] = (WCHAR*)targetModel.c_str();
		bool ok = true;
        compileFunc(DX12_Shader_Type::PIXEL_SHADER, error, ok);

        if (ok == false)
        {
            return nullptr;
		}
	}

	return std::make_shared<HLSLRasterizationProgram>(shaders);
}
