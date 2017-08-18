#pragma once

#include <vector>
#include "ModelConstants.h"

using namespace DirectX;


class Sphere
{
public:
	Sphere();
	~Sphere();

	bool Initialize(const XMFLOAT3& pos, float r);
	void Shutdown();

	HRESULT OnD3D11CreateDevice(ID3D11Device*, ID3D11DeviceContext*);
	void Render(ID3D11Device*, ID3D11DeviceContext*);

	void SetWVP(const XMMATRIX&);
	void SetViewPos(const XMFLOAT4&);
	void CreateSphere(float radius, UINT sliceCount, UINT stackCount, std::vector<XMFLOAT3>* Positions, std::vector<WORD>* Indices, std::vector<XMFLOAT2>* UVs);
private:
	ID3D11VertexShader*				m_pVertexShader = nullptr;
	ID3D11PixelShader*				m_pPixelShader = nullptr;
	ID3D11Buffer*					m_pVertexBuffer = nullptr;
	ID3D11Buffer*					m_pIndexBuffer = nullptr;
	ID3D11Buffer*					m_pCBChangesEveryFrame = nullptr;
	ID3D11InputLayout*				m_pVertexLayout = nullptr;
	XMMATRIX						m_WVP;
	XMFLOAT4						m_ViewPos;
	XMFLOAT3						Position;
	float							Radius;
	std::vector<XMFLOAT3>			Vertices;
	std::vector<WORD>				Indices;
	std::vector<XMFLOAT2>			Tex;
};

