#pragma once
#include <memory>
#include "RenderStates.h"

__declspec(align(16)) class FrameBuffer
{
public:
	FrameBuffer(void);
	~FrameBuffer(void);

	bool Initialize();
	HRESULT OnD3D11CreateDevice(ID3D11Device*, ID3D11DeviceContext*);
	void Release();

	void Resize(int W,int H);
	void SetFormat(DXGI_FORMAT);
	void SetDepthFormat(DXGI_FORMAT);
	HRESULT UpdateFormat();
	HRESULT UpdateDepthFormat();

	ID3D11Texture2D* GetDepthResource();
	ID3D11Texture2D* GetFrameBufferResource();
	ID3D11ShaderResourceView* GetDepthSRV();
	ID3D11ShaderResourceView* GetFrameBufferSRV();
	ID3D11RenderTargetView* GetRenderTargetView();
	ID3D11DepthStencilView* GetDepthStencilView();
	D3D11_VIEWPORT*	GetViewPort();

	void ClearRTV(float ClearColor[4]);
	void ClearDepth(float Depth);
	void Activate(bool clear=true);
	void Deactivate();
	void DeactivateDepth();
	void ActivateDepth(bool clear=false);
	ID3D11DeviceContext* GetD3dImmediateContext();

	static DXGI_FORMAT GetDepthResourceFormat(DXGI_FORMAT depthformat);
	static DXGI_FORMAT GetDepthSRVFormat(DXGI_FORMAT depthformat);

	//void* operator new(size_t i)
	//{
	//	return _mm_malloc(i, 16);
	//}

	//void operator delete(void* p)
	//{
	//	_mm_free(p);
	//}
private:
	D3D11_VIEWPORT									m_viewport;
	DXGI_FORMAT										m_ResourceFormat;
	DXGI_FORMAT										m_DepthDSVFormat;
	ID3D11Device*									m_pd3dDevice;
	ID3D11DeviceContext*							m_pd3dImmediateContext;
	ID3D11Texture2D*								m_pFrameResource;
	ID3D11RenderTargetView*							m_pFrameRenderTargetView;
	ID3D11ShaderResourceView*						m_pFrameShaderResourceView;
	ID3D11DepthStencilView*							m_pDepthStencilView;
	ID3D11Texture2D*								m_pDepthStencilBuffer;
	ID3D11ShaderResourceView*						m_pDepthSRV;
	ID3D11BlendState*								m_pBlendState;
	int												width;
	int												height;
};

