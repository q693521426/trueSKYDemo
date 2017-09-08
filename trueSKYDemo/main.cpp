#include "DXUT.h"
#include "SDKmisc.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "DXUTCamera.h"

#include "Model.h" 
#include "TrueSKYRender.h"
#include "FullScreenQuad.h"
#include "FrameBuffer.h"
#include "UIContstants.h"

#include <map>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#define LOAD_MODEL	1
#define TRUESKY		1
//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------

CDXUTDialogResourceManager          g_DialogResourceManager;
CD3DSettingsDlg                     g_D3DSettingsDlg;       // Device settings dialog
CDXUTDialog							g_SkyParaUI;
CDXUTDialog                         g_CloudParaUI;
CDXUTDialog                         g_Cloud2DParaUI; 
CDXUTDialog                         g_SwitchUI; 


const char							sqFile[]="./Resource/basic_setting_plus_windspeed.sq";
const float							m_EyeHeight = 2100.f;	// Unit:m 
const float							m_ModelScaling = 100.f;	
int									screen_width = 1280;
int									screen_height = 720;
D3DXMATRIX							g_World;
D3DXMATRIX							g_View;
D3DXMATRIX							g_Projection;
D3D11_VIEWPORT						g_viewport;
CFirstPersonCamera					mCamera;
D3DXVECTOR3							s_Eye( 10*m_ModelScaling, m_EyeHeight, 20*m_ModelScaling );
D3DXVECTOR3							s_At( 0.0f, m_EyeHeight, 20*m_ModelScaling );
D3DXVECTOR3							s_Up( 0.0f, 0.0f, 1.0f );
ID3D11Device*						g_pd3dDevice = nullptr;
ID3D11DeviceContext*				g_pd3dImmediateContext = nullptr;
ID3D11RenderTargetView*				g_pRenderTargetView = nullptr;
ID3D11DepthStencilView*				g_pDepthStencilView = nullptr;
ID3D11RasterizerState*				g_pBackCCWRS = nullptr;
ID3D11RasterizerState*				g_pBackCWRS = nullptr;
ID3D11DepthStencilState*			g_pOnDepthStencilState = nullptr;
ID3D11DepthStencilState*			g_pOffDepthStencilState = nullptr;
Model*								m_Model = nullptr;
TrueSKYRender*						m_TrueSKYRender = nullptr;
LightBuffer							g_Light;
D3DXVECTOR4							viewPos;
float								ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
bool								keydown[256];

bool								IsLoadSq = true;
bool								IsLoadModel = true;
bool								IsTrueSKY = true;
bool								IsProfile = false;
bool								IsCloud2D = true;
bool								IsCloud = true;
bool								IsSky = true;

simul::sky::SkyKeyframer*			g_SkyKeyFramer = nullptr;
simul::clouds::CloudKeyframer*		g_CloudKeyFramer = nullptr;
simul::clouds::CloudKeyframer*		g_Cloud2DKeyFramer = nullptr;

simul::sky::SkyKeyframe*			g_SkyKeyFrame = nullptr;
simul::clouds::CloudKeyframe*		g_CloudKeyFrame = nullptr;
simul::clouds::CloudKeyframe*		g_Cloud2DKeyFrame = nullptr;

std::map<IDC_UI_ID,UIAttr>					UIAttrUsedMap;

void InitApp();
void InitUI();
void UpdateMatrix();
void UpdateKeyFrameAttr();
CDXUTDialog* GetCDXUTDialog(IDC_UI_ID);
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

	g_pd3dDevice = pd3dDevice;
	g_pd3dImmediateContext = DXUTGetD3D11DeviceContext();

	V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, g_pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );

	D3DXMatrixIdentity(&g_World);
	mCamera.SetViewParams(&s_Eye, &s_At);
	g_View = *(mCamera.GetViewMatrix());

	g_viewport.Width = static_cast<float>(screen_width);
	g_viewport.Height = static_cast<float>(screen_height);
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
	g_pd3dDevice->CreateRasterizerState(&rasterDesc, &g_pBackCCWRS);

	rasterDesc.FrontCounterClockwise = false;
	g_pd3dDevice->CreateRasterizerState(&rasterDesc, &g_pBackCWRS);

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

	V_RETURN(g_pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &g_pOnDepthStencilState));

	depthStencilDesc.DepthEnable = false;
	V_RETURN(g_pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &g_pOffDepthStencilState));

