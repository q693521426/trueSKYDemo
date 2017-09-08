#pragma once

#include <vector>
#include "ModelConstants.h"

__declspec(align(16)) class Sphere
{
public:
	Sphere();
	~Sphere();

	bool Initialize(const D3DXVECTOR3& pos, float r);
	void Release();

	HRESULT OnD3D11CreateDevice(ID3D11Device*, ID3D11DeviceContext*);
	void Render(ID3D11Device*, ID3D11DeviceContext*);

	void SetWVP(const D3DXMATRIX&);
	void SetViewPos(const D3DXVECTOR4&);
	void CreateSphere(float radius, UINT sliceCount, UINT stackCount, std::vector<D3DXVECTOR3>* Positions, std::vector<UINT>* Indices, std::vector<D3DXVECTOR2>* UVs);
	
	//void* operator new(size_t i)
	//{
	//	return _mm_malloc(i, 16);
	//}

	//void operator delete(void* p)
	//{
	//	_mm_free(p);
	//}
private:
	ID3D11VertexShader*				m_pVertexShader;
	ID3D11PixelShader*				m_pPixelShader;
	ID3D11Buffer*					m_pVertexBuffer;
	ID3D11Buffer*					m_pIndexBuffer;
	ID3D11Buffer*					m_pCBChangesEveryFrame;
	ID3D11InputLayout*				m_pVertexLayout;
	D3DXMATRIX						m_WVP;
	D3DXVECTOR4						m_ViewPos;
	D3DXVECTOR3						Position;
	float							Radius;
	std::vector<D3DXVECTOR3>		Vertices;
	std::vector<UINT>				Indices;
	std::vector<D3DXVECTOR2>		Tex;
};

