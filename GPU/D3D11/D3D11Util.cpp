#include "ppsspp_config.h"

#include <cstdint>
#include <vector>
#include <d3d11.h>
#include <D3Dcompiler.h>


#if PPSSPP_PLATFORM(UWP)
#define ptr_D3DCompile D3DCompile
#else
#include "thin3d/d3d11_loader.h"
#endif

#include "base/logging.h"
#include "base/stringutil.h"

#include "D3D11Util.h"

static std::vector<uint8_t> CompileShaderToBytecode(const char *code, size_t codeSize, const char *target, UINT flags) {
	ID3DBlob *compiledCode = nullptr;
	ID3DBlob *errorMsgs = nullptr;
	HRESULT result = ptr_D3DCompile(code, codeSize, nullptr, nullptr, nullptr, "main", target, flags, 0, &compiledCode, &errorMsgs);
	std::string errors;
	if (errorMsgs) {
		errors = std::string((const char *)errorMsgs->GetBufferPointer(), errorMsgs->GetBufferSize());
		ELOG("%s: %s", SUCCEEDED(result) ? "warnings" : "errors", errors.c_str());
		OutputDebugStringA(LineNumberString(code).c_str());
		errorMsgs->Release();
	}
	if (compiledCode) {
		const uint8_t *buf = (const uint8_t *)compiledCode->GetBufferPointer();
		std::vector<uint8_t> compiled = std::vector<uint8_t>(buf, buf +  compiledCode->GetBufferSize());
		compiledCode->Release();
		return compiled;
	}
	return std::vector<uint8_t>();
}

ID3D11VertexShader *CreateVertexShaderD3D11(ID3D11Device *device, const char *code, size_t codeSize, std::vector<uint8_t> *byteCodeOut, UINT flags) {
	std::vector<uint8_t> byteCode = CompileShaderToBytecode(code, codeSize, "vs_5_0", flags);
	if (byteCode.empty())
		return nullptr;

	ID3D11VertexShader *vs;
	device->CreateVertexShader(byteCode.data(), byteCode.size(), nullptr, &vs);
	if (byteCodeOut)
		*byteCodeOut = byteCode;
	return vs;
}

ID3D11PixelShader *CreatePixelShaderD3D11(ID3D11Device *device, const char *code, size_t codeSize, UINT flags) {
	std::vector<uint8_t> byteCode = CompileShaderToBytecode(code, codeSize, "ps_5_0", flags);
	if (byteCode.empty())
		return nullptr;

	ID3D11PixelShader *ps;
	device->CreatePixelShader(byteCode.data(), byteCode.size(), nullptr, &ps);
	return ps;
}

ID3D11ComputeShader *CreateComputeShaderD3D11(ID3D11Device *device, const char *code, size_t codeSize, UINT flags) {
	std::vector<uint8_t> byteCode = CompileShaderToBytecode(code, codeSize, "cs_5_0", flags);
	if (byteCode.empty())
		return nullptr;

	ID3D11ComputeShader *cs;
	device->CreateComputeShader(byteCode.data(), byteCode.size(), nullptr, &cs);
	return cs;
}

ID3D11GeometryShader *CreateGeometryShaderD3D11(ID3D11Device *device, const char *code, size_t codeSize, UINT flags) {
	std::vector<uint8_t> byteCode = CompileShaderToBytecode(code, codeSize, "gs_5_0", flags);
	if (byteCode.empty())
		return nullptr;

	ID3D11GeometryShader *gs;
	device->CreateGeometryShader(byteCode.data(), byteCode.size(), nullptr, &gs);
	return gs;
}


void StockObjectsD3D11::Create(ID3D11Device *device) {
	D3D11_BLEND_DESC blend_desc{};
	blend_desc.RenderTarget[0].BlendEnable = false;
	blend_desc.IndependentBlendEnable = false;
	for (int i = 0; i < 16; i++) {
		blend_desc.RenderTarget[0].RenderTargetWriteMask = i;
		device->CreateBlendState(&blend_desc, &blendStateDisabledWithColorMask[i]);
	}

	D3D11_DEPTH_STENCIL_DESC depth_desc{};
	depth_desc.DepthEnable = FALSE;
	device->CreateDepthStencilState(&depth_desc, &depthStencilDisabled);
	depth_desc.StencilEnable = TRUE;
	depth_desc.StencilReadMask = 0xFF;
	depth_desc.StencilWriteMask = 0xFF;
	depth_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	depth_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
	depth_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
	depth_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depth_desc.BackFace = depth_desc.FrontFace;
	device->CreateDepthStencilState(&depth_desc, &depthDisabledStencilWrite);

	D3D11_RASTERIZER_DESC raster_desc{};
	raster_desc.FillMode = D3D11_FILL_SOLID;
	raster_desc.CullMode = D3D11_CULL_NONE;
	raster_desc.ScissorEnable = FALSE;
	device->CreateRasterizerState(&raster_desc, &rasterStateNoCull);

	D3D11_SAMPLER_DESC sampler_desc{};
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	device->CreateSamplerState(&sampler_desc, &samplerPoint2DWrap);
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	device->CreateSamplerState(&sampler_desc, &samplerLinear2DWrap);
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	device->CreateSamplerState(&sampler_desc, &samplerPoint2DClamp);
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	device->CreateSamplerState(&sampler_desc, &samplerLinear2DClamp);
}

void StockObjectsD3D11::Destroy() {
	for (int i = 0; i < 16; i++) {
		blendStateDisabledWithColorMask[i]->Release();
	}
	depthStencilDisabled->Release();
	depthDisabledStencilWrite->Release();
	rasterStateNoCull->Release();
	samplerPoint2DWrap->Release();
	samplerLinear2DWrap->Release();
	samplerPoint2DClamp->Release();
	samplerLinear2DClamp->Release();
}

StockObjectsD3D11 stockD3D11;