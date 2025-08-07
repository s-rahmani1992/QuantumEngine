#include "pch.h"
#include "HLSLShader.h"
#include "HLSLShaderImporter.h"
#include <dxcapi.h>
#include <wrl/client.h>
#include <fstream>
#include <vector>
#include <string>
#include <comdef.h>
#include <filesystem>

using namespace Microsoft::WRL;

ref<QuantumEngine::Rendering::Shader> QuantumEngine::Rendering::DX12::HLSLShaderImporter::Import(const std::wstring& fileName, const DX12_Shader_Type& shaderType, std::string& error)
{
    // Read file into memory
    std::ifstream shaderFile(fileName, std::ios::binary | std::ios::ate);
    if (!shaderFile) {
        error = "Failed to compile file at " + std::to_string(shaderType) + "\nFailed to open shader file.";
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

    HRESULT result;

    // Create Utils
    ComPtr<IDxcUtils> pUtils;
    result = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
    
    if (FAILED(result)) {
        return nullptr;
    }

    // Create Include Handler
    ComPtr<IDxcIncludeHandler> includeHandler;
    result = pUtils->CreateDefaultIncludeHandler(&includeHandler);

    if (FAILED(result)) {
        return nullptr;
    }

    // Create Compiler
    ComPtr<IDxcCompiler3> dxcCompiler;
    result = result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    
    if (FAILED(result)) {
        return nullptr;
    }

    // shader type
    std::wstring targetProfile;
    switch (shaderType) {
    case DX12_Shader_Type::VERTEX_SHADER:
        targetProfile = L"vs_6_6";
        break;
    case DX12_Shader_Type::PIXEL_SHADER:
        targetProfile = L"ps_6_6";
        break;
    case DX12_Shader_Type::LIB_SHADER:
        targetProfile = L"lib_6_6";
        break;
        // Add other shader types as needed
    default:
        error = "Unsupported shader type.";
        return nullptr;
    }

    // Compile arguments
    std::vector<LPWSTR> arguments;
    // -E for the entry point (eg. 'main')
    arguments.push_back((WCHAR*)L"-E");
    arguments.push_back((WCHAR*)(shaderType == DX12_Shader_Type::LIB_SHADER ? L"" : L"main"));

    // -T for the target profile (eg. 'ps_6_6')
    arguments.push_back((WCHAR*)L"-T");
    arguments.push_back((WCHAR*)targetProfile.c_str());

    auto path = std::filesystem::path(fileName);
    std::wstring shaderDir = path.parent_path().c_str(); /* extract directory from fileName */;
    arguments.push_back((WCHAR*)L"-I");
    arguments.push_back((WCHAR*)shaderDir.c_str());
    
    // Strip reflection data and pdbs (see later)
    arguments.push_back((WCHAR*)L"-Qstrip_debug");
    arguments.push_back((WCHAR*)L"-Qstrip_reflect");
    arguments.push_back((WCHAR*)L"-Od");

    arguments.push_back((WCHAR*)DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
    arguments.push_back((WCHAR*)DXC_ARG_DEBUG); //-Zi



    ComPtr<IDxcResult> compileResult;
    result = dxcCompiler->Compile(&sourceBuffer, (LPCWSTR*)arguments.data(), arguments.size(), includeHandler.Get(), IID_PPV_ARGS(&compileResult));

    if (FAILED(result)) {
        return nullptr;
    }

    ComPtr<IDxcBlobUtf8> pErrors;

    result = compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);
    
    if (FAILED(result)) {
        return nullptr;
    }
    
    if (pErrors && pErrors->GetStringLength() > 0)
    {
        error = std::string(pErrors->GetStringPointer(), pErrors->GetStringLength());
        return nullptr;
    }

    ComPtr<IDxcBlob> pshaderObjectData;
    result = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pshaderObjectData), nullptr);

    if (FAILED(result)) {
        return nullptr;
    }

    // Get Reflection
    ComPtr<IDxcBlob> pReflectionData;
    compileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(pReflectionData.GetAddressOf()), nullptr);
    DxcBuffer reflectionBuffer{
        .Ptr = pReflectionData->GetBufferPointer(),
        .Size = pReflectionData->GetBufferSize(),
        .Encoding = 0,
    };

    if (shaderType == DX12_Shader_Type::LIB_SHADER) {
        ComPtr<ID3D12LibraryReflection> pLibraryReflection;
        result = pUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&pLibraryReflection));

        if (FAILED(result)) {
            return nullptr;
        }

        return std::make_shared<HLSLShader>((Byte*)pshaderObjectData->GetBufferPointer(), (UInt64)pshaderObjectData->GetBufferSize(), shaderType, pLibraryReflection);
    }
    else {
        ComPtr<ID3D12ShaderReflection> pShaderReflection;
        result = pUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&pShaderReflection));

        if (FAILED(result)) {
            return nullptr;
        }

        return std::make_shared<HLSLShader>((Byte*)pshaderObjectData->GetBufferPointer(), (UInt64)pshaderObjectData->GetBufferSize(), shaderType, pShaderReflection);
    }
}
