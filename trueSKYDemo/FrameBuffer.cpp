#include "DXUT.h"
#include "FrameBuffer.h"


FrameBuffer::FrameBuffer(void):
	m_pFrameResource(nullptr),
	m_pFrameRenderTargetView(nullptr),
	m_pFrameShaderResourceView(nullptr),
	m_pDepthStencilView(nullptr),
	m_pDepthStencilBuffer(nullptr),
	m_pDepthSRV(nullptr),
	m_pBlendState(nullptr),
	m_pd3dDevice(nullptr),
	m_pd3dImmediateContext(nullptr),
	width(1024),
	height(768),
	m_ResourceFormat(DXGI_FORMAT_R32G32B32A32_FLOAT),
	m_DepthDSVFormat(DXGI_FORMAT_D24_UNORM_S8_UINT)
{
}

FrameBuffer::~FrameBuffer(void)
{
}

bool Initialize()
{
	return true;
}

void FrameBuffer::Release()
{
	m_pd3dDevice = nullptr;
	m_pd3dImmediateContext = nullptr;
	SAFE_RELEASE(m_pFrameResource);
	SAFE_RELEASE(m_pFrameRenderTargetView);
	SAFE_RELEASE(m_pFrameShaderResourceView);
	SAFE_RELEASE(m_pDepthStencilView);
	SAFE_RELEASE(m_pDepthStencilBuffer);
	SAFE_RELEASE(m_pDepthSRV);
	SAFE_RELEASE(m_pBlendState);
}

void FrameBuffer::Resize(int W,int H)
{
	if(W == width && H == height)
		return;
	width = W;
	height = H;
	UpdateFormat();
	UpdateDepthFormat();
	m_viewport.Width = static_cast<float>(W);
	m_viewport.Height = static_cast<float>(H);
}

void FrameBuffer::SetFormat(DXGI_FORMAT ResourceFormat)
{
	if(m_ResourceFormat == ResourceFormat)
		return;
	m_ResourceFormat = ResourceFormat;
	if(m_pd3dDevice)
		UpdateFormat();
}

void FrameBuffer::SetDepthFormat(DXGI_FORMAT DepthDSVFormat)
{
	if(m_DepthDSVFormat == DepthDSVFormat)
		return;
	m_DepthDSVFormat = DepthDSVFormat;
	if(m_pd3dDevice)
		UpdateDepthFormat();
}

HRESULT FrameBuffer::UpdateFormat()
{
	HRESULT hr = S_OK;

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = m_ResourceFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	SAFE_RELEASE(m_pFrameResource);
	V_RETURN(m_pd3dDevice->CreateTexture2D(&texDesc, nullptr, &m_pFrameResource));

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = texDesc.Format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Texture2D.MipSlice = 0;
	SAFE_RELEASE(m_pFrameRenderTargetView);
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pFrameResource, &RTVDesc, &m_pFrameRenderTargetView));
	

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = texDesc.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;
	SAFE_RELEASE(m_pFrameShaderResourceView);
	V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pFrameResource, &SRVDesc, &m_pFrameShaderResourceView));

	return hr;
}

HRESULT FrameBuffer::UpdateDepthFormat()
{
	HRESULT hr = S_OK;
	DXGI_FORMAT depthDSVFormat = m_DepthDSVFormat;
	DXGI_FORMAT depthTexFormat = GetDepthResourceFormat(depthDSVFormat);
	DXGI_FORMAT depthSRVFormat = GetDepthSRVFormat(depthDSVFormat);

	D3D11_TEXTURE2D_DESC depthDescTex;
	ZeroMemory(&depthDescTex, sizeof(depthDescTex));
	depthDescTex.Width = width;
	depthDescTex.Height = height;
	depthDescTex.MipLevels = 1;
	depthDescTex.ArraySize = 1;
	depthDescTex.Format = depthTexFormat;
	depthDescTex.SampleDesc.Count = 1;
	depthDescTex.SampleDesc.Quality = 0;
	depthDescTex.Usage = D3D11_USAGE_DEFAULT;
	depthDescTex.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthDescTex.CPUAccessFlags = 0;
	depthDescTex.MiscFlags = 0;
	SAFE_RELEASE(m_pDepthStencilBuffer);
	V_RETURN(m_pd3dDevice->CreateTexture2D(&depthDescTex, nullptr, &m_pDepthStencilBuffer));

	D3D11_DEPTH_STENCIL_VIEW_DESC depthDescDSV;
	ZeroMemory(&depthDescDSV, sizeof(depthDescDSV));
	depthDescDSV.Format = depthDSVFormat;
	depthDescDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthDescDSV.Texture2D.MipSlice = 0;
	SAFE_RELEASE(m_pDepthStencilView);
	V_RETURN(m_pd3dDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthDescDSV, &m_pDepthStencilView));

	D3D11_SHADER_RESOURCE_VIEW_DESC depthDescSRV;
	ZeroMemory(&depthDescSRV, sizeof(depthDescSRV));
	depthDescSRV.Format = depthSRVFormat;
	depthDescSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	depthDescSRV.Texture2D.MostDetailedMip = 0;
	depthDescSRV.Texture2D.MipLevels = 1;
	SAFE_RELEASE(m_pDepthSRV);
	V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pDepthStencilBuffer, &depthDescSRV, &m_pDepthSRV));

	return hr;
}

