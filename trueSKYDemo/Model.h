#pragma once

#include "FBXLoader/CFBXRendererDX11.h"
#include "ModelConstants.h"
#include "Sphere.h"

using namespace DirectX;

#define LOAD_MODEL		1
#define LIGHT_SPHERE	0

__declspec(align(16)) class Model
{

public:
	Model();
	~Model();
	bool Initialize();
	HRESULT OnD3D11CreateDevice(ID3D11Device*,ID3D11DeviceContext*);
	void Release();

	void Render(ID3D11Device*, ID3D11DeviceContext*);
	void Resize(const DXGI_SURFACE_DESC*);

	void SetWVP(const D3DXMATRIX&);
	void SetViewPos(const D3DXVECTOR4&);
	void SetLight(const LightBuffer*);

	//void* operator new(size_t i)
	//{
	//	return _mm_malloc(i, 16);
	//}

	//void operator delete(void* p)
	//{
	//	_mm_free(p);
	//}
private:
	float							ModelHeight;
	float							ModelScaling;
	D3DXMATRIX						m_World;
	D3DXMATRIX						m_WVP;
	D3DXVECTOR4						m_ViewPos;
	const static DWORD				NUMBER_OF_MODELS = 1;
	ID3D11VertexShader*				m_pVertexShader;
	ID3D11PixelShader*				m_pPixelShader;
	ID3D11Buffer*					m_pVertexBuffer;
	ID3D11Buffer*					m_pIndexBuffer;
	ID3D11Buffer*					m_pCBChangesEveryFrame;
	ID3D11Buffer*					m_pLightBuffer;
	ID3D11Buffer*					m_pFrustumBuffer;
	ID3D11ShaderResourceView*		m_pTextureRV;
	ID3D11SamplerState*				m_pSamplerLinear;
	Sphere*							m_LightSphere;
	FBX_LOADER::CFBXRenderDX11*		m_pFbxDX11[NUMBER_OF_MODELS];
	char*							m_files[NUMBER_OF_MODELS];
	LightBuffer						m_Light;

};

