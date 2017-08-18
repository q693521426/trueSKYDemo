#include "DXUT.h"
#include "Sphere.h"


Sphere::Sphere()
{
}


Sphere::~Sphere()
{
}

bool Sphere::Initialize(const XMFLOAT3& pos, float r)
{
	Position = pos;
	Radius = r;
	
	CreateSphere(1.0, 100, 100, &Vertices, &Indices, &Tex);

	return true;
}

void Sphere::Shutdown()
{
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pCBChangesEveryFrame);
	SAFE_RELEASE(m_pVertexLayout);
}

HRESULT Sphere::OnD3D11CreateDevice(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	HRESULT hr;
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pVSBlob = nullptr;
	V_RETURN(DXUTCompileFromFile(L"Sphere.hlsl", nullptr, "VS", "vs_4_0", dwShaderFlags, 0, &pVSBlob));

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
	V_RETURN(DXUTCompileFromFile(L"Sphere.hlsl", nullptr, "PS", "ps_4_0", dwShaderFlags, 0, &pPSBlob));

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	SAFE_RELEASE(pPSBlob);
	if (FAILED(hr))
		return hr;

	SimpleVertex* verts = static_cast<SimpleVertex*>(std::malloc(Vertices.size() * sizeof(SimpleVertex))); //this is some of the data we will pass to the Directx 11 machine to make the vertex buffer

	for (int i = 0; i < Vertices.size(); i++)
	{
		SimpleVertex v;
		v.Pos = Vertices[i];
		XMVECTOR n = XMLoadFloat3(&Vertices[i]);
		XMStoreFloat3(&v.Normal, XMVector3Normalize(n));

		verts[i] = v;
	}

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * Vertices.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = verts;
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &m_pVertexBuffer));

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * Indices.size();
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = Indices.data();
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &m_pIndexBuffer));
	
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = sizeof(CBChangesEveryFrame);
	V_RETURN(pd3dDevice->CreateBuffer(&bd, nullptr, &m_pCBChangesEveryFrame));

	free(verts);
}

void Sphere::Render(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	unsigned int stride;
	unsigned int offset;

	stride = sizeof(SimpleVertex);
	offset = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	V(pd3dImmediateContext->Map(m_pCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	auto pCB = reinterpret_cast<CBChangesEveryFrame*>(MappedResource.pData);
	XMMATRIX translation = XMMatrixTranslation(Position.x, Position.y, Position.z);
	XMMATRIX scale = XMMatrixScaling(Radius,Radius,Radius);
	XMVECTOR det = XMMatrixDeterminant(scale);
	XMStoreFloat4x4(&pCB->mWVP, XMMatrixTranspose(scale*translation*m_WVP));
	pd3dImmediateContext->Unmap(m_pCBChangesEveryFrame, 0);

	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &m_pCBChangesEveryFrame);

	pd3dImmediateContext->IASetInputLayout(m_pVertexLayout);
	pd3dImmediateContext->VSSetShader(m_pVertexShader, nullptr, 0);
	pd3dImmediateContext->PSSetShader(m_pPixelShader, nullptr, 0);

	pd3dImmediateContext->DrawIndexed(Indices.size(), 0, 0);
}



void Sphere::SetWVP(const XMMATRIX& wvp)
{
	m_WVP = wvp;
}

void Sphere::SetViewPos(const XMFLOAT4& viewPos)
{
	m_ViewPos = viewPos;
}

void Sphere::CreateSphere(float radius, UINT sliceCount, UINT stackCount, std::vector<XMFLOAT3>* Positions, std::vector<WORD>* Indices, std::vector<XMFLOAT2>* UVs) //From Frank D. Luna's book but edited a little bit
{
	//
	// Compute the vertices stating at the top pole and moving down the stacks.
	//

	// Poles: note that there will be texture coordinate distortion as there is
	// not a unique point on the texture map to assign to the pole when mapping
	// a rectangular texture onto a sphere.
	XMFLOAT3 topVertex(0.0f, +radius, 0.0f);
	XMFLOAT3 bottomVertex(0.0f, -radius, 0.0f);

	Positions->push_back(topVertex);

	float phiStep = XM_PI / stackCount;
	float thetaStep = 2.0f*XM_PI / sliceCount;

	// Compute vertices for each stack ring (do not count the poles as rings).
	for (UINT i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i*phiStep;

		// Vertices of ring.
		for (UINT j = 0; j <= sliceCount; ++j)
		{
			float theta = j*thetaStep;

			XMFLOAT3 v;

			// spherical to cartesian
			v.x = radius*sinf(phi)*cosf(theta);
			v.y = radius*cosf(phi);
			v.z = radius*sinf(phi)*sinf(theta);

			//// Partial derivative of P with respect to theta
			//v.TangentU.x = -radius*sinf(phi)*sinf(theta);
			//v.TangentU.y = 0.0f;
			//v.TangentU.z = +radius*sinf(phi)*cosf(theta);

			//XMVECTOR T = XMLoadFloat3(&v.TangentU);
			//XMStoreFloat3(&v.TangentU, XMVector3Normalize(T));

			//XMVECTOR p = XMLoadFloat3(&v.Position);
			//XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

			XMFLOAT2 uv;

			uv.x = theta / XM_2PI;
			uv.y = phi / XM_PI;

			Positions->push_back(v);
			UVs->push_back(uv);
		}
	}

	Positions->push_back(bottomVertex);

	//
	// Compute indices for top stack.  The top stack was written first to the vertex buffer
	// and connects the top pole to the first ring.
	//

	for (UINT i = 1; i <= sliceCount; ++i)
	{
		Indices->push_back(0);
		Indices->push_back(i + 1);
		Indices->push_back(i);
	}

	//
	// Compute indices for inner stacks (not connected to poles).
	//

	// Offset the indices to the index of the first vertex in the first ring.
	// This is just skipping the top pole vertex.
	UINT baseIndex = 1;
	UINT ringVertexCount = sliceCount + 1;
	for (UINT i = 0; i < stackCount - 2; ++i)
	{
		for (UINT j = 0; j < sliceCount; ++j)
		{
			Indices->push_back(baseIndex + i*ringVertexCount + j);
			Indices->push_back(baseIndex + i*ringVertexCount + j + 1);
			Indices->push_back(baseIndex + (i + 1)*ringVertexCount + j);

			Indices->push_back(baseIndex + (i + 1)*ringVertexCount + j);
			Indices->push_back(baseIndex + i*ringVertexCount + j + 1);
			Indices->push_back(baseIndex + (i + 1)*ringVertexCount + j + 1);
		}
	}

	//
	// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
	// and connects the bottom pole to the bottom ring.
	//

	// South pole vertex was added last.
	UINT southPoleIndex = (UINT)Positions->size() - 1;

	// Offset the indices to the index of the first vertex in the last ring.
	baseIndex = southPoleIndex - ringVertexCount;

	for (UINT i = 0; i < sliceCount; ++i)
	{
		Indices->push_back(southPoleIndex);
		Indices->push_back(baseIndex + i);
		Indices->push_back(baseIndex + i + 1);
	}
}

