//
// Game.cpp -
//
#define NOMINMAX
#include "pch.h"
#include "Game.h"

#if 1
#include "Simul/Clouds/Environment.h"
#include "Simul/Base/EnvironmentVariables.h"
#include "Simul/Platform/DirectX11/Direct3D11Renderer.h"
#include "Simul/Clouds/BaseWeatherRenderer.h"
#include "Simul/Clouds/BaseCloudRenderer.h"
#include "Simul/Platform/CrossPlatform/HDRRenderer.h"
#include "Simul/Terrain/BaseTerrainRenderer.h"
#include "Simul/Platform/CrossPlatform/GpuProfiler.h"
#include "Simul/Platform/DirectX11/ESRAMTexture.h"
#include "Simul/Base/RuntimeError.h"
#include "Simul/Base/Timer.h"
#include "Simul/Platform/Windows/VisualStudioDebugOutput.h"
#endif
VisualStudioDebugOutput visualStudioDebugOutput;
using namespace Windows::Xbox::Input;
using namespace Windows::Foundation::Collections;

#if 1
simul::clouds::Environment *environment=NULL;
static const bool reverseDepth=true;
/// An instance of the weather renderer class for dx11.
simul::clouds::BaseWeatherRenderer		*weatherRenderer=NULL;
/// An HDR Renderer to put the contents of hdrFramebuffer to the screen. In practice you will probably have your own method for this.
simul::crossplatform::HdrRenderer		*hDRRenderer=NULL;
/// A terrain renderer is included in order to generate example depth information for the compositor.
simul::terrain::BaseTerrainRenderer				*terrainRenderer=NULL;
/// A framebuffer to store the colour and depth textures for the view.
simul::dx11::Framebuffer					hdrFramebuffer=NULL;
/// This class performs general rendering functions for its platform, in this case, DirectX 11.
simul::dx11::RenderPlatform renderPlatformDx11;

// This allows live-recompile of shaders. 
void RecompileShaders()
{
	renderPlatformDx11.RecompileShaders();
	weatherRenderer->RecompileShaders();
	hDRRenderer->RecompileShaders();
	terrainRenderer->RecompileShaders();
}

