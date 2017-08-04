#pragma once

#include <DirectXMath.h>
#include "Simul/Platform/DirectX11/RenderPlatform.h"

using namespace DirectX;

class CrossPlatformMath
{
public:
	static XMMATRIX XMLoadMatrix(simul::math::Matrix4x4& m)
	{
		XMMATRIX res;
		for (int i = 0; i < 4; ++i)
		{
			XMFLOAT4 tmp;
			tmp.x = m(i * 4, 0);
			tmp.y = m(i * 4, 1);
			tmp.z = m(i * 4, 2);
			tmp.w = m(i * 4, 3);
			res.r[i] = XMLoadFloat4(&tmp);
		}
	}
	static void XMStoreMatrix(simul::math::Matrix4x4& Dst, XMMATRIX m)
	{
		float* f = new float[16]();
		for (int i = 0; i < 4; ++i)
		{
			XMVECTOR v = m.r[i];
			XMFLOAT4 tmp;
			XMStoreFloat4(&tmp,v);
			f[i * 4] = tmp.x;
			f[i * 4 + 1] = tmp.y;
			f[i * 4 + 2] = tmp.z;
			f[i * 4 + 3] = tmp.w;
		}
		Dst.Set(f);
		delete[] f;
	}
};