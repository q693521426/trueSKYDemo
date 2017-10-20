#include "DXUT.h"
#include "TrueSKYRender.h"
#include "./MicroProfile/microprofile.h"
//#pragma optimize("",off)

#pragma region AttrName
char* TrueSKYRender::cloudFramerIntAttrName[]=
{
	"GridWidth"	,		
	"GridHeight",			
	"Use3DNoise",			
	"Wrap",
	"NoiseResolution",		
	"OverrideWind",		
	"EdgeNoiseOctaves",	
	"EdgeNoiseFrequency",
	"EdgeNoiseTextureSize",
	"ExplicitOffsets",		
	"ShadowTextureSize",	
	"GodraysSteps",		
	"DefaultNumSlices",	
	"Visible",				
	"MeetHorizon"
};

char* TrueSKYRender::cloudFrameFloatAttrName[]=
{
	"cloudiness",				
	"distributionBaseLayer",	
	"distributionTransition",		
	"upperDensity",				
	"localDensity",				
	"windSpeed",					
	"windDirection",				
	"persistence",					
	"cloudBase",					
	"cloudHeight",					
	"directLight",					
	"indirectLight",				
	"ambientLight",				
	"lightAsymmetry",				
	"precipitation",				
	"fractalWavelength",			
	"fractalAmplitude",			
	"edgeSharpness",				
	"churn",						
	"extinction",					
	"rainToSnow",					
	"precipitationWindEffect",		
	"precipitationWaver",			
	"diffusivity",					
	"cache_built",					
	"lightning",					
	"godrayStrength",				
	"baseNoiseFactor",				
	"offsetx",						
	"offsety",						
	"rainCentreXKm",				
	"rainCentreYKm",				
	"rainRadiusKm",				
	"rainEffectStrength",			
	"simulation",					
	"worleyNoise",					
	"worleyScale"				
};
#pragma endregion 

TrueSKYRender::TrueSKYRender():
	reverseDepth(false),
	env(nullptr),
	weatherRenderer(nullptr),
	hdrFramebuffer(nullptr),
	hDRRenderer(nullptr),
	m_FrameBuffer(nullptr),
	m_Renderer(nullptr),
	m_SkyFrameAttr(nullptr),
	m_CloudFrameAttr(nullptr),
	m_Cloud2DFrameAttr(nullptr),
	skyKeyFramer(nullptr),
	cloudKeyFramer(nullptr),
	cloud2DKeyFramer(nullptr),
	m_pRenderTargetView(nullptr),
	m_pDenthStencilView(nullptr),
	m_pViewPort(nullptr),
	time_refresh(0.1f),
	time_step(10)
{
}

TrueSKYRender::~TrueSKYRender()
{
}

#pragma region Initiliaze
bool TrueSKYRender::Initialize(const char* sq)
{
	if(!InitEnv(sq))
		return false;

	InitKeyFrameAttr();
	InitKeyFrameArray();

	weatherRenderer = new simul::clouds::BaseWeatherRenderer(env, NULL);

#if SIMUL_HDR
	hDRRenderer = new simul::crossplatform::HdrRenderer();
	hdrFramebuffer = renderPlatformDx11.CreateFramebuffer();
	hdrFramebuffer->SetFormat(simul::crossplatform::RGBA_16_FLOAT);
	hdrFramebuffer->SetDepthFormat(simul::crossplatform::D_32_FLOAT);
#else
	m_Renderer = new FullScreenQuad();
	m_FrameBuffer = new FrameBuffer();
	m_FrameBuffer->SetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_FrameBuffer->SetDepthFormat(DXGI_FORMAT_D32_FLOAT);
#endif

#if SIMUL_CAMERA
	camera.SetPositionAsXYZ(200.0f, 20.0f, 2200.f);
	float look[] = { 0.f,1.f,0.f }, up[] = { 0.f,0.f,1.f };
	camera.LookInDirection(look, up);
	camera.SetHorizontalFieldOfViewDegrees(90.f);
	camera.SetVerticalFieldOfViewDegrees(0.f);
#if 0
	crossplatform::CameraViewStruct vs;
	vs.exposure = 1.f;
	vs.farZ = 300000.f;
	vs.nearZ = 1.f;
	vs.gamma = 0.44f;
	vs.InfiniteFarPlane = false;
	vs.projection = crossplatform::DEPTH_REVERSE;
	camera.SetCameraViewStruct(vs);
#endif
#endif

	renderPlatformDx11.PushShaderPath("Platform/DirectX11/HLSL");
	renderPlatformDx11.PushShaderPath("Platform/CrossPlatform/SFX");
	renderPlatformDx11.PushTexturePath("Media/Textures");
	renderPlatformDx11.SetShaderBinaryPath("shaderbin");

	return true;
}

