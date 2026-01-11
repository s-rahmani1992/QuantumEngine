#include "pch.h"
#include "HLSLComputeProgramImporter.h"
#include "HLSLComputeProgram.h"
#include "../Shader/HLSLRasterizationProgram.h"
#include <dxcapi.h>
#include <fstream>
#include <filesystem>
#include <comdef.h>
#include <vector>
#include "StringUtilities.h"

using namespace Microsoft::WRL;

namespace Compute = QuantumEngine::Rendering::DX12::Compute;

ref<Compute::HLSLComputeProgram> Compute::HLSLComputeProgramImporter::Import(const std::wstring& hlslFile, const Compute::HLSLComputeProgramImportDesc& properties, std::string& error)
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
    auto mainName = CharToString(properties.mainFunction.c_str());
    auto targetModel = L"cs_" + CharToString(properties.shaderModel.c_str());

    std::vector<LPWSTR> arguments;
    // -E for the entry point (eg. 'main')
    arguments.push_back((WCHAR*)L"-E");
    arguments.push_back((WCHAR*)mainName.c_str());

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
        error = "Failed to compile file at " + WStringToString(hlslFile) + "\nUnKnown Error when Compiling.";
        return nullptr;
    }

    ComPtr<IDxcBlobUtf8> pErrors;

    result = compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);

    if (FAILED(result)) {
        error = "Failed to compile file at " + WStringToString(hlslFile) + "\nUnKnown Error when Compiling.";
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
        error = "Failed to compile file at " + WStringToString(hlslFile) + "\nUnKnown Error when Fetching Bytecode.";
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

    ComPtr<ID3D12ShaderReflection> pShaderReflection;
    result = pUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&pShaderReflection));

	return std::make_shared<Compute::HLSLComputeProgram>((Byte*)pshaderObjectData->GetBufferPointer(), pshaderObjectData->GetBufferSize(), pShaderReflection);
}
