#pragma once
#include "ModelConstants.h"
class FullScreenQuad
{
public:
	FullScreenQuad();
	~FullScreenQuad();
	
	bool Initialize(ID3D11Device*, int, int);
	void Shutdown();
	HRESULT OnD3D11CreateDevice(ID3D11Device*, ID3D11DeviceContext*);
	void Render(ID3D11Device*, ID3D11DeviceContext*, ID3D11ShaderResourceView*);

private:
	int								m_vertexCount;
	int								m_indexCount;
	ID3D11Buffer*					m_vertexBuffer = nullptr;
	ID3D11Buffer*					m_indexBuffer = nullptr;
	ID3D11VertexShader*				m_pVertexShader = nullptr;
	ID3D11PixelShader*				m_pPixelShader = nullptr;
	ID3D11InputLayout*				m_pVertexLayout = nullptr;
	ID3D11SamplerState*				g_pSamplerCCC = nullptr;
	ID3D11Buffer*					m_pCBChangesEveryFrame = nullptr;
	XMMATRIX						m_World;
	XMMATRIX						m_WVP;
};

