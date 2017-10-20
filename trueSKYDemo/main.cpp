#include "DXUT.h"
#include "SDKmisc.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "DXUTCamera.h"

#include "./MicroProfile/microprofile.h"

#include "Model.h" 
#include "TrueSKYRender.h"
#include "UIContstants.h"
#include "RenderStates.h"

#include <map>
#include <sstream>
#include <fstream>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define LOAD_MODEL	1
#define TRUESKY		1
#define MICROPROFILE 0
//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager          g_DialogResourceManager;
CD3DSettingsDlg                     g_D3DSettingsDlg;       // Device settings dialog
CDXUTDialog							g_SkyParaUI;
CDXUTDialog                         g_CloudParaUI;
CDXUTDialog                         g_Cloud2DParaUI; 
CDXUTDialog                         g_SwitchUI; 

const char*							sqFileName[]={"./Resource/basic_setting_plus_windspeed.sq",
												"./Resource/blue_casual_sky_02.sq",
												"./Resource/newsequence_03.sq"};
const char*							sqFile=sqFileName[1];
float								m_EyeHeight = 13904.f;	// Unit:m 
float								m_ModelScaling = 10000.f;	
int									screen_width = 1280;
int									screen_height = 720;
D3DXMATRIX							g_World;
D3DXMATRIX							g_View;
D3DXMATRIX							g_Projection;
D3D11_VIEWPORT						g_viewport;
CFirstPersonCamera					mCamera;
D3DXVECTOR3							g_Eye( 10*m_ModelScaling, m_EyeHeight, 20*m_ModelScaling );
D3DXVECTOR3							g_At( 0.0f, m_EyeHeight, 20*m_ModelScaling );
ID3D11Device*						g_pd3dDevice = nullptr;
ID3D11DeviceContext*				g_pd3dImmediateContext = nullptr;
ID3D11RenderTargetView*				g_pRenderTargetView = nullptr;
ID3D11DepthStencilView*				g_pDepthStencilView = nullptr;
Model*								m_Model = nullptr;
TrueSKYRender*						m_TrueSKYRender = nullptr;
CameraSqFile*						m_CameraSqFile = nullptr;
DirectionalLight					g_DirectionalLight;
float								ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
bool								keydown[256];

bool								IsLoadSq = true;
bool								IsLoadModel = true;
bool								IsTrueSKY = true;
bool								IsProfile = false;
bool								IsCloud2D = true;
bool								IsCloud = true;
bool								IsSky = true;
bool								IsAnimation = false;

simul::sky::SkyKeyframer*			g_SkyKeyFramer = nullptr;
simul::clouds::CloudKeyframer*		g_CloudKeyFramer = nullptr;
simul::clouds::CloudKeyframer*		g_Cloud2DKeyFramer = nullptr;

simul::sky::SkyKeyframe*			g_SkyKeyFrame = nullptr;
simul::clouds::CloudKeyframe*		g_CloudKeyFrame = nullptr;
simul::clouds::CloudKeyframe*		g_Cloud2DKeyFrame = nullptr;

std::map<IDC_UI_ID,UIAttr>			UIAttrUsedMap;

int									frame_number = 0;
int									frame_refresh = 60;
int									animation_time_step = 10;

void InitApp();
void InitUI();
void UpdateMatrix();
void InitKeyFrameAttr();
void LoadCamera();
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
	
	V_RETURN(RenderStates::Initialize(pd3dDevice));

#if TRUESKY
	m_TrueSKYRender = new TrueSKYRender;
	if(IsLoadSq)
	{
		//Load Camera Info From Sq File.
		LoadCamera();
		m_TrueSKYRender->Initialize(sqFile);
	}
	else
		m_TrueSKYRender->Initialize();
	V_RETURN(m_TrueSKYRender->OnD3D11CreateDevice(g_pd3dDevice, g_pd3dImmediateContext));
	InitKeyFrameAttr();
#endif
	D3DXMatrixIdentity(&g_World);
	mCamera.SetViewParams(&g_Eye, &g_At);
	g_View = *(mCamera.GetViewMatrix());

	g_viewport.Width = static_cast<float>(screen_width);
	g_viewport.Height = static_cast<float>(screen_height);
	g_viewport.MinDepth = 0.0f;
	g_viewport.MaxDepth = 1.0f;
	g_viewport.TopLeftX = 0.0f;
	g_viewport.TopLeftY = 0.0f; 