bool TrueSKYRender::ReLoadSq(const char* sq)
{
	if(!InitEnv(sq))
		return false;

	del(weatherRenderer,nullptr);
	weatherRenderer = new simul::clouds::BaseWeatherRenderer(env, NULL);

	InitKeyFrameAttr();
	InitKeyFrameArray();

	weatherRenderer->RestoreDeviceObjects(&renderPlatformDx11);

	return true;
}

bool TrueSKYRender::InitEnv(const char* sq)
{
	simul::base::SetLicence(SIMUL_LICENSE_KEY);
	del(env,nullptr);
	env = new simul::clouds::Environment();
	if (sq)
	{
		simul::crossplatform::TextFileInput ifs;
		ifs.Load(sq);

		if (ifs.Good())
		{
			env->LoadFromText(ifs);
		}
		else
		{
			return false;
		}
	}
	else
	{
		env->cloudKeyframer->AddStorm(0.0f,1.0f,vec2(0.f,0.f),100.0f);
		env->cloudKeyframer->InsertKeyframe(0.5f)->cloudiness=0.75f;
	}
	return true;
}

void TrueSKYRender::InitKeyFrameArray()
{
	skyKeyFramerArray.clear();
	//skyKeyFramer->SetLinkKeyframeTimeAndDaytime(true);
	for(int i = 0; i < skyKeyFramer->GetNumKeyframes();i++)
	{
		simul::sky::SkyKeyframe *k=(simul::sky::SkyKeyframe*)skyKeyFramer->GetKeyframe(i);
		skyKeyFramerArray.push_back(*k);
	}
	
	cloudKeyFramerArray.clear();
	for(int i = 0; i < cloudKeyFramer->GetNumKeyframes();i++)
	{
		simul::clouds::CloudKeyframe *k=(simul::clouds::CloudKeyframe*)cloudKeyFramer->GetKeyframe(i);
		cloudKeyFramerArray.push_back(*k);
	}

//	cloudKeyFramer->SetExplicitOffsets(true);
	cloud2DKeyFramerArray.clear();
	for(int i =0; i < cloud2DKeyFramer->GetNumKeyframes();i++)
	{
		simul::clouds::CloudKeyframe *k=(simul::clouds::CloudKeyframe*)cloud2DKeyFramer->GetKeyframe(i);
		cloud2DKeyFramerArray.push_back(*k);
	}

}

void TrueSKYRender::InitKeyFrameAttr()
{
	del(m_SkyFrameAttr,nullptr);
	del(m_CloudFrameAttr,nullptr);
	del(m_Cloud2DFrameAttr,nullptr);

	m_SkyFrameAttr=new simul::sky::SkyKeyframe();
	m_CloudFrameAttr=new simul::clouds::CloudKeyframe();
	m_Cloud2DFrameAttr=new simul::clouds::CloudKeyframe();

	for(int i=0;i<cloudFloatAttrSize;i++)
	{
		m_CloudFrameAttr->SetFloat(cloudFrameFloatAttrName[i],0.f);
		m_Cloud2DFrameAttr->SetFloat(cloudFrameFloatAttrName[i],0.f);
	}

	skyKeyFramer=env->skyKeyframer;
	cloudKeyFramer=env->cloudKeyframer;
	cloud2DKeyFramer=env->cloud2DKeyframer;
}
#pragma endregion

