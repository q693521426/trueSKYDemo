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

#include "TrueSKYRenderer.h"
#include "CFBXRendererDX11.h"

#pragma warning( disable : 4100 )

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};

struct CBChangesEveryFrame
{
	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;
};


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
const DWORD	NUMBER_OF_MODELS = 1;
const int	screen_width = 1280;
const int	screen_height = 720;
const uint32_t g_InstanceMAX = 32;
simul::clouds::Environment*			environment = nullptr;
ID3D11VertexShader*         g_pModelVertexShader = nullptr;
ID3D11VertexShader*         g_pModelInstancingVertexShader = nullptr;
ID3D11PixelShader*          g_pModelPixelShader = nullptr;
ID3D11InputLayout*          g_pVertexLayout = nullptr;
ID3D11Buffer*               g_pVertexBuffer = nullptr;
ID3D11Buffer*               g_pIndexBuffer = nullptr;
ID3D11Buffer*               g_pCBChangesEveryFrame = nullptr;
ID3D11ShaderResourceView*   g_pTextureRV = nullptr;
ID3D11SamplerState*         g_pSamplerLinear = nullptr;
XMMATRIX                    g_World;
XMMATRIX                    g_View;
XMMATRIX                    g_Projection;
XMFLOAT4                    g_vMeshColor(0.7f, 0.7f, 0.7f, 1.0f);
D3D11_VIEWPORT				g_viewport;

ID3D11Buffer*					g_pTransformStructuredBuffer = nullptr;
ID3D11ShaderResourceView*		g_pTransformSRV = nullptr;
struct SRVPerInstanceData
{
	XMMATRIX mWorld;
};
FBX_LOADER::CFBXRenderDX11*	g_pFbxDX11[NUMBER_OF_MODELS];
char						g_files[NUMBER_OF_MODELS][256] =
{
	"sulan2.fbx"
	//"Assets\\model1.fbx"
};

simul::clouds::Environment*			env = nullptr;
TrueSKYRenderer*					trueSKYRenderer = nullptr;
simul::dx11::RenderPlatform			renderPlatformDx11;
simul::clouds::BaseWeatherRenderer*	weatherRenderer = nullptr;
bool								IsTrueSKY = false;


