#pragma once
#ifndef RENDERSTATES_H_
#define RENDERSTATES_H_

class RenderStates
{
public:
	RenderStates();
	~RenderStates();

	static HRESULT Initialize(ID3D11Device* device);
	static void Release();
	
	static ID3D11RasterizerState* CullClockWiseRS;
	static ID3D11RasterizerState* CullCounterClockWiseRS;
	static ID3D11DepthStencilState*	OnDepthStencilState;
	static ID3D11DepthStencilState*	OffDepthStencilState;
};

#endif