#pragma region Release
void TrueSKYRender::OnD3D11LostDevice()
{
	weatherRenderer->InvalidateDeviceObjects();
	renderPlatformDx11.InvalidateDeviceObjects();
#if SIMUL_HDR
	hDRRenderer->InvalidateDeviceObjects();
	hdrFramebuffer->InvalidateDeviceObjects();
#endif
}

void TrueSKYRender::Release()
{
	OnD3D11LostDevice();
	del(weatherRenderer, nullptr);
	cloudKeyFramer=nullptr;
	cloud2DKeyFramer=nullptr;
	skyKeyFramer=nullptr;
	m_pRenderTargetView=nullptr;
	m_pDenthStencilView=nullptr;
	m_pViewPort=nullptr;
	del(env, nullptr);
	del(m_SkyFrameAttr,nullptr);
	del(m_CloudFrameAttr,nullptr);
	del(m_Cloud2DFrameAttr,nullptr);
#if SIMUL_HDR
	del(hdrFramebuffer, nullptr);
	del(hDRRenderer, nullptr);
#else
	if(m_Renderer)
	{
		m_Renderer->Release();
		delete m_Renderer;
		m_Renderer = nullptr;
	}
	if(m_FrameBuffer)
	{
		m_FrameBuffer->Release();
		delete m_FrameBuffer;
		m_FrameBuffer = nullptr;
	}
#endif

}
#pragma endregion

