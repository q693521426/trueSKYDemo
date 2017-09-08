#pragma once
#include "DXUT.h"
#include "SDKmisc.h"
#include <d3dcompiler.h>
#include <malloc.h>
#pragma warning( disable : 4100 )
#pragma warning( disable : 4324 )
struct SimpleVertex
{
	D3DXVECTOR3 Pos;
	D3DXVECTOR3 Normal;
	D3DXVECTOR2 Tex;
};

struct Material
{
    D3DXVECTOR4 Ambient;
    D3DXVECTOR4 Diffuse;
    D3DXVECTOR3 Specular;
    float Power;
    D3DXVECTOR4 Emisive;
};

struct CBChangesEveryFrame
{
	D3DXMATRIX mWorld;
	D3DXMATRIX mWVP;
};

struct LightBuffer
{
	D3DXVECTOR4 LightPos;
	D3DXVECTOR4 LightColor;

	D3DXVECTOR4 Ambient;
	D3DXVECTOR4 Diffuse;
	D3DXVECTOR4 Specular;

	float Constant;
	float Linear;
	float Quadratic;

	float Lpad;
};

struct FrustumBuffer
{
	D3DXVECTOR4 ViewPos;
};

HRESULT CompileShader( _In_ LPCWSTR srcFile, _In_ LPCSTR entryPoint, _In_ LPCSTR profile, _Outptr_ ID3DBlob** blob );