#if LOAD_MODEL
	m_Model = new Model;
	m_Model->Initialize();
	m_Model->SetModelHeight(m_EyeHeight);
	V_RETURN(m_Model->OnD3D11CreateDevice(g_pd3dDevice, g_pd3dImmediateContext));
	m_Model->SetWVP(g_World*g_View*g_Projection);
#endif

#if MICROPROFILE
	MicroProfileOnThreadCreate("trueSKYDemo");
	//	MicroProfileSetForceEnable(true);
	MicroProfileSetEnableAllGroups(true);
	MicroProfileSetForceMetaCounters(true);

	MicroProfileGpuInitD3D11(g_pd3dDevice, g_pd3dImmediateContext);

	MICROPROFILE_GPU_SET_CONTEXT(g_pd3dImmediateContext, MicroProfileGetGlobalGpuThreadLog());
	MicroProfileStartContextSwitchTrace();

	char buffer[256];
	snprintf(buffer, sizeof(buffer)-1, "Webserver started in localhost:%d\n", MicroProfileWebServerPort());
	OutputDebugStringA(buffer);
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
	mCamera.FrameMove(fElapsedTime*1000.f);
	m_TrueSKYRender->OnFrameMove(fTime,fElapsedTime,keydown);
	{
		frame_number++;
		int new_time =  (int)(g_SkyKeyFramer->GetInterpolatedKeyframe()->daytime*24.0f);
		static int old_time = new_time;
		if(frame_number>=60 && old_time != new_time)
		{
			WCHAR sz[100];
			swprintf_s( sz, 100, L"Time: %d",new_time );
			g_SkyParaUI.GetStatic( IDC_SKY_TIME+IDC_STATIC_OFFSET )->SetText( sz );
			g_SkyParaUI.GetSlider( IDC_SKY_TIME )->SetValue(new_time);
			frame_number = 0;
		}
	}
	
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
								 double fTime, float fElapsedTime, void* pUserContext)
{
#if MICROPROFILE
	MICROPROFILE_SCOPEGPUI("Draw Total", 0xff00ff);
#endif
	if( g_D3DSettingsDlg.IsActive() )
	{
		g_D3DSettingsDlg.OnRender( fElapsedTime );
		return;
	}

	auto pRTV = DXUTGetD3D11RenderTargetView();
	auto pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearRenderTargetView(pRTV, ClearColor);
	pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );
	pd3dImmediateContext->OMSetDepthStencilState(RenderStates::OnDepthStencilState, 1);

	UpdateMatrix();

#if TRUESKY
	m_TrueSKYRender->SetViewPos(g_Eye);
	m_TrueSKYRender->SetView(g_View);
	m_TrueSKYRender->SetPro(g_Projection);
	{
#if MICROPROFILE
		MICROPROFILE_SCOPEGPUI("TrueSKY PreRender", 0x00ffff);
		MICROPROFILE_SCOPEI("Main","TrueSKY PreRender", 0x00ffff);
#endif
		m_TrueSKYRender->PreRender(0, pd3dDevice, pd3dImmediateContext,
			IsProfile,IsCloud2D,IsCloud,IsSky,IsAnimation);
		//g_DirectionalLight.Direction = D3DXVECTOR3(m_TrueSKYRender->GetLightDir())
	}
#endif
#if LOAD_MODEL
	if(IsLoadModel)
	{
#if MICROPROFILE
		MICROPROFILE_SCOPEGPUI("Render Model", 0xff00ff);
		MICROPROFILE_SCOPEI("Main","Render Model", 0xff00ff);
#endif
		//counter-clockwise
		pd3dImmediateContext->RSSetState(RenderStates::CullClockWiseRS);
		m_Model->SetWVP(g_World*g_View*g_Projection);
		m_Model->SetViewPos(g_Eye);
		m_Model->SetLight(&g_DirectionalLight);
		m_Model->Render(pd3dDevice, pd3dImmediateContext);
		pd3dImmediateContext->RSSetState(RenderStates::CullCounterClockWiseRS);
	}
#endif //LOAD_MODEL
#if TRUESKY
	{
#if MICROPROFILE
		MICROPROFILE_SCOPEGPUI("Render TrueSKY", 0xffff00);
		MICROPROFILE_SCOPEI("Main","Render TrueSKY", 0xffff00);
#endif
		m_TrueSKYRender->Render(IsTrueSKY,IsProfile);
	}
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

#if MICROPROFILE
	MicroProfileFlip(g_pd3dImmediateContext);