HRESULT TrueSKYRender::OnD3D11CreateDevice(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	HRESULT hr = S_OK;
	renderPlatformDx11.RestoreDeviceObjects(pd3dDevice);
	weatherRenderer->RestoreDeviceObjects(&renderPlatformDx11);

#if SIMUL_HDR
	hDRRenderer->RestoreDeviceObjects(&renderPlatformDx11);
	hdrFramebuffer->RestoreDeviceObjects(&renderPlatformDx11);
#else
	V_RETURN(m_Renderer->OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
	V_RETURN(m_FrameBuffer->OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
#endif
	return hr;
}

void TrueSKYRender::IsRender(bool Is2DCloud,bool IsCloud,bool IsSky)
{
	//skyKeyFramer->ClearKeyframes();
	if(Is2DCloud)
	{
		if(cloud2DKeyFramer->GetNumKeyframes()==0)
		{
			for(int i = 0 ;i<static_cast<int>(cloud2DKeyFramerArray.size());i++)
			{
				cloud2DKeyFramer->InsertKeyframe(cloud2DKeyFramerArray[i].time);
				*(cloud2DKeyFramer->GetKeyframe(i))=cloud2DKeyFramerArray[i];
			}
		}
	}
	else
	{
		if(cloud2DKeyFramer->GetNumKeyframes()>0)
			cloud2DKeyFramer->ClearKeyframes();
	}
	if(IsCloud)
	{
		if(cloudKeyFramer->GetNumKeyframes()==0)
		{
			for(int i = 0 ;i<static_cast<int>(cloudKeyFramerArray.size());i++)
			{
				cloudKeyFramer->InsertKeyframe(cloudKeyFramerArray[i].time);
				*(cloudKeyFramer->GetKeyframe(i))=cloudKeyFramerArray[i];
			}
		}
	}
	else
	{
		if(cloudKeyFramer->GetNumKeyframes()>0)
			cloudKeyFramer->ClearKeyframes();
	}
}

void TrueSKYRender::PreRender(int view_id, ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
							  bool IsProfile,bool Is2DCloud,bool IsCloud,bool IsSky,bool IsAnimation)
{
	deviceContext.platform_context = pd3dImmediateContext;
	deviceContext.renderPlatform = &renderPlatformDx11;
	deviceContext.viewStruct.view_id = view_id;
	deviceContext.viewStruct.depthTextureStyle = simul::crossplatform::PROJECTION;
	if(IsProfile)
	{
		simul::crossplatform::SetGpuProfilingInterface(deviceContext, renderPlatformDx11.GetGpuProfiler());
		renderPlatformDx11.GetGpuProfiler()->StartFrame(deviceContext);
	}

	viewport = renderPlatformDx11.GetViewport(deviceContext, 0);
	
#if SIMUL_CAMERA
	float aspect = (float)viewport.w / (float)viewport.h;
	if (reverseDepth)
		deviceContext.viewStruct.proj = camera.MakeDepthReversedProjectionMatrix(aspect);
	else
		deviceContext.viewStruct.proj = camera.MakeProjectionMatrix(aspect);
	deviceContext.viewStruct.view = camera.MakeViewMatrix();
#else	
	deviceContext.viewStruct.view = simul::math::Matrix4x4(m_View);
	deviceContext.viewStruct.proj = simul::math::Matrix4x4(m_Projection);
#endif
	// MUST call this after modifying a deviceContext.
	deviceContext.viewStruct.Init();

	env->Update();
	
	IsRender(Is2DCloud,IsCloud,IsSky);

	static simul::base::Timer timer;
	static float old_time = timer.UpdateTimeSum() / 1000.0f; 
	float real_time = timer.UpdateTimeSum() / 1000.0f;
	
	if(IsAnimation)
	{
		skyKeyFramer->GetNextModifiableKeyframe();
		skyKeyFramer->GetInterpolatedKeyframe()->daytime += (real_time - old_time) * time_step / 24.0f / 3600.0f;
		if(skyKeyFramer->GetInterpolatedKeyframe()->daytime>=1.f)
			skyKeyFramer->GetInterpolatedKeyframe()->daytime-=1.0f;
		skyKeyFramer->Update();
	}
	old_time = real_time;

	{
#if MICROPROFILE
		MICROPROFILE_SCOPEI("TrueSKY PreRender","BaseWeather PreRender Update",0x00ffff);
#endif
		//weatherRenderer->EnableSky(false);
		//weatherRenderer->SetShowSky(false);
		//weatherRenderer->SetShowAtmospherics(false);
		//weatherRenderer->SetShow2DCloudTextures(false);
		weatherRenderer->PreRenderUpdate(deviceContext, real_time);
	}
#if SIMUL_HDR
	hdrFramebuffer->Activate(deviceContext);
	hdrFramebuffer->ActivateDepth(deviceContext);
	hdrFramebuffer->Clear(deviceContext, 0.f, 0.f, 0.f, 0.f, reverseDepth ? 0.f : 1.f);
#else
	m_FrameBuffer->Activate();
	m_FrameBuffer->ActivateDepth(true);
#endif
}

void TrueSKYRender::Render(bool IsTrueSKY,bool IsProfile)
{
	simul::dx11::Texture depthTexture;
#if SIMUL_HDR
	hdrFramebuffer->DeactivateDepth(deviceContext);
	depthTexture.InitFromExternalD3D11Texture2D(&renderPlatformDx11, hdrFramebuffer->GetDepthTexture()->AsD3D11Texture2D(), hdrFramebuffer->GetDepthTexture()->AsD3D11ShaderResourceView());
#else
	m_FrameBuffer->DeactivateDepth();
	depthTexture.InitFromExternalD3D11Texture2D(&renderPlatformDx11, m_FrameBuffer->GetDepthResource(), m_FrameBuffer->GetDepthSRV());
#endif

#if SIMUL_HDR
	hdrFramebuffer->Activate(deviceContext);
	hdrFramebuffer->DeactivateDepth(deviceContext);
	hdrFramebuffer->Clear(deviceContext, 0.f, 0.f, 0.f, 0.f, reverseDepth ? 0.f : 1.f);
#else
	simul::crossplatform::BaseFramebuffer::setDefaultRenderTargets(m_FrameBuffer->GetRenderTargetView(),
		nullptr,
		static_cast<uint32_t>(m_pViewPort->TopLeftX),
		static_cast<uint32_t>(m_pViewPort->TopLeftY),
		static_cast<uint32_t>(m_pViewPort->TopLeftX + m_pViewPort->Width),
		static_cast<uint32_t>(m_pViewPort->TopLeftY + m_pViewPort->Height)
	);
#endif

	if(IsTrueSKY)
		weatherRenderer->Render(deviceContext, simul::clouds::TrueSkyRenderMode::STANDARD, 1.0f, 1.0f, &depthTexture, nullptr, &viewport);

#if SIMUL_HDR
	hdrFramebuffer->Deactivate(deviceContext);
	hDRRenderer->Render(deviceContext, hdrFramebuffer->GetTexture(), 1.0f, 1.0f);
#else
	simul::dx11::Texture tex;
	tex.InitFromExternalD3D11Texture2D(&renderPlatformDx11, m_FrameBuffer->GetFrameBufferResource(), m_FrameBuffer->GetFrameBufferSRV());
	m_Renderer->Render(m_pRenderTargetView,m_FrameBuffer->GetFrameBufferSRV());
	
	simul::crossplatform::BaseFramebuffer::setDefaultRenderTargets(m_pRenderTargetView,
		nullptr,
		static_cast<uint32_t>(m_pViewPort->TopLeftX),
		static_cast<uint32_t>(m_pViewPort->TopLeftY),
		static_cast<uint32_t>(m_pViewPort->TopLeftX + m_pViewPort->Width),
		static_cast<uint32_t>(m_pViewPort->TopLeftY + m_pViewPort->Height)
	);
#endif
	// Optional debug displays:
	if(IsProfile)
	{
		weatherRenderer->SetShowCloudCrossSections(true);
		weatherRenderer->SetShowCompositing(true);
		weatherRenderer->RenderOverlays(deviceContext);
		renderPlatformDx11.GetGpuProfiler()->EndFrame(deviceContext);
		//const char *txt = simul::dx11::Profiler::GetGlobalProfiler().GetDebugText(simul::base::PLAINTEXT);
		//renderPlatformDx11.Print(deviceContext, 16, 16, txt,vec4(1,1,1,1),vec4(0,0,0,.5f));
		//renderPlatformDx11.DrawDepth(deviceContext, 32, 32, 64, 64, hdrFramebuffer->GetDepthTexture());
	}
}

void TrueSKYRender::Resize(ID3D11RenderTargetView* pRTV,
							   ID3D11DepthStencilView*	pDSV,
							   D3D11_VIEWPORT* pViewPort,
							   const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
	int W = pBackBufferSurfaceDesc->Width;
	int H = pBackBufferSurfaceDesc->Height;

	m_pRenderTargetView = pRTV;
	m_pDenthStencilView = pDSV;
	m_pViewPort = pViewPort;
	simul::crossplatform::BaseFramebuffer::setDefaultRenderTargets(m_pRenderTargetView,
		m_pDenthStencilView,
		static_cast<uint32_t>(m_pViewPort->TopLeftX),
		static_cast<uint32_t>(m_pViewPort->TopLeftY),
		static_cast<uint32_t>(m_pViewPort->TopLeftX + m_pViewPort->Width),
		static_cast<uint32_t>(m_pViewPort->TopLeftY + m_pViewPort->Height)
	);

#if SIMUL_HDR
	hdrFramebuffer->SetWidthAndHeight(W, H);
	hdrFramebuffer->SetAntialiasing(1);
	hDRRenderer->SetBufferSize(W, H);
#else 
	m_Renderer->Resize(W,H);
	m_FrameBuffer->Resize(W,H);
#endif

}

void TrueSKYRender::RecompileShaders()
{
	renderPlatformDx11.RecompileShaders();
	weatherRenderer->RecompileShaders();
#if SIMUL_HDR
	hDRRenderer->RecompileShaders();
#endif
}

bool TrueSKYRender::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch (uMsg)
	{
#if SIMUL_CAMERA
	case WM_MOUSEWHEEL:
		{
			int xPos = GET_X_LPARAM(lParam); 
			int yPos = GET_Y_LPARAM(lParam); 
			//short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			OnMouse((wParam&MK_LBUTTON)!=0
				,(wParam&MK_RBUTTON)!=0
				,(wParam&MK_MBUTTON)!=0
				,0,xPos,yPos);
			break;
		}
	case WM_MOUSEMOVE:
		{
			int xPos = GET_X_LPARAM(lParam); 
			int yPos = GET_Y_LPARAM(lParam); 
			OnMouse((wParam&MK_LBUTTON)!=0
				,(wParam&MK_RBUTTON)!=0
				,(wParam&MK_MBUTTON)!=0
				,0,xPos,yPos);
			break;
		}
#endif
	default:
		break;
	}
	return true;
}

