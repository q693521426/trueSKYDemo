#include "Mesh.h"

#include "Utilities.h"
#include "Simul/Platform/CrossPlatform/SL/CppSl.hs"
#include "Simul/Platform/CrossPlatform/DeviceContext.h"
#include "Simul/Platform/CrossPlatform/Effect.h"
using namespace simul;
using namespace orbis;

Mesh::Mesh()
	:vertexBuffer(NULL)
	,indexBuffer(NULL)
	,inputLayout(NULL)
	,stride(0)
	,numVertices(0)
	,numIndices(0)
{
}

Mesh::~Mesh()
{
	InvalidateDeviceObjects();
}
void Mesh::InvalidateDeviceObjects()
{
//	releaseBuffers();
}

bool Mesh::Initialize(crossplatform::RenderPlatform *renderPlatform, int lPolygonVertexCount, const float *lVertices, const float *lNormals, const float *lUVs, int lPolygonCount, const unsigned int *lIndices)
{
	return true;
}
void Mesh::GetVertices(void *target, void *indices)
{
}

void Mesh::UpdateVertexPositions(int lVertexCount, float *lVertices) const
{
}

void Mesh::BeginDraw(crossplatform::DeviceContext &deviceContext,crossplatform::ShadingMode pShadingMode) const
{
}
// Draw all the faces with specific material with given shading mode.
void Mesh::Draw(crossplatform::DeviceContext &deviceContext,int pMaterialIndex,crossplatform::ShadingMode pShadingMode) const
{
}
// Unbind buffers, reset vertex arrays, turn off lighting and texture.
void Mesh::EndDraw(crossplatform::DeviceContext &deviceContext) const
{
}