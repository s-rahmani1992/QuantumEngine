#include "pch.h"
#include "HLSLRayTracingProgramImporter.h"
#include "HLSLRayTracingProgram.h"
#include <dxcapi.h>
#include <wrl/client.h>
#include <fstream>
#include <vector>
#include <string>
#include <comdef.h>
#include <filesystem>
#include "StringUtilities.h"

using namespace Microsoft::WRL;

namespace RayTracing = QuantumEngine::Rendering::DX12::RayTracing;

ref<QuantumEngine::Rendering::ShaderProgram> RayTracing::HLSLRayTracingProgramImporter::ImportShader(const std::wstring& hlslFile, const HLSLRayTracingProgramProperties& properties, std::string& error)
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

    // Compile arguments
    auto targetModel = L"lib_" + CharToString(properties.shaderModel.c_str());

    // Compile arguments
    std::vector<LPWSTR> arguments;
    // -E for the entry point (eg. 'main')
    arguments.push_back((WCHAR*)L"-E");
    arguments.push_back((WCHAR*)L"");

    // -T for the target profile (eg. 'ps_6_6')
    arguments.push_back((WCHAR*)L"-T");
    arguments.push_back((WCHAR*)targetModel.c_str());

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

    ComPtr<ID3D12LibraryReflection> pLibraryReflection;
    result = pUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&pLibraryReflection));

    if (FAILED(result)) {
        return nullptr;
    }

    return std::make_shared<HLSLRayTracingProgram>((Byte*)pshaderObjectData->GetBufferPointer(), (UInt64)pshaderObjectData->GetBufferSize(), properties, pLibraryReflection);
}