void TrueSKYRender::OnMouse(bool bLeftButtonDown
			,bool bRightButtonDown
			,bool bMiddleButtonDown
            ,int nMouseWheelDelta
            ,int xPos
			,int yPos )
{
		mouseCameraInput.MouseButtons
			=(bLeftButtonDown?simul::crossplatform::MouseCameraInput::LEFT_BUTTON:0)
			|(bRightButtonDown?simul::crossplatform::MouseCameraInput::RIGHT_BUTTON:0)
			|(bMiddleButtonDown?simul::crossplatform::MouseCameraInput::MIDDLE_BUTTON:0);
		mouseCameraInput.MouseX=xPos;
		mouseCameraInput.MouseY=yPos;
}

void TrueSKYRender::OnFrameMove(double fTime,float fElapsedTime,bool* keydown)
{
#if SIMUL_CAMERA
	mouseCameraInput.forward_back_input	=(float)keydown['w']-(float)keydown['s'];
	mouseCameraInput.right_left_input	=(float)keydown['d']-(float)keydown['a'];
	mouseCameraInput.up_down_input		=(float)keydown['t']-(float)keydown['g'];
	
	simul::crossplatform::UpdateMouseCamera(&camera
							,time_step
							,1000.f
							,mouseCameraState
							,mouseCameraInput
							,10000.f);
#endif
}

