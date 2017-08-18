#pragma once

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
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
#ifdef _MSC_VER
#include "Simul/Platform/Windows/VisualStudioDebugOutput.h"
#include "Simul/Platform/DirectX11/Direct3D11Renderer.h"
#include "Simul/Platform/CrossPlatform/SL/sky_constants.sl"
#include "Simul/Clouds/BaseWeatherRenderer.h"
#include "Simul/Sky/BaseSkyRenderer.h"
#endif

#include "ModelConstants.h"
#define SIMUL_CAMERA 1
#define SIMUL_HDR 1

class TrueSKYRender
{
public:
	TrueSKYRender();
	~TrueSKYRender();

	bool Initialize(char* sq);
	void OnD3D11LostDevice();
	void Shutdown();
	void OnD3D11CreateDevice(ID3D11Device*, ID3D11DeviceContext*);
	void PreRender(int, ID3D11Device*, ID3D11DeviceContext*, ID3D11RenderTargetView* , ID3D11DepthStencilView* ,D3D11_VIEWPORT* );
	void Render(ID3D11Texture2D*, ID3D11ShaderResourceView*);
	void Resize(const DXGI_SURFACE_DESC*);

	void SetWorld(const XMMATRIX&);
	void SetView(const XMMATRIX&);
	void SetPro(const XMMATRIX&);
	void SetViewPos(const XMFLOAT4&);

private:
	static const bool reverseDepth = false;
	simul::clouds::Environment*				env = nullptr;
	simul::dx11::RenderPlatform				renderPlatformDx11;
	simul::clouds::BaseWeatherRenderer*		weatherRenderer = nullptr;
	simul::crossplatform::BaseFramebuffer*	hdrFramebuffer = nullptr;
	simul::crossplatform::HdrRenderer*		hDRRenderer = nullptr;
	simul::crossplatform::Camera			camera;
	simul::crossplatform::DeviceContext		deviceContext;
	simul::crossplatform::Viewport			viewport;

	XMMATRIX						g_World;
	XMMATRIX						g_View;
	XMMATRIX						g_Projection;
	XMFLOAT4						g_ViewPos;
};

