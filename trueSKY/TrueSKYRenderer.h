#pragma once
#ifndef __TRUESKYRENDERER_H_
#define __TRUESKYRENDERER_H_

#include "Simul/LicenseKey.h"
#include "Simul/Base/EnvironmentVariables.h"
#include "Simul/Clouds/TrueSkyRenderer.h"
#include "Simul/Platform/DirectX11/RenderPlatform.h"
#include "Simul/Platform/DirectX11/Direct3D11Manager.h"
#include "Simul/Platform/DirectX11/Texture.h"
#include "Simul/Clouds/BaseWeatherRenderer.h"
#include "Simul/Clouds/BaseCloudRenderer.h"

#include "Simul/Platform/DirectX11/SaveTextureDX1x.h"		// To save textures

#include "Simul/Platform/CrossPlatform/HDRRenderer.h"
#include "Simul/Platform/CrossPlatform/MixedResolutionView.h"

#include "Simul/Platform/CrossPlatform/GpuProfiler.h"
// Terrain, to provide example depth data:
#include "Simul/Terrain/BaseTerrainRenderer.h"
// A camera class to control the viewpoint
#include "Simul/Platform/CrossPlatform/Camera.h"

#include "Simul/Platform/DirectX11/Effect.h"
#include "Simul/Platform/CrossPlatform/DeviceContext.h"
#include "Simul/Platform/CrossPlatform/CommandLineParams.h"

#pragma warning( disable : 4100 )

class TrueSKYRenderer
{
public:
	TrueSKYRenderer(simul::clouds::Environment *env);
	~TrueSKYRenderer();

	void OnD3D11CreateDevice(struct ID3D11Device* pd3dDevice);
	void Render(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext, 
		ID3D11RenderTargetView* pRenderTargetView, ID3D11DepthStencilView* pDenthStencilView, 
		D3D11_VIEWPORT* pViewPort);
	void ResizeView(int width, int height);
	void RecompileShaders();
	void OnD3D11LostDevice();
private:
	static const bool reverseDepth = true;
	simul::dx11::RenderPlatform				renderPlatformDx11;
	simul::clouds::BaseWeatherRenderer*		weatherRenderer = nullptr;
	simul::terrain::BaseTerrainRenderer*	terrainRenderer = nullptr;
	simul::crossplatform::BaseFramebuffer*	hdrFramebuffer = nullptr;
	simul::crossplatform::HdrRenderer		*hDRRenderer = nullptr;
	simul::crossplatform::Camera			camera;
};



#endif // !__TRUESKYRENDERER