simul::crossplatform::Camera cam;
int view_id=-1;
#endif
// Constructor.
Game::Game()
{
    m_frame = 0;
	environment=new simul::clouds::Environment();
	{
		weatherRenderer	=new simul::clouds::BaseWeatherRenderer(environment,NULL);

		hDRRenderer		=new simul::crossplatform::HdrRenderer();
		terrainRenderer	=new simul::terrain::BaseTerrainRenderer(NULL);
		terrainRenderer->SetBaseSkyInterface(environment->skyKeyframer);

		hdrFramebuffer.SetFormat(simul::crossplatform::RGBA_16_FLOAT);
		hdrFramebuffer.SetDepthFormat(simul::crossplatform::D_16_UNORM);
		hdrFramebuffer.SetUseFastRAM(true,true);
	}
	environment->cloudKeyframer->SetDefaultNumSlices(90);

	renderPlatformDx11.PushShaderPath("Simul\\Platform\\DirectX11\\HLSL");
	renderPlatformDx11.PushTexturePath("Simul\\Media\\Textures");
	renderPlatformDx11.SetShaderBinaryPath("Simul\\shaderbin");
	renderPlatformDx11.SetShaderBuildMode(simul::crossplatform::NEVER_BUILD);
	
	// Optional things:
	//direct3D11Renderer->SetAntialiasing(1);
	//direct3D11Renderer->GetSimulWeatherRenderer()->GetBaseCloudRenderer()->SetMaxSlices(view_id,90);
	//direct3D11Renderer->GetSimulWeatherRenderer()->SetDownscale(4);
	cam.SetPositionAsXYZ(0,0,800.0f);
	cam.SetVerticalFieldOfViewDegrees(30.f);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(Windows::UI::Core::CoreWindow^ window)
{
    m_window = window;

    CreateDevice();

    CreateResources();

    m_timer = ref new BasicTimer();
    gamepadManager.InitializeCurrentGamepad();
}

void Game::OnShutdown()
{
    gamepadManager.ShutdownCurrentGamepad();
	
	weatherRenderer->InvalidateDeviceObjects();
	hDRRenderer->InvalidateDeviceObjects();
	terrainRenderer->InvalidateDeviceObjects();
	renderPlatformDx11.InvalidateDeviceObjects();
	hdrFramebuffer.InvalidateDeviceObjects();

	del(weatherRenderer,NULL);
	del(hDRRenderer,NULL);
	del(terrainRenderer,NULL);
	delete environment;
}

void Game::UpdateCamera()
{
	float dx=gamepadManager.dx;
	float dy=gamepadManager.dy;
	simul::math::Vector3 Z(0,0,1.f);
	simul::math::Vector3 X=cam.GetOrientation().Tx();
	static float rspeed=3.0f;
	cam.Rotate(dx*rspeed,Z);
	cam.Rotate(dy*rspeed,X);
}

// Executes basic game loop.
void Game::Tick()
{
    PIXBeginEvent(EVT_COLOR_FRAME, L"Frame %d", m_frame);
	ERRNO_CHECK
	gamepadManager.Update(0.01f);
	UpdateCamera();
    m_timer->Update();
	ERRNO_CHECK
    Update(m_timer->Total, m_timer->Delta);
	ERRNO_CHECK
#if 1
	environment->Update();
#endif
    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world
void Game::Update(float /* totalTime */, float /* elapsedTime */)
{
    PIXBeginEvent(EVT_COLOR_UPDATE, L"Update");

    // Allow the game to exit by pressing the view button on the controller.
    // This is just a helper for development.
    IVectorView<IGamepad^>^ gamepads = Gamepad::Gamepads;

    if(gamepads->Size > 0)
    {
        IGamepadReading^ reading = gamepads->GetAt(0)->GetCurrentReading();
        if (reading->IsViewPressed)
        {
            Windows::ApplicationModel::Core::CoreApplication::Exit();
        }
    }

    // TODO: Add your game logic here

    PIXEndEvent();
}

void Test(simul::crossplatform::DeviceContext	&deviceContext,simul::crossplatform::Texture *tex)
{
	int x1=0,y1=0,dx=1,dy=1;
	simul::crossplatform::Effect *m_pDebugEffect=renderPlatformDx11.GetDebugEffect();
	simul::crossplatform::EffectTechnique *tech	=m_pDebugEffect->GetTechniqueByName("show_depth");
	if(tex->GetSampleCount()>0)
	{
		tech=m_pDebugEffect->GetTechniqueByName("show_depth_ms");
		m_pDebugEffect->SetTexture(deviceContext,"imageTextureMS",tex);
	}
	else
	{
		m_pDebugEffect->SetTexture(deviceContext,"imageTexture",tex);
	}
	ID3D11DeviceContext *pContext=deviceContext.asD3D11DeviceContext();
	unsigned int num_v=1;
	D3D11_VIEWPORT viewport;
	pContext->RSGetViewports(&num_v,&viewport);
}
// Draws the scene
void Game::Render()
{
    PIXBeginEvent(EVT_COLOR_RENDER, L"Render");

    Clear();
	
	//direct3D11Renderer->Render(view_id,m_d3dDevice.Get(),m_d3dContext.Get());
	{
		simul::crossplatform::DeviceContext	deviceContext;
		deviceContext.platform_context	=m_d3dContext.Get();
		deviceContext.renderPlatform	=&renderPlatformDx11;
		deviceContext.viewStruct.view_id=view_id;
		deviceContext.viewStruct.depthTextureStyle	=simul::crossplatform::PROJECTION;
		
	
		simul::crossplatform::SetGpuProfilingInterface(deviceContext,renderPlatformDx11.GetGpuProfiler());
		SIMUL_COMBINED_PROFILE_STARTFRAME(deviceContext)
		SIMUL_COMBINED_PROFILE_START(deviceContext,"Frame")

		// In normal circumstances you will not have a crossplatform::Texture pointer to hand with depth information
		// In that case, instantiate a dx11::Texture, and use InitFromExternalD3D11Texture2D (see below).
		simul::crossplatform::Texture *depthTexture;
		hdrFramebuffer.Activate(deviceContext);
		{
			simul::crossplatform::Viewport viewport=renderPlatformDx11.GetViewport(deviceContext,0);
			hdrFramebuffer.Clear(deviceContext,0.f,0.f,0.f,0.f,reverseDepth?0.f:1.f);
			deviceContext.viewStruct.view=cam.MakeViewMatrix();
			if(reverseDepth)
				deviceContext.viewStruct.proj=cam.MakeDepthReversedProjectionMatrix((float)viewport.w/(float)viewport.h);
			else
				deviceContext.viewStruct.proj=cam.MakeProjectionMatrix((float)viewport.w/(float)viewport.h);
			weatherRenderer->PreRenderUpdate(deviceContext,0.0f);
			// We draw terrain to provide some depth information for the mixed-resolution compositor:
			if(terrainRenderer)
			{
				terrainRenderer->Render(deviceContext,1.f);
			}
			// We must deactivate the depth buffer here, in order to use it as a texture:
			hdrFramebuffer.DeactivateDepth(deviceContext);
			depthTexture=hdrFramebuffer.GetDepthTexture();
	
			// In practice the MixedResolutionView class can manage its own framebuffer and depth texture, but here we use an external one
			// from hdrFramebuffer to show how this can be done.
			// Convert the current D3D viewport information into a Simul-style viewport structure:
			weatherRenderer->Render(deviceContext,simul::clouds::TrueSkyRenderMode::STANDARD,1.f,.44f
				,depthTexture,nullptr
				,&viewport);
		}
		hdrFramebuffer.Deactivate(deviceContext);
		hDRRenderer->Render(deviceContext,hdrFramebuffer.GetTexture(),1.f,1.f);
		// Optional debug displays:
		//weatherRenderer->GetSkyRenderer()->RenderFades(deviceContext,0,0,600,400);
		//weatherRenderer->GetCloudRenderer()->RenderCrossSections(deviceContext,0,0,600,400);
		// Enable these overlays if needed:
		//view->RenderDepthBuffers(deviceContext,depthTexture,view->GetScreenWidth()/2,0,view->GetScreenWidth()/2,view->GetScreenHeight()/2);
		//weatherRenderer->RenderCompositingTextures(deviceContext,view->GetScreenWidth()/2,view->GetScreenHeight()/2,view->GetScreenWidth()/2,view->GetScreenHeight()/2);
	
		SIMUL_COMBINED_PROFILE_END(deviceContext)
		SIMUL_COMBINED_PROFILE_ENDFRAME(deviceContext)
		{
			float white[]={1.f,1.f,1.f,1.f};
			float grey[]={0.2f,0.2f,0.2f,0.5f};
			const char *txt=renderPlatformDx11.GetGpuProfiler()->GetDebugText();
			renderPlatformDx11.Print(deviceContext			,12	,12,txt,white,grey);
		}
	}
    Present();

    PIXEndEvent();
}

// Helper method to clear the backbuffers
void Game::Clear()
{
    // Clear the views
    const float clearColor[] = { 0.39f, 0.58f, 0.93f, 1.000f };
    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

// Presents the backbuffer contents to the screen
void Game::Present()
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        Initialize(m_window.Get());
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}

void Game::Suspend()
{
    m_d3dXboxPerfContext->Suspend(0);
}

void Game::Resume()
{
    m_d3dXboxPerfContext->Resume();
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
    // This flag adds support for surfaces with a different color channel ordering than the API default.
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	creationFlags|=D3D11_CREATE_DEVICE_INSTRUMENTED;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0
    };

    // Create the DX11 API device object, and get a corresponding context.
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

    DX::ThrowIfFailed(
        D3D11CreateDevice(
            nullptr,                    // specify null to use the default adapter
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,                    // leave as nullptr unless software device
            creationFlags,              // optionally set debug and Direct2D compatibility flags
            featureLevels,              // list of feature levels this app can support
            ARRAYSIZE(featureLevels),   // number of entries in above list
            D3D11_SDK_VERSION,          // always set this to D3D11_SDK_VERSION
            &device,                    // returns the Direct3D device created
            &m_featureLevel,            // returns feature level of device created
            &context                    // returns the device immediate context
            )
        );

    // Get the DirectX11.1 device by QI off the DirectX11 one.
    DX::ThrowIfFailed(device.As(&m_d3dDevice));

    // And get the corresponding device context in the same way.
    DX::ThrowIfFailed(context.As(&m_d3dContext));

    // Get the perf context for the graphics suspend and resume.
    DX::ThrowIfFailed(context.As(&m_d3dXboxPerfContext));

	// Use these in practice:
	renderPlatformDx11.RestoreDeviceObjects(m_d3dDevice.Get());
	weatherRenderer->RestoreDeviceObjects(&renderPlatformDx11);
	// These are for example:
	hDRRenderer->RestoreDeviceObjects(&renderPlatformDx11);
	hdrFramebuffer.RestoreDeviceObjects(&renderPlatformDx11);
	terrainRenderer->RestoreDeviceObjects(&renderPlatformDx11);
	
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Store the window bounds so the next time we get a SizeChanged event we can
    // avoid rebuilding everything if the size is identical.
    m_windowBounds = m_window.Get()->Bounds;

    // If the swap chain already exists, resize it, otherwise create one.
    if(m_swapChain != nullptr)
    {
        DX::ThrowIfFailed(m_swapChain->ResizeBuffers(2, 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0));
    }
    else
    {
        // First, retrieve the underlying DXGI Device from the D3D Device
        Microsoft::WRL::ComPtr<IDXGIDevice1>  dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(&dxgiAdapter));

        // And obtain the factory object that created it.
        Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), &dxgiFactory));

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
        swapChainDesc.Width = 1920;
        swapChainDesc.Height = 1080;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.Flags = 0;

        // Create a SwapChain from a CoreWindow.
        DX::ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(m_d3dDevice.Get(), reinterpret_cast<IUnknown*>(m_window.Get()), &swapChainDesc, nullptr, &m_swapChain));
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView));

    // Cache the rendertarget dimensions in our helper class for convenient use.
    D3D11_TEXTURE2D_DESC backBufferDesc = {0};
    backBuffer->GetDesc(&backBufferDesc);

    // Allocate a 2-D surface as the depth/stencil buffer and
    // create a DepthStencil view on this surface to use on bind.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D16_UNORM, backBufferDesc.Width, backBufferDesc.Height, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencil));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, &m_depthStencilView));

    // Create a viewport descriptor of the full window size.
    CD3D11_VIEWPORT viewPort(0.0f, 0.0f, static_cast<float>(backBufferDesc.Width), static_cast<float>(backBufferDesc.Height));

    // Set the current viewport using the descriptor.
    m_d3dContext->RSSetViewports(1, &viewPort);

	
	/// TRUESKY
	int W	=backBufferDesc.Width;
	int H	=backBufferDesc.Height;
	hDRRenderer->SetBufferSize(W,H);
	hdrFramebuffer.SetWidthAndHeight(W,H);
	hdrFramebuffer.SetAntialiasing(1);
	hdrFramebuffer.MoveToFastRAM();
	if(view_id<0)
		view_id=0;
	
	/// END TRUESKY
}
