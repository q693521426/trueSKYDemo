#pragma once
#include "ModelConstants.h"
class FullScreenQuad
{
public:
	FullScreenQuad();
	~FullScreenQuad();
	
	bool Resize(int, int);
	void Release();
	HRESULT OnD3D11CreateDevice(ID3D11Device*, ID3D11DeviceContext*);
	void Render(ID3D11RenderTargetView*, ID3D11ShaderResourceView*);

private:
	D3DXMATRIX						m_World;
	D3DXMATRIX						m_WVP;
	int								m_vertexCount;
	int								m_indexCount;
	ID3D11Device*					m_pd3dDevice;
	ID3D11DeviceContext*			m_pd3dImmediateContext;
	ID3D11Buffer*					m_vertexBuffer;
	ID3D11Buffer*					m_indexBuffer;
	ID3D11VertexShader*				m_pVertexShader;
	ID3D11PixelShader*				m_pPixelShader;
	ID3D11InputLayout*				m_pVertexLayout;
	ID3D11SamplerState*				m_pSamplerCCC;
	ID3D11Buffer*					m_pCBChangesEveryFrame;
	ID3D11BlendState*				m_pBlendState;
};

