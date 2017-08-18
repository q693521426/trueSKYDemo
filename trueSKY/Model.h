#pragma once

#include "CFBXRendererDX11.h"
#include "ModelConstants.h"
#include "Sphere.h"

using namespace DirectX;

#define LOAD_MODEL		1
#define LIGHT_SPHERE	0

class Model
{

public:
	Model();
	~Model();
	bool Initialize();
	HRESULT OnD3D11CreateDevice(ID3D11Device*,ID3D11DeviceContext*);
	void Shutdown();

	void Render(ID3D11Device*, ID3D11DeviceContext*);
	void Resize(const DXGI_SURFACE_DESC*);

	void SetWVP(const XMMATRIX&);
	void SetViewPos(const XMFLOAT4&);
	void SetLight();
private:
	const static DWORD				NUMBER_OF_MODELS = 1;
	ID3D11VertexShader*				m_pVertexShader = nullptr;
	ID3D11PixelShader*				m_pPixelShader = nullptr;
	ID3D11Buffer*					m_pVertexBuffer = nullptr;
	ID3D11Buffer*					m_pIndexBuffer = nullptr;
	ID3D11Buffer*					m_pCBChangesEveryFrame = nullptr;
	ID3D11Buffer*					m_pLightBuffer = nullptr;
	ID3D11Buffer*					m_pFrustumBuffer = nullptr;
	ID3D11ShaderResourceView*		m_pTextureRV = nullptr;
	ID3D11SamplerState*				m_pSamplerLinear = nullptr;
	XMMATRIX						m_World;
	XMMATRIX						m_WVP;
	XMFLOAT4						m_ViewPos;
	LightBuffer						m_Light;
	Sphere*							m_LightSphere;

	FBX_LOADER::CFBXRenderDX11*		m_pFbxDX11[NUMBER_OF_MODELS];
	char*							m_files[NUMBER_OF_MODELS];

	
};

