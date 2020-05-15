#pragma once

#include <filesystem>
#include <wrl.h>
#include <d3d11_1.h>

#include <absl/functional/function_ref.h>

Microsoft::WRL::ComPtr<ID3D11ComputeShader> compileComputeShader(
    const Microsoft::WRL::ComPtr<ID3D11Device>& device,
    const std::filesystem::path& filename,
    const char* entryPoint
);

Microsoft::WRL::ComPtr<ID3D11VertexShader> compileVertexShader(
    const Microsoft::WRL::ComPtr<ID3D11Device>& device,
    const std::filesystem::path& filename,
    const char* entryPoint
);

Microsoft::WRL::ComPtr<ID3D11VertexShader> compileVertexShader(
    const Microsoft::WRL::ComPtr<ID3D11Device>& device,
    const std::filesystem::path& filename,
    const char* entryPoint,
    absl::FunctionRef<void(ID3DBlob*)> callback
);

Microsoft::WRL::ComPtr<ID3D11PixelShader> compilePixelShader(
    const Microsoft::WRL::ComPtr<ID3D11Device>& device,
    const std::filesystem::path& filename,
    const char* entryPoint
);

Microsoft::WRL::ComPtr<ID3D11GeometryShader> compileGeometryShader(
    const Microsoft::WRL::ComPtr<ID3D11Device>& device,
    const std::filesystem::path& filename,
    const char* entryPoint
);