#if LOAD_MODEL
	m_Model = new Model;
	m_Model->Initialize();
	V_RETURN(m_Model->OnD3D11CreateDevice(g_pd3dDevice, g_pd3dImmediateContext));
	m_Model->SetWVP(g_World*g_View*g_Projection);
#endif

#if TRUESKY
	m_TrueSKYRender = new TrueSKYRender;
	if(IsLoadSq)
		m_TrueSKYRender->Initialize(sqFile);
	else
		m_TrueSKYRender->Initialize();
	V_RETURN(m_TrueSKYRender->OnD3D11CreateDevice(g_pd3dDevice, g_pd3dImmediateContext));

	UpdateKeyFrameAttr();
#endif
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	HRESULT hr;
	screen_width = pBackBufferSurfaceDesc->Width;
	screen_height = pBackBufferSurfaceDesc->Height;
	g_viewport.Width = static_cast<float>(screen_width);
	g_viewport.Height = static_cast<float>(screen_height);
	float fAspect = g_viewport.Width / g_viewport.Height;
	mCamera.SetProjParams(D3DX_PI * 0.25f, fAspect, 0.1f, 100.f*m_ModelScaling);
	g_Projection = *(mCamera.GetProjMatrix());

	g_pRenderTargetView = DXUTGetD3D11RenderTargetView();
	g_pDepthStencilView = DXUTGetD3D11DepthStencilView();

#if LOAD_MODEL
	m_Model->Resize(pBackBufferSurfaceDesc);
	m_Model->SetWVP(g_World*g_View*g_Projection);
#endif
#if TRUESKY
	m_TrueSKYRender->Resize(g_pRenderTargetView,g_pDepthStencilView,&g_viewport,pBackBufferSurfaceDesc);
	m_TrueSKYRender->SetPro(g_Projection);
