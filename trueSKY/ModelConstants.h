#pragma once
#include "DXUT.h"
#include "SDKmisc.h"

#pragma warning( disable : 4100 )

using namespace DirectX;

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Tex;
};

struct CBChangesEveryFrame
{
	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mWVP;
};

struct LightBuffer
{
	XMFLOAT4 LightPos;
	XMFLOAT4 LightColor;

	XMFLOAT4 Ambient;
	XMFLOAT4 Diffuse;
	XMFLOAT4 Specular;

	float Constant;
	float Linear;
	float Quadratic;

	float Lpad;
};

struct FrustumBuffer
{
	XMFLOAT4 ViewPos;
};