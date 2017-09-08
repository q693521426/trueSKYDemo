//
// Game.h
//

#pragma once

#include "pch.h"
#include <agile.h>
#include "BasicTimer.h"
#include "Gamepad.h"

// A basic game implementation that creates a D3D11 device and
// provides a game loop
ref class Game sealed
{
public:

    Game();

    // Initialization and management
    void Initialize(Windows::UI::Core::CoreWindow^ window);

    // Basic game loop
    void Tick();
    void Update(float totalTime, float elapsedTime);
    void Render();
	void OnShutdown();

    // Rendering helpers
    void Clear();
    void Present();

    void Suspend();
    void Resume();

private:
	void UpdateCamera();
	GamepadManager gamepadManager;
    void CreateDevice();
    void CreateResources();

    // Core Application state
    Platform::Agile<Windows::UI::Core::CoreWindow>     m_window;
    Windows::Foundation::Rect                          m_windowBounds;

    // Direct3D Objects
    D3D_FEATURE_LEVEL                                  m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device1>              m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>       m_d3dContext;
    Microsoft::WRL::ComPtr<ID3DXboxPerformanceContext> m_d3dXboxPerfContext;

    // Rendering resources
    Microsoft::WRL::ComPtr<IDXGISwapChain1>            m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>     m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>     m_depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>            m_depthStencil;

    // Game state
    INT64                                              m_frame;
    BasicTimer^                                        m_timer;
};

// PIX event colors
const DWORD EVT_COLOR_FRAME = PIX_COLOR_INDEX(1);
const DWORD EVT_COLOR_UPDATE = PIX_COLOR_INDEX(2);
const DWORD EVT_COLOR_RENDER = PIX_COLOR_INDEX(3);