#endif

	V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

	g_SkyParaUI.SetLocation(pBackBufferSurfaceDesc->Width - 600, 0 );
	g_SkyParaUI.SetSize(200,300);

    g_CloudParaUI.SetLocation( pBackBufferSurfaceDesc->Width - 400, 0 );
    g_CloudParaUI.SetSize( 200, 300 );

	g_Cloud2DParaUI.SetLocation( pBackBufferSurfaceDesc->Width - 200, 0 );
    g_Cloud2DParaUI.SetSize( 200, 300 );

	g_SwitchUI.SetLocation( 0, 0 );
    g_SwitchUI.SetSize( 200, 300 );
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	mCamera.FrameMove(fElapsedTime*100.f);
	m_TrueSKYRender->OnFrameMove(fTime,fElapsedTime,keydown);
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
	double fTime, float fElapsedTime, void* pUserContext)
{
	if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

	auto pRTV = DXUTGetD3D11RenderTargetView();
	//auto pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearRenderTargetView(pRTV, ClearColor);
	pd3dImmediateContext->OMSetDepthStencilState(g_pOnDepthStencilState, 1);

	UpdateMatrix();

#if TRUESKY
	D3DXMATRIX change(	-1.f, 0.f, 0.f, 0.f,
						0.f, 0.f, -1.f, 0.f,
						0.f, -1.f, 0.f, 0.f,
						0.f, 0.f, 0.f, 1.f);
	
	D3DXMATRIX rotateZ,rotateX,rotateY;
	D3DXMatrixRotationZ(&rotateZ,D3DX_PI);
	D3DXMatrixRotationX(&rotateX,D3DX_PI);
	D3DXMatrixRotationY(&rotateY,D3DX_PI);

	D3DXMATRIX view =  g_View;

	view = change * view * rotateZ;
	
	D3DXVECTOR3 eye = *(mCamera.GetEyePt());
	D3DXMATRIX trans(	1.f, 0.f, 0.f, 0.f,
						0.f, 1.f, 0.f, 0.f,
						0.f, 0.f, 1.f, 0.f,
						-eye.x , -eye.z , -eye.y , 1.f);
	for(int i=0;i<3;i++)
		view.m[3][i]=0;
	view = trans * view;

	m_TrueSKYRender->SetView(view);
	m_TrueSKYRender->SetPro(g_Projection);
	m_TrueSKYRender->PreRender(0, pd3dDevice, pd3dImmediateContext,IsSky);
	//g_Light.LightPos = m_TrueSKYRender->GetDirToSun();
#endif
#if LOAD_MODEL
	if(IsLoadModel)
	{
		//counter-clockwise
		pd3dImmediateContext->OMSetDepthStencilState(g_pOnDepthStencilState, 1);
		pd3dImmediateContext->RSSetState(g_pBackCCWRS);
		m_Model->SetWVP(g_World*g_View*g_Projection);
		m_Model->SetViewPos(viewPos);
		m_Model->SetLight(&g_Light);
		m_Model->Render(pd3dDevice, pd3dImmediateContext);
		pd3dImmediateContext->RSSetState(g_pBackCWRS);
	}
#endif //LOAD_MODEL
#if TRUESKY
	pd3dImmediateContext->OMSetDepthStencilState(g_pOffDepthStencilState, 1);
	m_TrueSKYRender->Render(IsTrueSKY,IsProfile,IsCloud2D,IsCloud,IsSky);
	pd3dImmediateContext->OMSetDepthStencilState(g_pOnDepthStencilState, 1);
#endif //TRUESKY
	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_SkyParaUI.OnRender( fElapsedTime );
	g_CloudParaUI.OnRender( fElapsedTime );
	g_Cloud2DParaUI.OnRender( fElapsedTime );
	g_SwitchUI.OnRender( fElapsedTime ); 
    DXUT_EndPerfEvent();

	WCHAR str[256] = L"trueSKY:";
	wcscat_s(str,DXUTGetFrameStats( DXUTIsVsyncEnabled()));
	SetWindowText(DXUTGetHWND(),str);
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
	g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
	SAFE_RELEASE(g_pBackCCWRS);
	SAFE_RELEASE(g_pBackCWRS);
	SAFE_RELEASE(g_pOnDepthStencilState);
	SAFE_RELEASE(g_pOffDepthStencilState);
#if TRUESKY
	if(m_TrueSKYRender)
	{
		m_TrueSKYRender->Release();
		delete m_TrueSKYRender;
		m_TrueSKYRender = nullptr;
	}
#endif

#if LOAD_MODEL
	if(m_Model)
	{
		m_Model->Release();
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
	 // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

	*pbNoFurtherProcessing = g_SkyParaUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
	
	*pbNoFurtherProcessing = g_CloudParaUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

	*pbNoFurtherProcessing = g_Cloud2DParaUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
	
	*pbNoFurtherProcessing = g_SwitchUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

	mCamera.HandleMessages(hWnd, uMsg, wParam, lParam);
	m_TrueSKYRender->MsgProc(hWnd, uMsg, wParam, lParam);
	return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
	switch (nChar)
	{
		case VK_LEFT: 
		case VK_RIGHT: 
		case VK_UP: 
		case VK_DOWN:
			break;
		case 'R':
			m_TrueSKYRender->RecompileShaders();
			break;
		default: 
			int  k=tolower(nChar);
			if(k>255)
				return;
			keydown[k]=bKeyDown?1:0;
		break; 
	}
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
	WCHAR sz[100];
	WCHAR m_wstr[100];

	swprintf_s(m_wstr,100,L"");
	switch( nControlID )
    {
		case IDC_ENV_LOAD_SQ:
		{
#if TRUESKY
			if(IsLoadSq)
			{
				m_TrueSKYRender->ReLoadSq();	
				IsLoadSq=false;
			}
			else
			{
				m_TrueSKYRender->ReLoadSq(sqFile);	
				IsLoadSq=true;
			}	
			UpdateKeyFrameAttr();
#endif
			break;
		}
		case IDC_TRUE_SKY:
		{
			IsTrueSKY=IsTrueSKY?false:true;
			break;
		}
		case IDC_LOAD_MODEL:
		{
			IsLoadModel=IsLoadModel?false:true;
			break;
		}
		case IDC_TRUESKY_PROFILE:
		{
			IsProfile=IsProfile?false:true;
			break;
		}
		case IDC_LOAD_CLOUD2D:
		{
			IsCloud2D=IsCloud2D?false:true;
			break;
		}
		case IDC_LOAD_CLOUD:
		{
			IsCloud=IsCloud?false:true;
			break;
		}
		case IDC_LOAD_SKY:
		{
			IsSky=IsSky?false:true;
			break;
		}
		default:
		{
			IDC_UI_ID id =(IDC_UI_ID)nControlID;
			UIAttr attr = UIAttrUsedMap[id];
			float val_f=0.f;
			int val_i=0;
			CDXUTDialog	*dialog = GetCDXUTDialog(id);
			if(id>IDC_SKY_FRAME_START && id<IDC_CLOUD_FRAME_START)
			{
				if(attr.IsFloat)
				{
					val_f = static_cast<float>(dialog->GetSlider( id )->GetValue()) / attr.scale_UI ;

					if(val_f == g_SkyKeyFrame->GetFloat(attr.name))
						break;
					g_SkyKeyFrame->SetFloat(attr.name,val_f);
#if TRUESKY
					m_TrueSKYRender->UpdateSkyFrameFloatAttr(
						attr.name,val_f,
						attr.floor.floor_f,
						attr.ceil.ceil_f);
#endif
				}
			}
			else if(id>IDC_CLOUD_FRAME_START && id<IDC_CLOUD_FRAMER_START)
			{
				if(attr.IsFloat)
				{
					val_f = static_cast<float>(dialog->GetSlider( id )->GetValue()) / attr.scale_UI ;

					if(val_f == g_CloudKeyFrame->GetFloat(attr.name))
						break;
					g_CloudKeyFrame->SetFloat(attr.name,val_f);
#if TRUESKY
					m_TrueSKYRender->UpdateCloudFrameFloatAttr(
						attr.name,val_f,
						attr.floor.floor_f,
						attr.ceil.ceil_f);
#endif
				}
			}
			else if(id>IDC_CLOUD_FRAMER_START && id<IDC_CLOUD2D_FRAME_START)
			{
				if(!attr.IsFloat)
				{
					val_i = dialog->GetSlider( id )->GetValue() / attr.scale_UI ;

					if(val_i == g_CloudKeyFramer->GetFloat(attr.name))
						break;
					g_CloudKeyFramer->SetInt(attr.name,val_i);
#if TRUESKY
					m_TrueSKYRender->UpdateCloudFramerIntAttr(
						attr.name,val_i,
						attr.floor.floor_i,
						attr.ceil.ceil_i);
#endif
				}
			}
			else if(id>IDC_CLOUD2D_FRAME_START && id<IDC_CLOUD2D_FRAMER_START)
			{
				if(attr.IsFloat)
				{
					val_f = static_cast<float>(dialog->GetSlider( id )->GetValue()) / attr.scale_UI ;

					if(val_f == g_Cloud2DKeyFrame->GetFloat(attr.name))
						break;
					g_Cloud2DKeyFrame->SetFloat(attr.name,val_f);
#if TRUESKY
					m_TrueSKYRender->UpdateCloud2DFrameFloatAttr(
						attr.name,val_f,
						attr.floor.floor_f,
						attr.ceil.ceil_f);
#endif
				}
				swprintf_s(m_wstr,100,L"2D");
			}
			else if(id>IDC_CLOUD2D_FRAMER_START)
			{
				if(!attr.IsFloat)
				{
					val_i = dialog->GetSlider( id )->GetValue() / attr.scale_UI ;

					if(val_i == g_Cloud2DKeyFramer->GetFloat(attr.name))
						break;
					g_Cloud2DKeyFramer->SetInt(attr.name,val_i);
#if TRUESKY
					m_TrueSKYRender->UpdateCloud2DFramerIntAttr(
						attr.name,val_i,
						attr.floor.floor_i,
						attr.ceil.ceil_i);
#endif
				}
				swprintf_s(m_wstr,100,L"2D");
			}
			int value =(val_i==0)?(int)(val_f*attr.scale_UI):(val_i*attr.scale_UI);
			std::wstringstream wss;
			wss<<attr.name;
			swprintf_s( sz, 100, L"%s%s: %d",wss.str().c_str() ,m_wstr,value );
			dialog->GetStatic( id+IDC_STATIC_OFFSET )->SetText( sz );
			dialog = nullptr;
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

	memset(keydown,0,sizeof(keydown));

	g_Light.LightPos = D3DXVECTOR4(10.0f, 3.0f, 1.0f, 1.0f);
	g_Light.LightColor = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	g_Light.Ambient = D3DXVECTOR4(0.05f, 0.05f, 0.05f, 1.0f);
	g_Light.Diffuse = D3DXVECTOR4(0.70f, 0.70f, 0.70f, 1.0f);
	g_Light.Specular = D3DXVECTOR4(0.70f, 0.70f, 0.70f, 0.70f);
	g_Light.Constant = 1.0f;
	g_Light.Linear = 0.009f;
	g_Light.Quadratic = 0.0032f;

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

	InitApp();
	DXUTInit(true, true, nullptr); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings(true, true); // Show the cursor and clip it when in full screen
	DXUTCreateWindow(L"trueSKY");

	// Only require 10-level hardware or later
	DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, screen_width, screen_height);
	
	InitUI();

	DXUTMainLoop(); // Enter into the DXUT render loop

	// Perform any application-level cleanup here

	return DXUTGetExitCode();
}

void UpdateMatrix()
{
	D3DXMatrixIdentity(&g_World);
	g_View = *(mCamera.GetViewMatrix());
	g_Projection = *(mCamera.GetProjMatrix());
	s_Eye = *(mCamera.GetEyePt());
	viewPos = D3DXVECTOR4(s_Eye);
	viewPos.w = 1.0f;
}

void InitApp()
{
	 // Initialize dialogs
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
	g_SkyParaUI.Init( &g_DialogResourceManager );
	g_CloudParaUI.Init( &g_DialogResourceManager );
	g_Cloud2DParaUI.Init( &g_DialogResourceManager );
	g_SwitchUI.Init( &g_DialogResourceManager );

	UIAttrUsedMap.emplace(IDC_CLOUD_FRAME_START,UIAttr());
	UIAttrUsedMap.emplace(IDC_CLOUD_CLOUDINESS,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::cloudiness],
							-100,100,100,0.f,1.f));
	UIAttrUsedMap.emplace(IDC_CLOUD_DISTRIBUTION_BASE_LAYER,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::distributionBaseLayer],
							-100,100,100,0.f,1.f));
	UIAttrUsedMap.emplace(IDC_CLOUD_DISTRIBUTION_TRANSITION,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::distributionTransition],
							-100,100,100,0.f,1.f));
	UIAttrUsedMap.emplace(IDC_CLOUD_UPPER_DENSITY,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::upperDensity],
							-100,100,100,0.f,1.f));
	UIAttrUsedMap.emplace(IDC_CLOUD_WIND_SPEED,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::windSpeed],
							-2000,2000,2000,FLT_MIN,FLT_MAX));
	UIAttrUsedMap.emplace(IDC_CLOUD_BASE_KM,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::cloudBase],
							-2000,2000,2000,FLT_MIN,FLT_MAX));
	UIAttrUsedMap.emplace(IDC_CLOUD_HEIGHT_KM,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::cloudHeight],
							-2000,2000,2000,FLT_MIN,FLT_MAX));
	UIAttrUsedMap.emplace(IDC_CLOUD_EDGE_NOISE_OCTAVES,
		UIAttr(TrueSKYRender::cloudFramerIntAttrName[CloudFramerIntAttr::EdgeNoiseOctaves],
							0,4,1,0,4));
	UIAttrUsedMap.emplace(IDC_CLOUD_EDGE_NOISE_FREQUENCY,
		UIAttr(TrueSKYRender::cloudFramerIntAttrName[CloudFramerIntAttr::EdgeNoiseFrequency],
							0,64,1,0,64));
	UIAttrUsedMap.emplace(IDC_CLOUD_EDGE_NOISE_TEXTURE_SIZE,
		UIAttr(TrueSKYRender::cloudFramerIntAttrName[CloudFramerIntAttr::EdgeNoiseTextureSize],
							0,64,1,0,64));

	UIAttrUsedMap.emplace(IDC_CLOUD2D_FRAME_START,UIAttr());
	UIAttrUsedMap.emplace(IDC_CLOUD2D_CLOUDINESS,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::cloudiness],
							-100,100,100,0.f,1.f));
	UIAttrUsedMap.emplace(IDC_CLOUD2D_DISTRIBUTION_BASE_LAYER,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::distributionBaseLayer],
							-100,100,100,0.f,1.f));
	UIAttrUsedMap.emplace(IDC_CLOUD2D_DISTRIBUTION_TRANSITION,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::distributionTransition],
							-100,100,100,0.f,1.f));
	UIAttrUsedMap.emplace(IDC_CLOUD2D_UPPER_DENSITY,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::upperDensity],
							-100,100,100,0.f,1.f));
	UIAttrUsedMap.emplace(IDC_CLOUD2D_WIND_SPEED,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::windSpeed],
							-2000,2000,2000,FLT_MIN,FLT_MAX));
	UIAttrUsedMap.emplace(IDC_CLOUD2D_BASE_KM,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::cloudBase],
							-2000,2000,2000,FLT_MIN,FLT_MAX));
	UIAttrUsedMap.emplace(IDC_CLOUD2D_HEIGHT_KM,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::cloudHeight],
							-2000,2000,2000,FLT_MIN,FLT_MAX));
	UIAttrUsedMap.emplace(IDC_CLOUD2D_EDGE_NOISE_OCTAVES,
		UIAttr(TrueSKYRender::cloudFramerIntAttrName[CloudFramerIntAttr::EdgeNoiseOctaves],
							0,4,1,0,4));
	UIAttrUsedMap.emplace(IDC_CLOUD2D_EDGE_NOISE_FREQUENCY,
		UIAttr(TrueSKYRender::cloudFramerIntAttrName[CloudFramerIntAttr::EdgeNoiseFrequency],
							0,64,1,0,64));
	UIAttrUsedMap.emplace(IDC_CLOUD2D_EDGE_NOISE_TEXTURE_SIZE,
		UIAttr(TrueSKYRender::cloudFramerIntAttrName[CloudFramerIntAttr::EdgeNoiseTextureSize],
							0,64,1,0,64));

}