#endif
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

	RenderStates::Release();
	g_SkyKeyFramer = nullptr;
	g_CloudKeyFramer = nullptr;
	g_Cloud2DKeyFramer = nullptr;
	g_SkyKeyFrame = nullptr;
	g_CloudKeyFrame = nullptr;
	g_Cloud2DKeyFrame = nullptr;

	if(m_TrueSKYRender)
	{
		m_TrueSKYRender->Release();
		delete m_TrueSKYRender;
		m_TrueSKYRender = nullptr;
	}

	if(m_Model)
	{
		m_Model->Release();
		delete m_Model;
		m_Model = nullptr;
	}
#if MICROPROFILE
	MicroProfileShutdown();
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
			InitKeyFrameAttr();
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
	case IDC_LOAD_ANIMATION:
		{
			IsAnimation=IsAnimation?false:true;
			break;
		}
	default:
		{
			IDC_UI_ID id =(IDC_UI_ID)nControlID;
			UIAttr attr = UIAttrUsedMap[id];
			float val_f=0.f;
			int val_i=0;
			CDXUTDialog	*dialog = GetCDXUTDialog(id);
			if(id>IDC_SKY_FRAMER_START && id<IDC_SKY_FRAME_START)
			{
				if(strcmp(attr.name,"AnimationTimeStep")==0)
				{
					val_i = dialog->GetSlider( id )->GetValue();
#if TRUESKY
					m_TrueSKYRender->SetAnimationTimeStep(val_i);
#endif
				}
				else if(attr.IsFloat)
				{
					val_f = static_cast<float>(dialog->GetSlider( id )->GetValue()) / attr.scale_UI ;

					if(val_f == g_SkyKeyFramer->GetFloat(attr.name))
						break;
#if TRUESKY
					m_TrueSKYRender->UpdateSkyFramerFloatAttr(
						attr.name,val_f,
						attr.floor.floor_f,
						attr.ceil.ceil_f);
#endif
				}
			}
			else if(id>IDC_SKY_FRAME_START && id<IDC_CLOUD_FRAME_START)
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
			else if(id>IDC_CLOUD2D_FRAMER_START && id<IDC_ENV_LOAD_SQ)
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

	g_DirectionalLight.Direction = D3DXVECTOR3(-10.0f, -3.0f, -1.0f);
	g_DirectionalLight.Ambient = D3DXVECTOR4(0.05f, 0.05f, 0.05f, 1.0f);
	g_DirectionalLight.Diffuse = D3DXVECTOR4(0.70f, 0.70f, 0.70f, 1.0f);
	g_DirectionalLight.Specular = D3DXVECTOR4(0.70f, 0.70f, 0.70f, 0.70f);

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
	g_Eye = *(mCamera.GetEyePt());
}

