//--------------------------------------------------------------------------------------
// File: Tutorial08.cpp
//
// Basic introduction to DXUT
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "SDKmisc.h"
#include "DXUTCamera.h"
#include "Model.h" 
#include "TrueSKYRender.h"
#include "FullScreenQuad.h"

#define WIN32_LEAN_AND_MEAN
#define LOAD_MODEL 1
#define TRUESKY 0
#define DEPTH_TEST 1

#pragma warning( disable : 4100 )

using namespace DirectX;

int							screen_width = 1280;
int							screen_height = 720;
XMMATRIX					g_World;
XMMATRIX					g_View;
XMMATRIX					g_Projection;
D3D11_VIEWPORT				g_viewport;
CFirstPersonCamera			mCamera;
XMVECTOR					s_Eye = { 10.0f, 1.0f, 1.0f, 0.f };
XMVECTOR					s_At = { 0.0f, 1.2f, 0.0f, 0.f };
XMVECTOR					s_Up = { 0.0f, 1.0f, 0.0f, 0.f };
ID3D11RasterizerState*		g_pBackCCWRS = nullptr;
ID3D11RasterizerState*		g_pBackCWRS = nullptr;
ID3D11DepthStencilView*		g_pDepthStencilView = nullptr;
ID3D11DepthStencilState*	g_pOnDepthStencilState = nullptr;
ID3D11DepthStencilState*	g_pOffDepthStencilState = nullptr;
ID3D11Texture2D*			g_pDepthStencilBuffer = nullptr;
ID3D11ShaderResourceView*	g_pDepthSRV = nullptr;
ID3D11Texture2D*			g_pTestTex = nullptr;
ID3D11RenderTargetView*		g_pTestRenderTargetView = nullptr;
ID3D11ShaderResourceView*	g_pTestSRV = nullptr;
Model*						m_Model;
FullScreenQuad*				m_DepthTest;
TrueSKYRender*				m_TrueSKYRender;
//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
	DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
	return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
	return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	HRESULT hr = S_OK;

	auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();


	g_World = XMMatrixIdentity();
	mCamera.SetViewParams(s_Eye, s_At);
	g_View = mCamera.GetViewMatrix();

	g_viewport.Width = screen_width;
	g_viewport.Height = screen_height;
	g_viewport.MinDepth = 0.0f;
	g_viewport.MaxDepth = 1.0f;
	g_viewport.TopLeftX = 0.0f;
	g_viewport.TopLeftY = 0.0f; 

	D3D11_RASTERIZER_DESC rasterDesc;
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
	pd3dDevice->CreateRasterizerState(&rasterDesc, &g_pBackCCWRS);

	rasterDesc.FrontCounterClockwise = false;
	pd3dDevice->CreateRasterizerState(&rasterDesc, &g_pBackCWRS);

	D3D11_TEXTURE2D_DESC depthDesc;
	ZeroMemory(&depthDesc, sizeof(depthDesc));
	depthDesc.Width = screen_width;
	depthDesc.Height = screen_height;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;
	V_RETURN(pd3dDevice->CreateTexture2D(&depthDesc, nullptr, &g_pDepthStencilBuffer));

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	V_RETURN(pd3dDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &descDSV, &g_pDepthStencilView));

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	V_RETURN(pd3dDevice->CreateShaderResourceView(g_pDepthStencilBuffer, &shaderResourceViewDesc, &g_pDepthSRV));


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

	V_RETURN(pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &g_pOnDepthStencilState));

	depthStencilDesc.DepthEnable = false;
	V_RETURN(pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &g_pOffDepthStencilState));

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = screen_width;
	texDesc.Height = screen_height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	V_RETURN(pd3dDevice->CreateTexture2D(&texDesc, nullptr, &g_pTestTex));

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = texDesc.Format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Texture2D.MipSlice = 0;
	V_RETURN(pd3dDevice->CreateRenderTargetView(g_pTestTex, &RTVDesc, &g_pTestRenderTargetView));
	

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = texDesc.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;
	V_RETURN(pd3dDevice->CreateShaderResourceView(g_pTestTex, &SRVDesc, &g_pTestSRV));

#if LOAD_MODEL
	m_Model = new Model();
	m_Model->Initialize();
	m_Model->OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext);
	m_Model->SetWVP(g_World*g_View*g_Projection);
#endif

#if DEPTH_TEST
	m_DepthTest = new FullScreenQuad;
	m_DepthTest->Initialize(pd3dDevice,screen_width,screen_height);
	m_DepthTest->OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext);
#endif
#if TRUESKY
	m_TrueSKYRender = new TrueSKYRender();
	int s = sizeof(TrueSKYRender);
	m_TrueSKYRender->Initialize("./basic_setting_plus_windspeed.sq");
	//m_TrueSKYRender->Initialize("");
	m_TrueSKYRender->OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext);
#endif
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	screen_width = pBackBufferSurfaceDesc->Width;
	screen_height = pBackBufferSurfaceDesc->Height;
	g_viewport.Width = screen_width;
	g_viewport.Height = screen_height;
	float fAspect = static_cast<float>(screen_width) / static_cast<float>(screen_height);
	mCamera.SetProjParams(XM_PI * 0.25f, fAspect, 0.1f, 100.0f);
	g_Projection = mCamera.GetProjMatrix();

