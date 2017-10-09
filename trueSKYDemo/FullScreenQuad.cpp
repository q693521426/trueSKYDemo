#include "DXUT.h"
#include "FullScreenQuad.h"


FullScreenQuad::FullScreenQuad():
	m_vertexBuffer(nullptr),
	m_indexBuffer(nullptr),
	m_pVertexShader(nullptr),
	m_pPixelShader(nullptr),
	m_pVertexLayout(nullptr),
	m_pSamplerCCC(nullptr),
	m_pCBChangesEveryFrame(nullptr),
	m_pd3dDevice(nullptr),
	m_pd3dImmediateContext(nullptr)
{
}


FullScreenQuad::~FullScreenQuad()
{
}

bool FullScreenQuad::Resize(int screen_width,int screen_height)
{
	float left, right, top, bottom;
	SimpleVertex* vertices;
	DWORD* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int i;

	// Calculate the screen coordinates of the left side of the window.
	left = (float)((screen_width / 2) * -1);

	// Calculate the screen coordinates of the right side of the window.
	right = left + (float)screen_width;

	// Calculate the screen coordinates of the top of the window.
	top = (float)(screen_height / 2);

	// Calculate the screen coordinates of the bottom of the window.
	bottom = top - (float)screen_height;

	// Set the number of vertices in the vertex array.
	m_vertexCount = 6;

	// Set the number of indices in the index array.
	m_indexCount = m_vertexCount;

	// Create the vertex array.
	vertices = new SimpleVertex[m_vertexCount];
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new DWORD[m_indexCount];
	if (!indices)
	{
		return false;
	}

	// Load the vertex array with data.
	// First triangle.
	vertices[0].Pos = D3DXVECTOR3(left, top, 0.0f);  // Top left.
	vertices[0].Tex = D3DXVECTOR2(0.0f, 0.0f);

	vertices[1].Pos = D3DXVECTOR3(right, bottom, 0.0f);  // Bottom right.
	vertices[1].Tex = D3DXVECTOR2(1.0f, 1.0f);

	vertices[2].Pos = D3DXVECTOR3(left, bottom, 0.0f);  // Bottom left.
	vertices[2].Tex = D3DXVECTOR2(0.0f, 1.0f);

	// Second triangle.
	vertices[3].Pos = D3DXVECTOR3(left, top, 0.0f);  // Top left.
	vertices[3].Tex = D3DXVECTOR2(0.0f, 0.0f);

	vertices[4].Pos = D3DXVECTOR3(right, top, 0.0f);  // Top right.
	vertices[4].Tex = D3DXVECTOR2(1.0f, 0.0f);

	vertices[5].Pos = D3DXVECTOR3(right, bottom, 0.0f);  // Bottom right.
	vertices[5].Tex = D3DXVECTOR2(1.0f, 1.0f);

	// Load the index array with data.
	for (i = 0; i<m_indexCount; i++)
	{
		indices[i] = i;
	}

	// Set up the description of the vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(SimpleVertex) * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now finally create the vertex buffer.
	result = m_pd3dDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Set up the description of the index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(DWORD) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = m_pd3dDevice->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	D3DXMatrixIdentity(&m_World);
	
	D3DXVECTOR3 m_Eye(0.0f, 0.0f, -1.0f);
	D3DXVECTOR3 m_At(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 m_Up(0.0f, 1.0f, 0.0f);
	D3DXMATRIX m_View,m_Project;
	D3DXMatrixLookAtLH(&m_View,&m_Eye,&m_At,&m_Up);
	D3DXMatrixOrthoLH(&m_Project,static_cast<float>(screen_width), static_cast<float>(screen_height), 0.1f, 100.f);

	m_WVP = m_World * m_View * m_Project;
	
	return true;
}

void FullScreenQuad::Release()
{
	m_pd3dDevice = nullptr;
	m_pd3dImmediateContext = nullptr;
	SAFE_RELEASE(m_vertexBuffer);
	SAFE_RELEASE(m_indexBuffer);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pVertexLayout);
	SAFE_RELEASE(m_pCBChangesEveryFrame);
	SAFE_RELEASE(m_pSamplerCCC);
	SAFE_RELEASE(m_pBlendState);
}

HRESULT FullScreenQuad::OnD3D11CreateDevice(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	HRESULT hr;

	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;

	ID3DBlob* pVSBlob = nullptr;
	V_RETURN(CompileShader(L"Shader/Quad.hlsl","VS", "vs_4_0",&pVSBlob));

	hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader);
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
	hr = m_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &m_pVertexLayout);
	SAFE_RELEASE(pVSBlob);
	if (FAILED(hr))
		return hr;

	ID3DBlob* pPSBlob = nullptr;
	V_RETURN(CompileShader(L"Shader/Quad.hlsl",  "PS", "ps_4_0",&pPSBlob));

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	SAFE_RELEASE(pPSBlob);
	if (FAILED(hr))
		return hr;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = sizeof(CBChangesEveryFrame);

	V_RETURN(m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pCBChangesEveryFrame));
	
	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	V_RETURN(m_pd3dDevice->CreateSamplerState(&sampDesc, &m_pSamplerCCC));

	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc,sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = true;
	for(int i=0;i<8;i++)
	{
		blendDesc.RenderTarget[i].BlendEnable = false;
		blendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	}

	V_RETURN(m_pd3dDevice->CreateBlendState(&blendDesc, &m_pBlendState));

	return hr;
}

void FullScreenQuad::Render(ID3D11RenderTargetView* RTV,ID3D11ShaderResourceView* SRV)
{
	//float BlendFactor[4] = {0.0f,0.0f,0.0f,0.0f};
	//m_pd3dImmediateContext->OMSetBlendState(m_pBlendState,BlendFactor,0xffffffff);
	if(RTV!=nullptr)
		m_pd3dImmediateContext->OMSetRenderTargets(1,&RTV,nullptr);

	unsigned int stride;
	unsigned int offset;

	stride = sizeof(SimpleVertex);
	offset = 0;

	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	m_pd3dImmediateContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	V(m_pd3dImmediateContext->Map(m_pCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	auto pCB = reinterpret_cast<CBChangesEveryFrame*>(MappedResource.pData);
	D3DXMatrixTranspose(&pCB->mWorld,&m_World);
	D3DXMatrixTranspose(&pCB->mWVP,&m_WVP);
	m_pd3dImmediateContext->Unmap(m_pCBChangesEveryFrame, 0);

	m_pd3dImmediateContext->VSSetConstantBuffers(0, 1, &m_pCBChangesEveryFrame);

	m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout);
	m_pd3dImmediateContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pd3dImmediateContext->PSSetShader(m_pPixelShader, nullptr, 0);

	m_pd3dImmediateContext->PSSetShaderResources(0, 1, &SRV);
	m_pd3dImmediateContext->PSSetSamplers(0, 1, &m_pSamplerCCC);

	m_pd3dImmediateContext->DrawIndexed(m_indexCount, 0, 0);
}