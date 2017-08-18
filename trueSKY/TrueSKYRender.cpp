#include "DXUT.h"
#include "TrueSKYRender.h"


TrueSKYRender::TrueSKYRender()
{
}


TrueSKYRender::~TrueSKYRender()
{
}

bool TrueSKYRender::Initialize(char* sq = nullptr)
{
	simul::base::SetLicence(SIMUL_LICENSE_KEY);
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
	auto skf = env->skyKeyframer;
	simul::sky::float4 dir = skf->GetDirectionToSun();

	weatherRenderer = new simul::clouds::BaseWeatherRenderer(env, NULL);

#if SIMUL_HDR
	hDRRenderer = new simul::crossplatform::HdrRenderer();
	hdrFramebuffer = renderPlatformDx11.CreateFramebuffer();
	hdrFramebuffer->SetFormat(simul::crossplatform::RGBA_16_FLOAT);
	hdrFramebuffer->SetDepthFormat(simul::crossplatform::D_32_FLOAT);
#endif

#if SIMUL_CAMERA
	camera.SetPositionAsXYZ(0, 0, 2200.f);
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
	// Shader binaries: we want to use a shared common directory under Simul/Media. But if we're running from some other place, we'll just create a "shaderbin" directory.
	//std::string simul_env=simul::base::EnvironmentVariables::GetSimulEnvironmentVariable("SIMUL");
	//if(simul_env.length())
	//renderPlatformDx11.SetShaderBinaryPath((simul_env+"/Media/shaderbin").c_str());
	//else
	renderPlatformDx11.SetShaderBinaryPath("shaderbin");

	return true;
}

void TrueSKYRender::OnD3D11LostDevice()
{
	weatherRenderer->InvalidateDeviceObjects();
	renderPlatformDx11.InvalidateDeviceObjects();
#if SIMUL_HDR
	hDRRenderer->InvalidateDeviceObjects();
	hdrFramebuffer->InvalidateDeviceObjects();
#endif
}

void TrueSKYRender::Shutdown()
{
	OnD3D11LostDevice();
	del(weatherRenderer, nullptr);
	del(hdrFramebuffer, nullptr);
	del(hDRRenderer, nullptr);
	del(env, nullptr);
}

void TrueSKYRender::OnD3D11CreateDevice(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	renderPlatformDx11.RestoreDeviceObjects(pd3dDevice);
	weatherRenderer->RestoreDeviceObjects(&renderPlatformDx11);

#if SIMUL_HDR
	hDRRenderer->RestoreDeviceObjects(&renderPlatformDx11);
	hdrFramebuffer->RestoreDeviceObjects(&renderPlatformDx11);
#endif
}

void TrueSKYRender::PreRender(int view_id, ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
							ID3D11RenderTargetView* pRenderTargetView, ID3D11DepthStencilView* pDenthStencilView,
							D3D11_VIEWPORT* pViewPort)
{
	env->Update();

	deviceContext.platform_context = pd3dImmediateContext;
	deviceContext.renderPlatform = &renderPlatformDx11;
	deviceContext.viewStruct.view_id = view_id;
	deviceContext.viewStruct.depthTextureStyle = simul::crossplatform::PROJECTION;

	simul::crossplatform::BaseFramebuffer::setDefaultRenderTargets(pRenderTargetView,
		pDenthStencilView,
		pViewPort->TopLeftX,
		pViewPort->TopLeftY,
		pViewPort->TopLeftX + pViewPort->Width,
		pViewPort->TopLeftY + pViewPort->Height
	);
	simul::crossplatform::SetGpuProfilingInterface(deviceContext, renderPlatformDx11.GetGpuProfiler());
	renderPlatformDx11.GetGpuProfiler()->StartFrame(deviceContext);

	viewport = renderPlatformDx11.GetViewport(deviceContext, 0);
	
#if SIMUL_CAMERA
	float aspect = (float)viewport.w / (float)viewport.h;
	if (reverseDepth)
		deviceContext.viewStruct.proj = camera.MakeDepthReversedProjectionMatrix(aspect);
	else
		deviceContext.viewStruct.proj = camera.MakeProjectionMatrix(aspect);
	deviceContext.viewStruct.view = camera.MakeViewMatrix();
#else	
	deviceContext.viewStruct.view = simul::math::Matrix4x4(&g_View.r[0].m128_f32[0]);
	deviceContext.viewStruct.proj = simul::math::Matrix4x4(&g_Projection.r[0].m128_f32[0]);
#endif
	// MUST call this after modifying a deviceContext.
	deviceContext.viewStruct.Init();

#if SIMUL_HDR
	hdrFramebuffer->Activate(deviceContext);
	hdrFramebuffer->Clear(deviceContext, 0.f, 0.f, 0.f, 0.f, reverseDepth ? 0.f : 1.f);
#endif

	static simul::base::Timer timer;
	float real_time = timer.UpdateTimeSum() / 1000.0f;


	weatherRenderer->PreRenderUpdate(deviceContext, real_time);
}

void TrueSKYRender::Render(ID3D11Texture2D* pDepthTexture,ID3D11ShaderResourceView* pDepthSRV)
{
	simul::dx11::Texture depthTexture;
#if SIMUL_HDR
	hdrFramebuffer->DeactivateDepth(deviceContext);
	depthTexture.InitFromExternalD3D11Texture2D(&renderPlatformDx11, hdrFramebuffer->GetDepthTexture()->AsD3D11Texture2D(), hdrFramebuffer->GetDepthTexture()->AsD3D11ShaderResourceView());
#else
	depthTexture.InitFromExternalD3D11Texture2D(&renderPlatformDx11, pDepthTexture, pDepthSRV);
#endif
	
	weatherRenderer->Render(deviceContext, simul::clouds::TrueSkyRenderMode::STANDARD, 1.0f, 1.0f, &depthTexture, nullptr, &viewport);

#if SIMUL_HDR	
	hdrFramebuffer->Deactivate(deviceContext);
	hDRRenderer->Render(deviceContext, hdrFramebuffer->GetTexture(), 1.0f, 1.0f);
#endif

	// Optional debug displays:
	//weatherRenderer->SetShowCloudCrossSections(true);
	//weatherRenderer->SetShowCompositing(true);
	//weatherRenderer->RenderOverlays(deviceContext);
	renderPlatformDx11.GetGpuProfiler()->EndFrame(deviceContext);
	//const char *txt = simul::dx11::Profiler::GetGlobalProfiler().GetDebugText(simul::base::PLAINTEXT);
	//renderPlatformDx11.Print(deviceContext, 16, 16, txt,vec4(1,1,1,1),vec4(0,0,0,.5f));
	//renderPlatformDx11.DrawDepth(deviceContext, 32, 32, 64, 64, hdrFramebuffer->GetDepthTexture());
}

void TrueSKYRender::Resize(const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
	int W = pBackBufferSurfaceDesc->Width;
	int H = pBackBufferSurfaceDesc->Height;
#if SIMUL_HDR
	hDRRenderer->SetBufferSize(W, H);
	hdrFramebuffer->SetWidthAndHeight(W, H);
	hdrFramebuffer->SetAntialiasing(1);
#endif
}

void TrueSKYRender::SetWorld(const XMMATRIX& world)
{
	g_World = world;
}

void TrueSKYRender::SetView(const XMMATRIX& view)
{
	g_View = view;
}

void TrueSKYRender::SetPro(const XMMATRIX& pro)
{
	g_Projection = pro;
}

void TrueSKYRender::SetViewPos(const XMFLOAT4& viewPos)
{
	g_ViewPos = viewPos;
}