void SetModelInstancingMatrix(ID3D11DeviceContext* g_pImmediateContext)
{
	HRESULT hr = S_OK;
	const uint32_t count = g_InstanceMAX;
	const float offset = -(g_InstanceMAX*60.0f / 2.0f);
	XMMATRIX mat;

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	hr = g_pImmediateContext->Map(g_pTransformStructuredBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	SRVPerInstanceData*	pSrvInstanceData = (SRVPerInstanceData*)MappedResource.pData;

	for (uint32_t i = 0; i<count; i++)
	{
		mat = XMMatrixTranspose(XMMatrixTranslation(0, 0, i*60.0f + offset));
		pSrvInstanceData[i].mWorld = (mat);
	}

	g_pImmediateContext->Unmap(g_pTransformStructuredBuffer, 0);
}

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

	for (DWORD i = 0; i < NUMBER_OF_MODELS; i++)
	{
		g_pFbxDX11[i] = new FBX_LOADER::CFBXRenderDX11;
		hr = g_pFbxDX11[i]->LoadFBX(g_files[i], pd3dDevice, pd3dImmediateContext);
	}
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"FBX Error", L"Error", MB_OK);
		return hr;
	}

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	V_RETURN(DXUTCompileFromFile(L"ModelVS.hlsl", nullptr, "VS", "vs_4_0", dwShaderFlags, 0, &pVSBlob));

	hr = pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pModelVertexShader);
	if (FAILED(hr))
	{
		SAFE_RELEASE(pVSBlob);
		return hr;
	}

	V_RETURN(DXUTCompileFromFile(L"ModelInstancingVS.hlsl", nullptr, "VS", "vs_4_0", dwShaderFlags, 0, &pVSBlob));

	hr = pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pModelInstancingVertexShader);
	if (FAILED(hr))
	{
		SAFE_RELEASE(pVSBlob);
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	for (DWORD i = 0; i < NUMBER_OF_MODELS; ++i)
	{
		hr = g_pFbxDX11[i]->CreateInputLayout(pd3dDevice, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), layout, numElements);
	}
	SAFE_RELEASE(pVSBlob);
	if (FAILED(hr))
		return hr;

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	V_RETURN(DXUTCompileFromFile(L"ModelPS.hlsl", nullptr, "PS", "ps_4_0", dwShaderFlags, 0, &pPSBlob));

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pModelPixelShader);
	SAFE_RELEASE(pPSBlob);
	if (FAILED(hr))
		return hr;

	// Create the constant buffers
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = sizeof(CBChangesEveryFrame);
	V_RETURN(pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBChangesEveryFrame));

	const uint32_t count = g_InstanceMAX;
	const uint32_t stride = static_cast<uint32_t>(sizeof(SRVPerInstanceData));
	bd.ByteWidth = stride * count;
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = stride;
	V_RETURN(pd3dDevice->CreateBuffer(&bd, nullptr, &g_pTransformStructuredBuffer));

	// Initialize the world matrices
	g_World = XMMatrixIdentity();

	// Initialize the view matrix
	static const XMVECTORF32 s_Eye = { 0.0f, 3.0f, -100.f, 0.f };
	static const XMVECTORF32 s_At = { 0.0f, 1.0f, 0.0f, 0.f };
	static const XMVECTORF32 s_Up = { 0.0f, 1.0f, 0.0f, 0.f };
	g_View = XMMatrixLookAtLH(s_Eye, s_At, s_Up);

	// Create ShaderResourceView
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;   
	srvDesc.BufferEx.FirstElement = 0;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.BufferEx.NumElements = count;                 
				
	V_RETURN(pd3dDevice->CreateShaderResourceView(g_pTransformStructuredBuffer, &srvDesc, &g_pTransformSRV));

	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	V_RETURN(pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear));

	g_viewport.Width = screen_width;
	g_viewport.Height = screen_height;
	g_viewport.MinDepth = 1.0f;
	g_viewport.MaxDepth = 0.0f;
	g_viewport.TopLeftX = 0.0f;
	g_viewport.TopLeftY = 0.0f;

	if (IsTrueSKY)
	{
		simul::base::SetLicence(SIMUL_LICENSE_KEY);
		env = new simul::clouds::Environment();
		simul::crossplatform::TextFileInput ifs;
		ifs.Load("./basic_setting.sq");
		if (ifs.Good())
		{
			env->LoadFromText(ifs);
		}

		trueSKYRenderer = new TrueSKYRenderer(env);
		trueSKYRenderer->OnD3D11CreateDevice(pd3dDevice);
	}

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	// Setup the projection parameters
	int W = pBackBufferSurfaceDesc->Width;
	int H = pBackBufferSurfaceDesc->Height;
	g_viewport.Width = W;
	g_viewport.Height = H;
	float fAspect = static_cast<float>(W) / static_cast<float>(H);
	g_Projection = XMMatrixPerspectiveFovLH(XM_PI * 0.25f, fAspect, 0.1f, 100.0f);
	if(IsTrueSKY)
		trueSKYRenderer->ResizeView(W,H);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	//	g_World = XMMatrixRotationY(60.0f * XMConvertToRadians((float)fTime));
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
	double fTime, float fElapsedTime, void* pUserContext)
{
	if(IsTrueSKY)
		env->Update();
	auto pRTV = DXUTGetD3D11RenderTargetView();
	auto pDSV = DXUTGetD3D11DepthStencilView();

	pd3dImmediateContext->ClearRenderTargetView(pRTV, Colors::MidnightBlue);
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
	if (IsTrueSKY)
		trueSKYRenderer->Render(pd3dDevice, pd3dImmediateContext,pRTV,pDSV,&g_viewport);
	   
	for (DWORD i = 0; i < NUMBER_OF_MODELS; i++)
	{
		size_t nodeCount = g_pFbxDX11[i]->GetNodeCount();

		pd3dImmediateContext->VSSetShader(g_pModelInstancingVertexShader, NULL, 0);
		pd3dImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBChangesEveryFrame);
		pd3dImmediateContext->PSSetShader(g_pModelPixelShader, NULL, 0);

		for (int j = 0; j < nodeCount; j++)
		{
			XMMATRIX mLocal;
			g_pFbxDX11[i]->GetNodeMatrix(j, &mLocal.r[0].m128_f32[0]);	

			HRESULT hr;
			D3D11_MAPPED_SUBRESOURCE MappedResource;
			V(pd3dImmediateContext->Map(g_pCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
			auto pCB = reinterpret_cast<CBChangesEveryFrame*>(MappedResource.pData);
			XMStoreFloat4x4(&pCB->mWorld, XMMatrixTranspose(g_World));
			XMStoreFloat4x4(&pCB->mView, XMMatrixTranspose(g_View));
			XMStoreFloat4x4(&pCB->mProj, XMMatrixTranspose(g_Projection));
			pd3dImmediateContext->Unmap(g_pCBChangesEveryFrame, 0);

			SetModelInstancingMatrix(pd3dImmediateContext);

			FBX_LOADER::MATERIAL_DATA material = g_pFbxDX11[i]->GetNodeMaterial(j);

			if (material.pMaterialCb)
				pd3dImmediateContext->UpdateSubresource(material.pMaterialCb, 0, NULL, &material.materialConstantData, 0, 0);

			pd3dImmediateContext->VSSetShaderResources(0, 1, &g_pTransformSRV);
			pd3dImmediateContext->PSSetShaderResources(0, 1, &material.pSRV);
			pd3dImmediateContext->PSSetConstantBuffers(0, 1, &material.pMaterialCb);
			pd3dImmediateContext->PSSetSamplers(0, 1, &material.pSampler);

			//g_pFbxDX11[i]->RenderNode(pd3dImmediateContext, j);
			g_pFbxDX11[i]->RenderNodeInstancing(pd3dImmediateContext, j, g_InstanceMAX);
		}
	}
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
	SAFE_RELEASE(g_pVertexBuffer);
	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_pVertexLayout);
	SAFE_RELEASE(g_pTextureRV);
	SAFE_RELEASE(g_pModelVertexShader);
	SAFE_RELEASE(g_pModelInstancingVertexShader);
	SAFE_RELEASE(g_pModelPixelShader);
	SAFE_RELEASE(g_pCBChangesEveryFrame);
	SAFE_RELEASE(g_pSamplerLinear);
	for (DWORD i = 0; i < NUMBER_OF_MODELS; ++i)
	{
		if (g_pFbxDX11[i])
		{
			delete g_pFbxDX11[i];
			g_pFbxDX11[i] = nullptr;
		}
	}
	delete env;
	delete trueSKYRenderer;
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	bool* pbNoFurtherProcessing, void* pUserContext)
{
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
	simul::base::SetLicence(SIMUL_LICENSE_KEY);
	env = new simul::clouds::Environment();
	simul::crossplatform::TextFileInput ifs;
	ifs.Load("./RollingStormClouds.sq");
	if (ifs.Good())
	{
		env->LoadFromText(ifs);
	}
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
