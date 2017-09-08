#pragma once
#ifndef RENDERSTATES_H_
#define RENDERSTATES_H_

class RenderStates
{
public:
	RenderStates(void);
	~RenderStates(void);

	static HRESULT Initialize(ID3D11Device* device);
	static void Release();
	
};

#endif