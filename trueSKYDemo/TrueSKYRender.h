#pragma once

#pragma warning( disable : 4100 )

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <vector>

#include "ModelConstants.h"
#include "FrameBuffer.h"
#include "FullScreenQuad.h"
#include "TrueSKYConstants.h"

#include "Simul/LicenseKey.h"
#include "Simul/Base/EnvironmentVariables.h"
#include "Simul/Clouds/TrueSkyRenderer.h"
#include "Simul/Platform/DirectX11/RenderPlatform.h"
#include "Simul/Platform/DirectX11/Direct3D11Manager.h"
#include "Simul/Platform/DirectX11/Texture.h"
#include "Simul/Clouds/BaseWeatherRenderer.h"
#include "Simul/Clouds/BaseCloudRenderer.h"
#include "Simul/Clouds/Environment.h"

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

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

#define SIMUL_CAMERA 0
#define SIMUL_HDR 0
#define MICROPROFILE 0
__declspec(align(16)) class TrueSKYRender
{
public:
	static char* cloudFramerIntAttrName[];
	static char* cloudFrameFloatAttrName[];

	TrueSKYRender();
	~TrueSKYRender();

	bool Initialize(const char* sq = nullptr);
	bool ReLoadSq(const char* sq = nullptr);
	void OnD3D11LostDevice();
	void Release();
	HRESULT OnD3D11CreateDevice(ID3D11Device*, ID3D11DeviceContext*);
	void PreRender(int, ID3D11Device*, ID3D11DeviceContext*,
		bool IsProfile = false,bool Is2DCloud = true,
		bool IsCloud = true,bool IsSky = true,bool IsAnimation = true);
	void Render(bool IsTrueSky = true,bool IsProfile = false);
	void Resize(ID3D11RenderTargetView*,ID3D11DepthStencilView*	,D3D11_VIEWPORT*,const DXGI_SURFACE_DESC*);
	void RecompileShaders();
	void OnFrameMove(double fTime,float fElapsedTime,bool* keydown);
	bool MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	void OnMouse(bool,bool,bool,int,int,int );
	bool InitEnv(const char* sq = nullptr);
	void InitKeyFrameArray();
	void InitKeyFrameAttr();

	void SetWorld(const D3DXMATRIX&);
	void SetView(const D3DXMATRIX&);
	void SetPro(const D3DXMATRIX&);
	void SetViewPos(const D3DXVECTOR3&);

	void SetAnimationTimeStep(const int);

	void IsRender(bool,bool,bool);

	simul::sky::SkyKeyframe* GetSkyFrameAttr();
	void UpdateSkyFrameFloatAttr(const char* name,float value,float min,float max);

	simul::sky::SkyKeyframer* GetSkyFramerAttr();
	void UpdateSkyFramerFloatAttr(const char* name,float value,float min,float max);

	simul::clouds::CloudKeyframe* GetCloudFrameAttr();
	void TrueSKYRender::UpdateCloudFrameFloatAttr(const char* name,float value,float min,float max);

	simul::clouds::CloudKeyframe* GetCloud2DFrameAttr();
	void UpdateCloud2DFrameFloatAttr(const char* name,float value,float min,float max);

	simul::clouds::CloudKeyframer* GetCloudFramerAttr();
	void UpdateCloudFramerIntAttr(const char* name,int value,int min,int max);

	simul::clouds::CloudKeyframer* GetCloud2DFramerAttr();
	void UpdateCloud2DFramerIntAttr(const char* name,int value,int min,int max);

	D3DXVECTOR3 GetLightDir();
	static int clamp(int i,int a,int b)
	{
		if(a == INT_MIN || b == INT_MAX)
			return i;
		return (i<a)?a:(i>b?b:i);
	}
	static float clamp(float i,float a,float b)
	{
		if(a == FLT_MIN || b == FLT_MAX)
			return i;
		return (i<a)?a:(i>b?b:i);
	}
	static D3DXVECTOR3 clamp(D3DXVECTOR3 i,D3DXVECTOR3 a,D3DXVECTOR3 b)
	{
		return D3DXVECTOR3(clamp(i.x,a.x,b.x),clamp(i.y,a.y,b.y),clamp(i.z,a.z,b.z));
	}
private:
	simul::dx11::RenderPlatform					renderPlatformDx11;
	simul::crossplatform::Camera				camera;
	simul::crossplatform::MouseCameraState		mouseCameraState;
	simul::crossplatform::MouseCameraInput		mouseCameraInput;
	simul::crossplatform::DeviceContext			deviceContext;
	simul::crossplatform::Viewport				viewport;
	std::vector<simul::sky::SkyKeyframe>		skyKeyFramerArray;
	std::vector<simul::clouds::CloudKeyframe>	cloudKeyFramerArray;
	std::vector<simul::clouds::CloudKeyframe>	cloud2DKeyFramerArray;

	D3DXMATRIX						m_World;
	D3DXMATRIX						m_View;
	D3DXMATRIX						m_Projection;
	D3DXVECTOR3						m_ViewPos;
	
	bool							reverseDepth;
	double							time_refresh;
	int								time_step;

	simul::clouds::Environment*					env;
	simul::clouds::BaseWeatherRenderer*			weatherRenderer;
	simul::crossplatform::BaseFramebuffer*		hdrFramebuffer;
	simul::crossplatform::HdrRenderer*			hDRRenderer;

	simul::sky::SkyKeyframer*					skyKeyFramer;
	simul::clouds::CloudKeyframer*				cloudKeyFramer;
	simul::clouds::CloudKeyframer*				cloud2DKeyFramer;

	simul::sky::SkyKeyframe*					m_SkyFrameAttr;
	simul::clouds::CloudKeyframe*				m_CloudFrameAttr;
	simul::clouds::CloudKeyframe*				m_Cloud2DFrameAttr;

	ID3D11RenderTargetView*						m_pRenderTargetView;
	ID3D11DepthStencilView*						m_pDenthStencilView;
	D3D11_VIEWPORT*								m_pViewPort;
	FrameBuffer*								m_FrameBuffer;
	FullScreenQuad*								m_Renderer;
};

