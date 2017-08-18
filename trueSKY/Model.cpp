#include "DXUT.h"
#include "Model.h"


Model::Model()
{
}


Model::~Model()
{
}

bool Model::Initialize()
{
#if LOAD_MODEL
	m_files[0] = new char[256]
	{
		"./Resource/sulan2.fbx"
	};
#endif

	m_Light.LightPos = XMFLOAT4(10.0f, 3.0f, 1.0f, 1.0f);
	m_Light.LightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_Light.Ambient = XMFLOAT4(0.05f, 0.05f, 0.05f, 1.0f);
	m_Light.Diffuse = XMFLOAT4(0.70f, 0.70f, 0.70f, 1.0f);
	m_Light.Specular = XMFLOAT4(0.70f, 0.70f, 0.70f, 0.70f);
	m_Light.Constant = 1.0f;
	m_Light.Linear = 0.009f;
	m_Light.Quadratic = 0.0032f;

#if LIGHT_SPHERE
	m_LightSphere = new Sphere();
	m_LightSphere->Initialize(XMFLOAT3(m_Light.LightPos.x, m_Light.LightPos.y, m_Light.LightPos.z), 0.2);
#endif
	return true;
}

void Model::Shutdown()
{
#if LIGHT_SPHERE
	m_LightSphere->Shutdown();
#endif

#if LOAD_MODEL
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pTextureRV);
	SAFE_RELEASE(m_pSamplerLinear);
	SAFE_RELEASE(m_pCBChangesEveryFrame);
	SAFE_RELEASE(m_pLightBuffer);
	SAFE_RELEASE(m_pFrustumBuffer);
	for (int i = 0; i < NUMBER_OF_MODELS; i++)
	{
		if (m_pFbxDX11[i])
			delete m_pFbxDX11[i];
		m_pFbxDX11[i] = nullptr;
	}
#endif
}

HRESULT Model::OnD3D11CreateDevice(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
#if LIGHT_SPHERE
	m_LightSphere->OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext);
#endif

#if LOAD_MODEL
	HRESULT hr = S_OK;

	for (DWORD i = 0; i < NUMBER_OF_MODELS; i++)
	{
		m_pFbxDX11[i] = new FBX_LOADER::CFBXRenderDX11;
		hr = m_pFbxDX11[i]->LoadFBX(m_files[i], pd3dDevice, pd3dImmediateContext);
	}
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"FBX Error", L"Error", MB_OK);
		return hr;
	}

	for (int i = 0; i < NUMBER_OF_MODELS; i++)
	{
		if (m_files[i])
			delete m_files[i];
		m_files[i] = nullptr;
	}

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pVSBlob = nullptr;
	V_RETURN(DXUTCompileFromFile(L"LoadModel.hlsl", nullptr, "VS", "vs_4_0", dwShaderFlags, 0, &pVSBlob));

	hr = pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader);
	if (FAILED(hr))
	{
		SAFE_RELEASE(pVSBlob);
		return hr;
	}

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);
	for (DWORD i = 0; i < NUMBER_OF_MODELS; ++i)
	{
		hr = m_pFbxDX11[i]->CreateInputLayout(pd3dDevice, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), layout, numElements);
	}
	SAFE_RELEASE(pVSBlob);
	if (FAILED(hr))
		return hr;

	ID3DBlob* pPSBlob = nullptr;
	V_RETURN(DXUTCompileFromFile(L"LoadModel.hlsl", nullptr, "PS", "ps_4_0", dwShaderFlags, 0, &pPSBlob));

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	SAFE_RELEASE(pPSBlob);
	if (FAILED(hr))
		return hr;

	// Create the constant buffers
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = sizeof(CBChangesEveryFrame);
	V_RETURN(pd3dDevice->CreateBuffer(&bd, nullptr, &m_pCBChangesEveryFrame));

	bd.ByteWidth = sizeof(LightBuffer);
	V_RETURN(pd3dDevice->CreateBuffer(&bd, NULL, &m_pLightBuffer));

	bd.ByteWidth = sizeof(FrustumBuffer);
	V_RETURN(pd3dDevice->CreateBuffer(&bd, NULL, &m_pFrustumBuffer));

	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	V_RETURN(pd3dDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinear));