#if LOAD_MODEL
	m_Model->Resize(pBackBufferSurfaceDesc);
	m_Model->SetWVP(g_World*g_View*g_Projection);
#endif
#if TRUESKY
	m_TrueSKYRender->Resize(pBackBufferSurfaceDesc);
	m_TrueSKYRender->SetPro(g_Projection);
#endif
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	mCamera.FrameMove(fElapsedTime);
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
	double fTime, float fElapsedTime, void* pUserContext)
{
	auto pRTV = DXUTGetD3D11RenderTargetView();
	auto pDSV = DXUTGetD3D11DepthStencilView();

	pd3dImmediateContext->ClearRenderTargetView(pRTV, Colors::MidnightBlue);
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_World = XMMatrixIdentity();
	//g_World = mCamera.GetWorldMatrix();
	g_View = mCamera.GetViewMatrix();
	g_Projection = mCamera.GetProjMatrix();
	s_Eye = mCamera.GetEyePt();
	XMFLOAT4 viewPos;
	XMStoreFloat4(&viewPos, s_Eye);

	pd3dImmediateContext->OMSetDepthStencilState(g_pOnDepthStencilState, 1);
#if TRUESKY
	m_TrueSKYRender->SetView(g_View);
	m_TrueSKYRender->SetPro(g_Projection);
	m_TrueSKYRender->PreRender(0, pd3dDevice, pd3dImmediateContext,pRTV,pDSV,&g_viewport);
#endif
	pd3dImmediateContext->OMSetRenderTargets(1, &g_pTestRenderTargetView, pDSV);
	pd3dImmediateContext->ClearRenderTargetView(g_pTestRenderTargetView, Colors::MidnightBlue);
#if LOAD_MODEL
	pd3dImmediateContext->RSSetState(g_pBackCCWRS);
	m_Model->SetWVP(g_World*g_View*g_Projection);
	m_Model->SetViewPos(viewPos);
	m_Model->Render(pd3dDevice, pd3dImmediateContext);
	pd3dImmediateContext->RSSetState(g_pBackCWRS);
#endif
	pd3dImmediateContext->OMSetRenderTargets(1, &pRTV, pDSV);
#if DEPTH_TEST
	pd3dImmediateContext->OMSetDepthStencilState(g_pOffDepthStencilState, 1);
	pd3dImmediateContext->ClearRenderTargetView(pRTV, Colors::MidnightBlue);
	//m_DepthTest->Render(pd3dDevice, pd3dImmediateContext, g_pTestSRV);
	m_DepthTest->Render(pd3dDevice, pd3dImmediateContext, g_pDepthSRV);
	pd3dImmediateContext->OMSetDepthStencilState(g_pOnDepthStencilState, 1);
#endif
#if TRUESKY
	m_TrueSKYRender->Render(g_pDepthStencilBuffer,g_pDepthSRV);
#endif

}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	SAFE_RELEASE(g_pTestTex);
	SAFE_RELEASE(g_pTestSRV);
	SAFE_RELEASE(g_pTestRenderTargetView);
	SAFE_RELEASE(g_pBackCCWRS);
	SAFE_RELEASE(g_pBackCWRS);
	SAFE_RELEASE(g_pDepthStencilView);
	SAFE_RELEASE(g_pOnDepthStencilState);
	SAFE_RELEASE(g_pOffDepthStencilState);
	SAFE_RELEASE(g_pDepthStencilBuffer);
	SAFE_RELEASE(g_pDepthSRV);
#if TRUESKY
	if (m_TrueSKYRender)
	{
		m_TrueSKYRender->Shutdown();
		delete m_TrueSKYRender;
		m_TrueSKYRender = nullptr;
	}
#endif

#if DEPTH_TEST
	if (m_DepthTest)
	{
		m_DepthTest->Shutdown();
		delete m_DepthTest;
		m_DepthTest = nullptr;
	}
#endif
#if LOAD_MODEL
	if (m_Model)
	{
		m_Model->Shutdown();
		delete m_Model;
		m_Model = nullptr;
	}
#endif
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	bool* pbNoFurtherProcessing, void* pUserContext)
{
	mCamera.HandleMessages(hWnd, uMsg, wParam, lParam);
	return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
	if (bKeyDown)
	{
		switch (nChar)
		{
		case VK_F1: // Change as needed                
			break;
		}
	}
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved(void* pUserContext)
{
	return true;
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	// Enable run-time memory check for debug builds.
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// DXUT will create and use the best device
	// that is available on the system depending on which D3D callbacks are set below

	// Set general DXUT callbacks
	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackKeyboard(OnKeyboard);
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
	DXUTSetCallbackDeviceRemoved(OnDeviceRemoved);

	// Set the D3D11 DXUT callbacks. Remove these sets if the app doesn't need to support D3D11
	DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable);
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
	DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);

	// Perform any application-level initialization here

	DXUTInit(true, true, nullptr); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings(true, true); // Show the cursor and clip it when in full screen
	DXUTCreateWindow(L"trueSKY");

	// Only require 10-level hardware or later
	DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, screen_width, screen_height);
	DXUTMainLoop(); // Enter into the DXUT render loop

	// Perform any application-level cleanup here

	return DXUTGetExitCode();
}