#pragma region Ser & Get
simul::sky::SkyKeyframe* TrueSKYRender::GetSkyFrameAttr()
{
	return m_SkyFrameAttr;
}

void TrueSKYRender::UpdateSkyFrameFloatAttr(const char* name,float value,float min,float max)
{
	skyKeyFramer->GetNextModifiableKeyframe();
	for(int i=0;i<(int)skyKeyFramerArray.size();i++)
	{
		simul::sky::SkyKeyframe *k=(simul::sky::SkyKeyframe *)skyKeyFramer->GetKeyframe(i);
		float data = skyKeyFramerArray[i].GetFloat(name) + value;
		data = clamp(data,min,max);
		k->SetFloat(name,data);
	}
	skyKeyFramer->Update();
}	

simul::sky::SkyKeyframer* TrueSKYRender::GetSkyFramerAttr()
{
	return skyKeyFramer;
}

void TrueSKYRender::UpdateSkyFramerFloatAttr(const char* name,float value,float min,float max)
{
	skyKeyFramer->GetNextModifiableKeyframe();
	float data = clamp(value,min,max);
	if(strcmp(name,"Time")==0)
	{
		skyKeyFramer->GetInterpolatedKeyframe()->daytime = value;
	}
	else
	{
		skyKeyFramer->SetFloat(name,data);	
	}
	skyKeyFramer->Update();
}	

simul::clouds::CloudKeyframe* TrueSKYRender::GetCloudFrameAttr()
{
	return m_CloudFrameAttr;
}

