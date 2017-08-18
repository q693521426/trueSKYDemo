#include "DXUT.h"
#include "FullScreenQuad.h"


FullScreenQuad::FullScreenQuad()
{
}


FullScreenQuad::~FullScreenQuad()
{
}

bool FullScreenQuad::Initialize(ID3D11Device* device,int screen_width,int screen_height)
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
	vertices[0].Pos = XMFLOAT3(left, top, 0.0f);  // Top left.
	vertices[0].Tex = XMFLOAT2(0.0f, 0.0f);

	vertices[1].Pos = XMFLOAT3(right, bottom, 0.0f);  // Bottom right.
	vertices[1].Tex = XMFLOAT2(1.0f, 1.0f);

	vertices[2].Pos = XMFLOAT3(left, bottom, 0.0f);  // Bottom left.
	vertices[2].Tex = XMFLOAT2(0.0f, 1.0f);

	// Second triangle.
	vertices[3].Pos = XMFLOAT3(left, top, 0.0f);  // Top left.
	vertices[3].Tex = XMFLOAT2(0.0f, 0.0f);

	vertices[4].Pos = XMFLOAT3(right, top, 0.0f);  // Top right.
	vertices[4].Tex = XMFLOAT2(1.0f, 0.0f);

	vertices[5].Pos = XMFLOAT3(right, bottom, 0.0f);  // Bottom right.
	vertices[5].Tex = XMFLOAT2(1.0f, 1.0f);

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
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
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
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	m_World = XMMatrixIdentity();
	
	XMMATRIX m_View = XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f),
		XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	XMMATRIX m_Project = XMMatrixOrthographicLH(screen_width, screen_height, 0.1f, 100.f);

	m_WVP = m_World * m_View * m_Project;

	return true;
}

void FullScreenQuad::Shutdown()
{
	SAFE_RELEASE(m_vertexBuffer);
	SAFE_RELEASE(m_indexBuffer);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pVertexLayout);
	SAFE_RELEASE(m_pCBChangesEveryFrame);
	SAFE_RELEASE(g_pSamplerCCC);
}

HRESULT FullScreenQuad::OnD3D11CreateDevice(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	HRESULT hr;
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pVSBlob = nullptr;
	V_RETURN(DXUTCompileFromFile(L"Quad.hlsl", nullptr, "VS", "vs_4_0", dwShaderFlags, 0, &pVSBlob));

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
	hr = pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &m_pVertexLayout);
	SAFE_RELEASE(pVSBlob);
	if (FAILED(hr))
		return hr;

	ID3DBlob* pPSBlob = nullptr;
	V_RETURN(DXUTCompileFromFile(L"Quad.hlsl", nullptr, "PS", "ps_4_0", dwShaderFlags, 0, &pPSBlob));

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	SAFE_RELEASE(pPSBlob);
	if (FAILED(hr))
		return hr;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = sizeof(CBChangesEveryFrame);

	V_RETURN(pd3dDevice->CreateBuffer(&bd, nullptr, &m_pCBChangesEveryFrame));
	
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
	V_RETURN(pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerCCC));
}

void FullScreenQuad::Render(ID3D11Device* device,ID3D11DeviceContext* deviceContext,ID3D11ShaderResourceView* SRV)
{
	unsigned int stride;
	unsigned int offset;

	stride = sizeof(SimpleVertex);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	V(deviceContext->Map(m_pCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	auto pCB = reinterpret_cast<CBChangesEveryFrame*>(MappedResource.pData);
	XMStoreFloat4x4(&pCB->mWorld, XMMatrixTranspose(m_World));
	XMStoreFloat4x4(&pCB->mWVP, XMMatrixTranspose(m_WVP));
	deviceContext->Unmap(m_pCBChangesEveryFrame, 0);

	deviceContext->VSSetConstantBuffers(0, 1, &m_pCBChangesEveryFrame);

	deviceContext->IASetInputLayout(m_pVertexLayout);
	deviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
	deviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

	deviceContext->PSSetShaderResources(0, 1, &SRV);
	deviceContext->PSSetSamplers(0, 1, &g_pSamplerCCC);

	deviceContext->DrawIndexed(m_indexCount, 0, 0);
}