void InitUI()
{
	int iY = 0;
	WCHAR sz[100];
	WCHAR m_wstr[100];
	swprintf_s( m_wstr, 100, L"");

	g_SwitchUI.SetCallback( OnGUIEvent );
	g_SwitchUI.AddButton( IDC_ENV_LOAD_SQ, L"Load Sky File (F1)", 0, iY += 26, 200, 22, VK_F1 );
	g_SwitchUI.AddButton( IDC_LOAD_MODEL, L"Load Model File (F2)", 0, iY += 26, 200, 22, VK_F2 );
	g_SwitchUI.AddButton( IDC_TRUESKY_PROFILE, L"Load Profile (F3)", 0, iY += 26, 200, 22, VK_F3 );
	g_SwitchUI.AddButton( IDC_LOAD_CLOUD2D, L"Load Cloud2D (F4)", 0, iY += 26, 200, 22, VK_F4 );
	g_SwitchUI.AddButton( IDC_LOAD_CLOUD, L"Load Cloud (F5)", 0, iY += 26, 200, 22, VK_F5 );

	g_SkyParaUI.SetCallback( OnGUIEvent );
	g_CloudParaUI.SetCallback( OnGUIEvent );
	g_Cloud2DParaUI.SetCallback( OnGUIEvent );

	iY=0;
	CDXUTDialog* dialog=nullptr;
	for(auto i=UIAttrUsedMap.begin();i!=UIAttrUsedMap.end();i++)
	{
		IDC_UI_ID id=i->first;
		UIAttr attr=i->second;
		int value=0;

		if(id == IDC_SKY_FRAME_START ||id == IDC_CLOUD_FRAME_START ||id == IDC_CLOUD2D_FRAME_START)
		{
			dialog = GetCDXUTDialog(id);
			iY=0;
			continue;
		}
		else if(id>IDC_SKY_FRAME_START && id<IDC_CLOUD_FRAME_START)
		{
			if(attr.IsFloat)
				value = (int)(g_SkyKeyFrame->GetFloat(attr.name)*attr.scale_UI);
			else
				value = g_SkyKeyFrame->GetInt(attr.name)*attr.scale_UI;
		}
		else if(id>IDC_CLOUD_FRAME_START && id<IDC_CLOUD_FRAMER_START)
		{
			if(attr.IsFloat)
				value = (int)(g_CloudKeyFrame->GetFloat(attr.name)*attr.scale_UI);
			else
				value = g_CloudKeyFrame->GetInt(attr.name)*attr.scale_UI;
		}
		else if(id>IDC_CLOUD_FRAMER_START && id<IDC_CLOUD2D_FRAME_START)
		{
			if(attr.IsFloat)
				value = (int)(g_CloudKeyFramer->GetFloat(attr.name)*attr.scale_UI);
			else
				value = g_CloudKeyFramer->GetInt(attr.name)*attr.scale_UI;
		}
		else if(id>IDC_CLOUD2D_FRAME_START && id<IDC_CLOUD2D_FRAMER_START)
		{
			if(attr.IsFloat)
				value = (int)(g_Cloud2DKeyFrame->GetFloat(attr.name)*attr.scale_UI);
			else
				value = g_Cloud2DKeyFrame->GetInt(attr.name)*attr.scale_UI;
			swprintf_s( m_wstr, 100, L"2D");
		}
		else if(id>IDC_CLOUD2D_FRAMER_START)
		{
			if(attr.IsFloat)
				value = (int)(g_Cloud2DKeyFramer->GetFloat(attr.name)*attr.scale_UI);
			else
				value = g_Cloud2DKeyFramer->GetInt(attr.name)*attr.scale_UI;
			swprintf_s( m_wstr, 100, L"2D");
		}
		std::wstringstream wss;
		wss<<i->second.name;
		swprintf_s( sz, 100, L"%s%s: %d",wss.str().c_str() ,m_wstr, value);
		dialog->AddStatic( i->first+IDC_STATIC_OFFSET, sz, 0, iY += 26, 200, 22 );
		dialog->AddSlider( i->first, 0, iY += 26, 100, 22, attr.min_UI, attr.max_UI,value );
	}
	dialog=nullptr;
}

CDXUTDialog* GetCDXUTDialog(IDC_UI_ID id)
{
	if(id>=IDC_SKY_FRAME_START && id<IDC_CLOUD_FRAME_START)
		return &g_SkyParaUI;
	else if(id>=IDC_CLOUD_FRAME_START && id<IDC_CLOUD2D_FRAME_START)
		return &g_CloudParaUI;
	else
		return &g_Cloud2DParaUI;
}

void UpdateKeyFrameAttr()
{
	m_TrueSKYRender->UpdateKeyFrameAttr();

	g_SkyKeyFrame = m_TrueSKYRender->GetSkyFrameAttr();
	g_CloudKeyFrame = m_TrueSKYRender->GetCloudFrameAttr();
	g_Cloud2DKeyFrame = m_TrueSKYRender->GetCloud2DFrameAttr();

	g_SkyKeyFramer = m_TrueSKYRender->GetSkyFramerAttr();
	g_CloudKeyFramer = m_TrueSKYRender->GetCloudFramerAttr();
	g_Cloud2DKeyFramer = m_TrueSKYRender->GetCloud2DFramerAttr();
}