HRESULT FrameBuffer::OnD3D11CreateDevice(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	HRESULT hr = S_OK;
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;

	V_RETURN(UpdateFormat());
	V_RETURN(UpdateDepthFormat());

	m_viewport.MinDepth = 0.f;
	m_viewport.MaxDepth = 1.f;
	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);
	m_viewport.TopLeftX = 0.f;
	m_viewport.TopLeftY = 0.f;

	return hr;
}

void FrameBuffer::ClearRTV(float ClearColor[4])
{
	m_pd3dImmediateContext->ClearRenderTargetView(m_pFrameRenderTargetView, ClearColor);
}

void FrameBuffer::ClearDepth(float Depth)
{
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, Depth, 0);
}

void FrameBuffer::Activate(bool clear)
{
	if(clear)
	{
		float	ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		ClearRTV(ClearColor);
		ClearDepth(1.f);
	}
	m_pd3dImmediateContext->OMSetRenderTargets(1,&m_pFrameRenderTargetView,m_pDepthStencilView);
	m_pd3dImmediateContext->RSSetViewports(1,&m_viewport);
}

void FrameBuffer::Deactivate()
{
}

void FrameBuffer::DeactivateDepth()
{
	//m_pd3dImmediateContext->OMSetDepthStencilState(RenderStates::OffDepthStencilState,1);
	m_pd3dImmediateContext->OMSetRenderTargets(1,&m_pFrameRenderTargetView,nullptr);
}

void FrameBuffer::ActivateDepth(bool clear)
{
	m_pd3dImmediateContext->OMSetDepthStencilState(RenderStates::OnDepthStencilState,1);
}

ID3D11Texture2D* FrameBuffer::GetDepthResource()
{
	return m_pDepthStencilBuffer;
}

ID3D11Texture2D* FrameBuffer::GetFrameBufferResource()
{
	return m_pFrameResource;
}

ID3D11ShaderResourceView* FrameBuffer::GetDepthSRV()
{
	return m_pDepthSRV;
}

ID3D11ShaderResourceView* FrameBuffer::GetFrameBufferSRV()
{
	return m_pFrameShaderResourceView;
}

ID3D11DeviceContext* FrameBuffer::GetD3dImmediateContext()
{
	return m_pd3dImmediateContext;
}

ID3D11RenderTargetView* FrameBuffer::GetRenderTargetView()
{
	return m_pFrameRenderTargetView;
}

D3D11_VIEWPORT*	FrameBuffer::GetViewPort()
{
	return &m_viewport;
}

ID3D11DepthStencilView* FrameBuffer::GetDepthStencilView()
{
	return m_pDepthStencilView;
}

DXGI_FORMAT FrameBuffer::GetDepthResourceFormat(DXGI_FORMAT depthformat)
{
	DXGI_FORMAT resformat;
	switch (depthformat)
	{
	case DXGI_FORMAT::DXGI_FORMAT_D16_UNORM:
		resformat = DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS;
		break;
	case DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT:
		resformat = DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS;
		break;
	case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT:
		resformat = DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS;
		break;
	case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		resformat = DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS;
		break;
	default:
		resformat = DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS;
	}

	return resformat;
}

DXGI_FORMAT FrameBuffer::GetDepthSRVFormat(DXGI_FORMAT depthformat)
{
	DXGI_FORMAT srvformat;
	switch (depthformat)
	{
	case DXGI_FORMAT::DXGI_FORMAT_D16_UNORM:
		srvformat = DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT;
		break;
	case DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT:
		srvformat = DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		break;
	case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT:
		srvformat = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		break;
	case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		srvformat = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		break;
	default:
		srvformat = DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	return srvformat;
}