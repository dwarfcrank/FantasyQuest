#include "pch.h"

#include "Common.h"
#include "Shader.h"
#include "RendererHelpers.h"

#include <d3dcompiler.h>

using namespace Microsoft::WRL;

ComPtr<ID3DBlob> compileShader(const std::filesystem::path& filename, const char* shaderProfile, const char* entryPoint)
{
    DWORD compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

    if constexpr (IsDebug) {
        compileFlags |= D3DCOMPILE_DEBUG;
        compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
    }

    ComPtr<ID3DBlob> bytecode, errors;

    auto filenameW = filename.wstring();
    Hresult hr = D3DCompileFromFile(filenameW.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint, shaderProfile, 0, 0, &bytecode, &errors);

    return bytecode;
}

ComPtr<ID3D11ComputeShader> compileComputeShader(const ComPtr<ID3D11Device>& device,
    const std::filesystem::path& filename, const char* entryPoint)
{
    ComPtr<ID3D11ComputeShader> shader;
    auto bytecode = compileShader(filename, "cs_5_0", entryPoint);
    Hresult hr = device->CreateComputeShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), nullptr, &shader);

    return shader;
}

ComPtr<ID3D11VertexShader> compileVertexShader(const ComPtr<ID3D11Device>& device,
    const std::filesystem::path& filename, const char* entryPoint)
{
    ComPtr<ID3D11VertexShader> shader;
    auto bytecode = compileShader(filename, "vs_5_0", entryPoint);
    Hresult hr = device->CreateVertexShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), nullptr, &shader);

    return shader;
}

ComPtr<ID3D11VertexShader> compileVertexShader(const ComPtr<ID3D11Device>& device,
    const std::filesystem::path& filename, const char* entryPoint, absl::FunctionRef<void(ID3DBlob*)> callback)
{
    ComPtr<ID3D11VertexShader> shader;
    auto bytecode = compileShader(filename, "vs_5_0", entryPoint);
    Hresult hr = device->CreateVertexShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), nullptr, &shader);

    callback(bytecode.Get());

    return shader;
}

ComPtr<ID3D11PixelShader> compilePixelShader(const ComPtr<ID3D11Device>& device,
    const std::filesystem::path& filename, const char* entryPoint)
{
    ComPtr<ID3D11PixelShader> shader;
    auto bytecode = compileShader(filename, "ps_5_0", entryPoint);
    Hresult hr = device->CreatePixelShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), nullptr, &shader);

    return shader;
}
