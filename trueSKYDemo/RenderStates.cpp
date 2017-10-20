#include "DXUT.h"
#include "RenderStates.h"
#include <minwinbase.h>

ID3D11RasterizerState* RenderStates::CullClockWiseRS = nullptr;
ID3D11RasterizerState* RenderStates::CullCounterClockWiseRS = nullptr;
ID3D11DepthStencilState* RenderStates::OnDepthStencilState = nullptr;
ID3D11DepthStencilState* RenderStates::OffDepthStencilState = nullptr;
RenderStates::RenderStates()
{
}


RenderStates::~RenderStates()
{
}

HRESULT RenderStates::Initialize(ID3D11Device* device)
{
	HRESULT hr = S_OK;
	D3D11_RASTERIZER_DESC rasterDesc;
	ZeroMemory(&rasterDesc,sizeof(rasterDesc));
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = true;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	device->CreateRasterizerState(&rasterDesc, &CullClockWiseRS);

	rasterDesc.FrontCounterClockwise = false;
	V_RETURN(device->CreateRasterizerState(&rasterDesc, &CullCounterClockWiseRS));

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xff;
	depthStencilDesc.StencilWriteMask = 0xff;

	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	V_RETURN(device->CreateDepthStencilState(&depthStencilDesc, &OnDepthStencilState));

	depthStencilDesc.DepthEnable = false;
	V_RETURN(device->CreateDepthStencilState(&depthStencilDesc, &OffDepthStencilState));

	return hr;
}

void RenderStates::Release()
{
	SAFE_RELEASE(CullClockWiseRS);
	SAFE_RELEASE(CullCounterClockWiseRS);
	SAFE_RELEASE(OnDepthStencilState);
	SAFE_RELEASE(OffDepthStencilState);
}
