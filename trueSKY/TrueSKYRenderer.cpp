#include "TrueSKYRenderer.h"


TrueSKYRenderer::TrueSKYRenderer(simul::clouds::Environment *env)
{
	weatherRenderer = new simul::clouds::BaseWeatherRenderer(env, NULL);

	hDRRenderer = new simul::crossplatform::HdrRenderer();
	terrainRenderer = new simul::terrain::BaseTerrainRenderer(NULL);
	terrainRenderer->SetBaseSkyInterface(env->skyKeyframer);

	// Downscale resolution for cloud rendering:
	hdrFramebuffer = renderPlatformDx11.CreateFramebuffer();
	hdrFramebuffer->SetFormat(simul::crossplatform::RGBA_16_FLOAT);
	hdrFramebuffer->SetDepthFormat(simul::crossplatform::D_32_FLOAT);

	camera.SetPositionAsXYZ(0, 0, 2200.f);
	float look[] = { 0.f,1.f,0.f }, up[] = { 0.f,0.f,1.f };
	camera.LookInDirection(look, up);
	camera.SetHorizontalFieldOfViewDegrees(90.f);
	// Automatic vertical fov - depends on window shape:
	camera.SetVerticalFieldOfViewDegrees(0.f);
	// We can leave the default camera setup in place, or change it:
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
	// Whether run from the project directory or from the executable location, we want to be
	// able to find the shaders and textures:
	renderPlatformDx11.PushTexturePath("Media/Textures");
	renderPlatformDx11.PushShaderPath("Platform/CrossPlatform/SFX");
	renderPlatformDx11.PushShaderPath("Platform/DirectX11/HLSL");
	renderPlatformDx11.SetShaderBinaryPath("shaderbin");


	// Shader binaries: we want to use a shared common directory under Simul/Media. But if we're running from some other place, we'll just create a "shaderbin" directory.
	//std::string simul_env=simul::base::EnvironmentVariables::GetSimulEnvironmentVariable("SIMUL");
	//if(simul_env.length())
	//renderPlatformDx11.SetShaderBinaryPath((simul_env+"/Media/shaderbin").c_str());
	//else
	renderPlatformDx11.SetShaderBinaryPath("shaderbin");
}


TrueSKYRenderer::~TrueSKYRenderer()
{
	OnD3D11LostDevice();
	del(weatherRenderer, NULL);
	del(hDRRenderer, NULL);
	del(terrainRenderer, NULL);
	del(hdrFramebuffer, NULL);
}

void TrueSKYRenderer::RecompileShaders()
{
	renderPlatformDx11.RecompileShaders();
	weatherRenderer->RecompileShaders();
	hDRRenderer->RecompileShaders();
	terrainRenderer->RecompileShaders();
}

void TrueSKYRenderer::OnD3D11LostDevice()
{
	weatherRenderer->InvalidateDeviceObjects();
	hDRRenderer->InvalidateDeviceObjects();
	terrainRenderer->InvalidateDeviceObjects();
	renderPlatformDx11.InvalidateDeviceObjects();
	hdrFramebuffer->InvalidateDeviceObjects();

}

void TrueSKYRenderer::ResizeView(int width,int height)
{
	hDRRenderer->SetBufferSize(width, height);
	hdrFramebuffer->SetWidthAndHeight(width, height);
	hdrFramebuffer->SetAntialiasing(1);
}

void TrueSKYRenderer::OnD3D11CreateDevice(struct ID3D11Device* pd3dDevice)
{
	// Use these in practice:
	renderPlatformDx11.RestoreDeviceObjects(pd3dDevice);
	weatherRenderer->RestoreDeviceObjects(&renderPlatformDx11);
	// These are for example:
	hDRRenderer->RestoreDeviceObjects(&renderPlatformDx11);
	hdrFramebuffer->RestoreDeviceObjects(&renderPlatformDx11);
	terrainRenderer->RestoreDeviceObjects(&renderPlatformDx11);

}