#endif
}

void Model::Render(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
#if LOAD_MODEL
	for (DWORD i = 0; i < NUMBER_OF_MODELS; i++)
	{
		size_t nodeCount = m_pFbxDX11[i]->GetNodeCount();

		pd3dImmediateContext->VSSetShader(m_pVertexShader, NULL, 0);
		pd3dImmediateContext->VSSetConstantBuffers(0, 1, &m_pCBChangesEveryFrame);
		pd3dImmediateContext->PSSetShader(m_pPixelShader, NULL, 0);

		HRESULT hr;
		D3D11_MAPPED_SUBRESOURCE MappedResource;

		V(pd3dImmediateContext->Map(m_pLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
		auto  LB = reinterpret_cast<LightBuffer*>(MappedResource.pData);
		*LB = m_Light;
		pd3dImmediateContext->Unmap(m_pLightBuffer, 0);

		V(pd3dImmediateContext->Map(m_pFrustumBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
		auto  FB = reinterpret_cast<FrustumBuffer*>(MappedResource.pData);
		FB->ViewPos = m_ViewPos;
		pd3dImmediateContext->Unmap(m_pFrustumBuffer, 0);

		pd3dImmediateContext->PSSetConstantBuffers(2, 1, &m_pLightBuffer);
		pd3dImmediateContext->PSSetConstantBuffers(3, 1, &m_pFrustumBuffer);


		for (int j = 0; j < nodeCount; j++)
		{
			bool flag = false;
			XMMATRIX m_Local;
			m_pFbxDX11[i]->GetNodeMatrix(j, &m_Local.r[0].m128_f32[0]);

			V(pd3dImmediateContext->Map(m_pCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
			auto pCB = reinterpret_cast<CBChangesEveryFrame*>(MappedResource.pData);
			
			XMMATRIX change(1.f, 0.f, 0.f, 0.f,
					0.f, 0.f, 1.f, 0.f,
					0.f, 1.f, 0.f, 0.f,
					0.f, 0.f, 0.f, 1.f);
			XMMATRIX translation = XMMatrixTranslation(40, 0, 545);
			m_Local = m_Local * change;
			m_Local = m_Local * translation;

			XMStoreFloat4x4(&pCB->mWorld, XMMatrixTranspose(m_Local));
			XMStoreFloat4x4(&pCB->mWVP, XMMatrixTranspose(m_Local * m_WVP));
			pd3dImmediateContext->Unmap(m_pCBChangesEveryFrame, 0);

			//SetModelInstancingMatrix(pd3dImmediateContext);

			FBX_LOADER::MATERIAL_DATA material = m_pFbxDX11[i]->GetNodeMaterial(j);

			if (material.pMaterialCb)
				pd3dImmediateContext->UpdateSubresource(material.pMaterialCb, 0, NULL, &material.materialConstantData, 0, 0);

			pd3dImmediateContext->PSSetShaderResources(0, 1, &material.pSRV);
			pd3dImmediateContext->PSSetConstantBuffers(1, 1, &material.pMaterialCb);
			pd3dImmediateContext->PSSetSamplers(0, 1, &material.pSampler);

			m_pFbxDX11[i]->RenderNode(pd3dImmediateContext, j);
		}
	}
#endif
#if LIGHT_SPHERE
	m_LightSphere->SetWVP(m_WVP);
	m_LightSphere->Render(pd3dDevice, pd3dImmediateContext);
#endif
}

void Model::Resize(const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
#if LIGHT_SPHERE
	m_LightSphere->SetWVP(m_WVP);
#endif
}


void Model::SetWVP(const XMMATRIX& wvp)
{
	m_WVP = wvp;
}


void Model::SetViewPos(const XMFLOAT4& viewPos)
{
	m_ViewPos = viewPos;
}

void Model::SetLight()
{

}