void TrueSKYRender::UpdateCloudFrameFloatAttr(const char* name,float value,float min,float max)
{
	cloudKeyFramer->GetNextModifiableKeyframe();
	for(int i=0;i<(int)cloudKeyFramerArray.size();i++)
	{
		simul::clouds::CloudKeyframe *k=(simul::clouds::CloudKeyframe *)cloudKeyFramer->GetKeyframe(i);
		float data = cloudKeyFramerArray[i].GetFloat(name) + value;
		data = clamp(data,min,max);
		k->SetFloat(name,data);
	}
	cloudKeyFramer->Update(0.005);
}

void TrueSKYRender::UpdateCloudFramerIntAttr(const char* name,int value,int min,int max)
{
	cloudKeyFramer->GetNextModifiableKeyframe();
	int data = clamp(value,min,max);
	cloudKeyFramer->SetInt(name,data);
	cloudKeyFramer->Update(0.005);
}

simul::clouds::CloudKeyframe* TrueSKYRender::GetCloud2DFrameAttr()
{
	return m_Cloud2DFrameAttr;
}

void TrueSKYRender::UpdateCloud2DFrameFloatAttr(const char* name,float value,float min,float max)
{
	cloud2DKeyFramer->GetNextModifiableKeyframe();
	for(int i=0;i<(int)cloud2DKeyFramerArray.size();i++)
	{
		simul::clouds::CloudKeyframe *k=(simul::clouds::CloudKeyframe *)cloud2DKeyFramer->GetKeyframe(i);
		float data = cloud2DKeyFramerArray[i].GetFloat(name) + value;
		data = clamp(data,min,max);
		k->SetFloat(name,data);
	}
	cloud2DKeyFramer->Update(0.005);
}

void TrueSKYRender::UpdateCloud2DFramerIntAttr(const char* name,int value,int min,int max)
{
	cloud2DKeyFramer->GetNextModifiableKeyframe();
	int data = clamp(value,min,max);
	cloud2DKeyFramer->SetInt(name,data);
	cloud2DKeyFramer->Update(0.005);
}

simul::clouds::CloudKeyframer* TrueSKYRender::GetCloudFramerAttr()
{
	return cloudKeyFramer;
}

simul::clouds::CloudKeyframer* TrueSKYRender::GetCloud2DFramerAttr()
{
	return cloud2DKeyFramer;
}

void TrueSKYRender::SetWorld(const D3DXMATRIX& world)
{
	m_World = world;
}

//should call SetviewPos() before
void TrueSKYRender::SetView(const D3DXMATRIX& view)
{
	D3DXMATRIX change(	-1.f, 0.f, 0.f, 0.f,
		0.f, 0.f, -1.f, 0.f,
		0.f, -1.f, 0.f, 0.f,
		0.f, 0.f, 0.f, 1.f);

	D3DXMATRIX rotateZ;
	D3DXMatrixRotationZ(&rotateZ,D3DX_PI);

	m_View = change * view * rotateZ;

	D3DXMATRIX trans(	1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		-m_ViewPos.x , -m_ViewPos.y , -m_ViewPos.z , 1.f);
	for(int i=0;i<3;i++)
		m_View.m[3][i]=0;
	m_View = trans * m_View;
}

void TrueSKYRender::SetPro(const D3DXMATRIX& pro)
{
	m_Projection = pro;
	m_Projection._33 = -m_Projection._33;
	m_Projection._34 = -m_Projection._34;
}

void TrueSKYRender::SetViewPos(const D3DXVECTOR3& viewPos)
{
	m_ViewPos = D3DXVECTOR3(viewPos.x,viewPos.z,viewPos.y);
}

void TrueSKYRender::SetAnimationTimeStep(const int t)
{
	time_step = t;
}

D3DXVECTOR3 TrueSKYRender::GetLightDir()
{
	//skyKeyFramer->GetInterpolatedKeyframe()->
	return D3DXVECTOR3();
}
#pragma endregion
