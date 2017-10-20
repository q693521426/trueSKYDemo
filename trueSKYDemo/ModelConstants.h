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

struct DirectionalLight
{
	DirectionalLight(){ ZeroMemory(this,sizeof(*this)); }
	
	D3DXVECTOR4 Ambient;
	D3DXVECTOR4 Diffuse;
	D3DXVECTOR4 Specular;
	D3DXVECTOR3 Direction;
	float Pad;
};

struct PointLight
{
	PointLight() { ZeroMemory(this,sizeof(*this)); }

	D3DXVECTOR4 Ambient;
	D3DXVECTOR4 Diffuse;
	D3DXVECTOR4 Specular;

	D3DXVECTOR3 Position;
	float Range;

	D3DXVECTOR3 Att;
	float Pad;
};

struct SpotLight
{
	SpotLight() { ZeroMemory(this,sizeof(*this)); }

	D3DXVECTOR4 Ambient;
	D3DXVECTOR4 Diffuse;
	D3DXVECTOR4 Specular;

	D3DXVECTOR3 Position;
	float Range;

	D3DXVECTOR3 Direction;
	float Spot;

	D3DXVECTOR3 Att;
	float Pad;
};
struct FrustumBuffer
{
	D3DXVECTOR4 ViewPos;
};

struct CameraSqFile
{
	float FieldOfViewDegrees;
	float PosKm[3];
	float Quaternion[4];
};

static HRESULT CompileShader( _In_ LPCWSTR srcFile, _In_ LPCSTR entryPoint, _In_ LPCSTR profile, _Outptr_ ID3DBlob** blob )
{
	if ( !srcFile || !entryPoint || !profile || !blob )
       return E_INVALIDARG;

    *blob = nullptr;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    flags |= D3DCOMPILE_DEBUG;
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    const D3D_SHADER_MACRO defines[] = 
    {
        "EXAMPLE_DEFINE", "1",
        NULL, NULL
    };

    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3DX11CompileFromFile  ( srcFile, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                     entryPoint, profile,
									 flags, 0, nullptr,&shaderBlob, &errorBlob,nullptr);
    if ( FAILED(hr) )
    {
        if ( errorBlob )
        {
            OutputDebugStringA( (char*)errorBlob->GetBufferPointer() );
            errorBlob->Release();
        }

        if ( shaderBlob )
           shaderBlob->Release();

        return hr;
    }    

    *blob = shaderBlob;

    return hr;
}