void InitApp()
{
	// Initialize dialogs
	g_D3DSettingsDlg.Init( &g_DialogResourceManager );
	g_SkyParaUI.Init( &g_DialogResourceManager );
	g_CloudParaUI.Init( &g_DialogResourceManager );
	g_Cloud2DParaUI.Init( &g_DialogResourceManager );
	g_SwitchUI.Init( &g_DialogResourceManager );

	UIAttrUsedMap.emplace(IDC_SKY_FRAMER_START,UIAttr());
	UIAttrUsedMap.emplace(IDC_SKY_TIME,
		UIAttr("Time",0,24,24,0.f,1.f));
	UIAttrUsedMap.emplace(IDC_ANIMATION_TIME_STEP,
		UIAttr("AnimationTimeStep",1,1000,1,0.f,1.f));

	UIAttrUsedMap.emplace(IDC_CLOUD_FRAME_START,UIAttr());
	UIAttrUsedMap.emplace(IDC_CLOUD_CLOUDINESS,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::cloudiness],
		-100,100,100,0.f,1.f));// name,min_UI,min_UI,scale,floor,ceil
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
		-20000,20000,100,FLT_MIN,FLT_MAX));
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
	UIAttrUsedMap.emplace(IDC_CLOUD2D_UPPER_DENSITY,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::upperDensity],
		-100,100,100,0.f,1.f));
	UIAttrUsedMap.emplace(IDC_CLOUD2D_BASE_KM,
		UIAttr(TrueSKYRender::cloudFrameFloatAttrName[CloudFrameFloatAttr::cloudBase],
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
	g_SwitchUI.AddCheckBox( IDC_LOAD_ANIMATION, L"Load Animation", 0, iY += 26, 200, 22, IsAnimation );

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

		if(id == IDC_SKY_FRAME_START || id == IDC_SKY_FRAMER_START ||
			id == IDC_CLOUD_FRAME_START || id == IDC_CLOUD2D_FRAME_START)
		{
			dialog = GetCDXUTDialog(id);
			iY=0;
			continue;
		}
		else if(id>IDC_SKY_FRAMER_START && id<IDC_SKY_FRAME_START)
		{
			if(strcmp(attr.name,"Time")==0)
			{
				float time = g_SkyKeyFramer->GetInterpolatedKeyframe()->daytime;
				value = (int)(time*attr.scale_UI);
			}
			else if(strcmp(attr.name,"AnimationTimeStep")==0)
			{
				value = animation_time_step;
			}
			else if(attr.IsFloat)
				value = (int)(g_SkyKeyFramer->GetFloat(attr.name)*attr.scale_UI);
			else
				value = g_SkyKeyFramer->GetInt(attr.name)*attr.scale_UI;
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
	if(id>=IDC_SKY_FRAMER_START && id<IDC_CLOUD_FRAME_START)
		return &g_SkyParaUI;
	else if(id>=IDC_CLOUD_FRAME_START && id<IDC_CLOUD2D_FRAME_START)
		return &g_CloudParaUI;
	else
		return &g_Cloud2DParaUI;
}

void InitKeyFrameAttr()
{
	m_TrueSKYRender->InitKeyFrameAttr();

	g_SkyKeyFrame = m_TrueSKYRender->GetSkyFrameAttr();
	g_CloudKeyFrame = m_TrueSKYRender->GetCloudFrameAttr();
	g_Cloud2DKeyFrame = m_TrueSKYRender->GetCloud2DFrameAttr();

	g_SkyKeyFramer = m_TrueSKYRender->GetSkyFramerAttr();
	g_CloudKeyFramer = m_TrueSKYRender->GetCloudFramerAttr();
	g_Cloud2DKeyFramer = m_TrueSKYRender->GetCloud2DFramerAttr();
}

void LoadCamera()
{
	std::ifstream fin(sqFile);
	std::string ignore;
	fin>>ignore;
	if(ignore=="\"camera\":")
	{
		m_CameraSqFile = new CameraSqFile;
		fin>>ignore;
		for(int i =0;i<3;i++)
		{
			std::string v;
			fin>>ignore>>v;
			v = v.substr(1,v.size()-2);
			if(ignore == "\"FieldOfViewDegrees\":")
			{
				m_CameraSqFile->FieldOfViewDegrees = std::stof(v);
			}
			else if(ignore =="\"PosKm\":")
			{
				char* s = strtok(const_cast<char*>(v.c_str()),",");
				float* p = m_CameraSqFile->PosKm;
				while(s)
				{
					if(p-m_CameraSqFile->PosKm>=3)
					{
						printf("error");
						break;
					}
					*p++=stof(std::string(s));
					s=strtok(nullptr,",");
				}
			}
			else if(ignore =="\"Quaternion\":")
			{
				char* s = strtok(const_cast<char*>(v.c_str()),",");
				float* p = m_CameraSqFile->Quaternion;
				while(s)
				{
					if(p-m_CameraSqFile->Quaternion>=3)
					{
						printf("error");
						break;
					}
					*p++=stof(std::string(s));
					s=strtok(nullptr,",");
				}
			}
		}
		D3DXMATRIX change(	-1.f, 0.f, 0.f, 0.f,
		0.f, 0.f, -1.f, 0.f,
		0.f, -1.f, 0.f, 0.f,
		0.f, 0.f, 0.f, 1.f);

		D3DXMATRIX rotateZ;
		D3DXMatrixRotationZ(&rotateZ,D3DX_PI);
		g_Eye=m_CameraSqFile->PosKm;
		g_Eye*=1000;
		D3DXVECTOR4 out;
		D3DXVec3Transform(&out,&g_Eye,&change);
		D3DXVec4Transform(&out,&out,&rotateZ);
		g_Eye=D3DXVECTOR3(out);
		m_EyeHeight= g_Eye.y;

		g_Eye=D3DXVECTOR3(10*m_ModelScaling,m_EyeHeight,20*m_ModelScaling);
		g_At= D3DXVECTOR3(0.f,m_EyeHeight,20*m_ModelScaling);

		//D3DXVECTOR3 look = g_At - g_Eye;
		//D3DXQUATERNION q(m_CameraSqFile->Quaternion[1],m_CameraSqFile->Quaternion[2],m_CameraSqFile->Quaternion[3],m_CameraSqFile->Quaternion[0]);
		//D3DXMATRIX q_matrix;
		//D3DXMatrixRotationQuaternion(&q_matrix,&q);
		//D3DXVec3Transform(&out,&look,&q_matrix);

		//g_At = g_Eye + D3DXVECTOR3(out);
	}
	fin.close();
}