void TrueSKYRenderer::Render(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext, 
							ID3D11RenderTargetView* pRenderTargetView, ID3D11DepthStencilView* pDenthStencilView, 
							D3D11_VIEWPORT* pViewPort)
{
	simul::crossplatform::DeviceContext	deviceContext;
	deviceContext.platform_context = pContext;
	deviceContext.renderPlatform = &renderPlatformDx11;
	deviceContext.viewStruct.view_id = 0;
	deviceContext.viewStruct.depthTextureStyle = simul::crossplatform::PROJECTION;

	//unsigned num = 0;
	//D3D11_VIEWPORT d3d11viewports[8];
	//((ID3D11DeviceContext*)pContext)->RSGetViewports(&num, NULL);
	//((ID3D11DeviceContext*)pContext)->RSGetViewports(&num, d3d11viewports);

	//ID3D11RenderTargetView *pOldRenderTarget[] = { NULL,NULL,NULL,NULL };
	//ID3D11DepthStencilView *pOldDepthSurface = NULL;
	//((ID3D11DeviceContext*)pContext)->OMGetRenderTargets(4,
	//	pOldRenderTarget,
	//	&pOldDepthSurface
	//);

	simul::crossplatform::BaseFramebuffer::setDefaultRenderTargets(pRenderTargetView,
		pDenthStencilView,
		pViewPort->TopLeftX,
		pViewPort->TopLeftY,
		pViewPort->TopLeftX + pViewPort->Width,
		pViewPort->TopLeftY + pViewPort->Height
	);
	simul::crossplatform::SetGpuProfilingInterface(deviceContext, renderPlatformDx11.GetGpuProfiler());
	renderPlatformDx11.GetGpuProfiler()->StartFrame(deviceContext);
	// In normal circumstances you will not have a crossplatform::Texture pointer to hand with depth information
	// In that case, instantiate a dx11::Texture, and use InitFromExternalD3D11Texture2D (see below).
	simul::dx11::Texture depthTexture;
	simul::crossplatform::Viewport viewport = renderPlatformDx11.GetViewport(deviceContext, 0);
	hdrFramebuffer->Activate(deviceContext);
	// The following block renders to the hdrFramebuffer's rendertarget:
	{
		hdrFramebuffer->Clear(deviceContext, 0.f, 0.f, 0.f, 0.f, reverseDepth ? 0.f : 1.f);
		deviceContext.viewStruct.view = camera.MakeViewMatrix();
		float aspect = (float)viewport.w / (float)viewport.h;
		if (reverseDepth)
			deviceContext.viewStruct.proj = camera.MakeDepthReversedProjectionMatrix(aspect);
		else
			deviceContext.viewStruct.proj = camera.MakeProjectionMatrix(aspect);
		// MUST call this after modifying a deviceContext.
		deviceContext.viewStruct.Init();
		static simul::base::Timer timer;
		float real_time = timer.UpdateTimeSum() / 1000.0f;

		weatherRenderer->PreRenderUpdate(deviceContext, real_time);
		//if (terrainRenderer)
		//{
		//	LightingQueryResult res = weatherRenderer->GetBaseSkyRenderer()->GetLightingQuery(0x913563A
		//		, crossplatform::GetCameraPosVector(deviceContext.viewStruct.view));
		//	vec3 cam_pos = simul::crossplatform::GetCameraPosVector(deviceContext.viewStruct.view);
		//	if (weatherRenderer&&weatherRenderer->GetBaseCloudRenderer())
		//		terrainRenderer->SetCloudShadowTexture(weatherRenderer->GetBaseCloudRenderer()->GetCloudShadowStruct());
		//	terrainRenderer->SetLighting((const float*)res.sunlight, (const float*)res.moonlight, (const float*)res.ambient);
		//	terrainRenderer->Render(deviceContext, 1.f);
		//}
		// We must deactivate the depth buffer here, in order to use it as a texture:
		hdrFramebuffer->DeactivateDepth(deviceContext);
		depthTexture.InitFromExternalD3D11Texture2D(&renderPlatformDx11, hdrFramebuffer->GetDepthTexture()->AsD3D11Texture2D(), hdrFramebuffer->GetDepthTexture()->AsD3D11ShaderResourceView());

		weatherRenderer->Render(deviceContext, simul::clouds::TrueSkyRenderMode::STANDARD, 1.0f, 1.0f, &depthTexture, nullptr, &viewport);
	}
	hdrFramebuffer->Deactivate(deviceContext);
	//hDRRenderer->Render(deviceContext, hdrFramebuffer->GetTexture(), 1.0f, 0.44f);
	hDRRenderer->Render(deviceContext, hdrFramebuffer->GetTexture(), 1.0f, 1.0f);
	// Optional debug displays:
	//weatherRenderer->SetShowCloudCrossSections(true);
	//weatherRenderer->SetShowCompositing(true);
	//weatherRenderer->RenderOverlays(deviceContext);
	renderPlatformDx11.GetGpuProfiler()->EndFrame(deviceContext);
	//const char *txt = simul::dx11::Profiler::GetGlobalProfiler().GetDebugText(simul::base::PLAINTEXT);
	//renderPlatformDx11.Print(deviceContext, 16, 16, txt,vec4(1,1,1,1),vec4(0,0,0,.5f));
	//renderPlatformDx11.DrawDepth(deviceContext, 32, 32, 64, 64, hdrFramebuffer->GetDepthTexture());
}
