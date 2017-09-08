#include "Utilities.h"
#include "simul/sky/Float4.h"
#include "simul/Math/Vector3.h"
#include "simul/Math/Matrix4x4.h"
#include "ShaderLoader.h"
#include "PSSL/CppPssl.h"
#include "Simul/Geometry/Orientation.h"
#include "Simul/Base/DefaultFileLoader.h"
#include "Simul/Base/RuntimeError.h"
#include "Simul/Platform/CrossPlatform/RenderPlatform.h"
#include "Simul/Platform/PS4/Render/RenderPlatform.h"
#include "gnf.h"
#include "PSSL/copy_constant_buffer.sl"
#include <string>

using namespace simul;
using namespace orbis;
using namespace std;

#define  SCRATCH_SIZE 4096


void syncOnRt(sce::Gnmx::LightweightGfxContext *gfxc)
{
	//gfxc->submit();
	volatile uint64_t* label = (volatile uint64_t*)gfxc->allocateFromCommandBuffer( sizeof(uint64_t), sce::Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
	*label = 0x0; // set the memory to have the val 0
	gfxc->writeImmediateAtEndOfPipe(sce::Gnm::kEopFlushCbDbCaches,(void *)label,0x1,sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2);
	gfxc->submit();
	//gfxc->waitOnAddress((void*)label, 0xffffffff,sce::Gnm::kWaitCompareFuncEqual, 0x1);
	while(*label != 0x1) {} 
}

void waitForComputeShader( sce::Gnmx::LightweightGfxContext *gfxc )
{
	volatile uint64_t* label = (volatile uint64_t*)gfxc->allocateFromCommandBuffer( sizeof(uint64_t), sce::Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
	*label = 0x0; // set the memory to have the val 0
	gfxc->writeAtEndOfShader( sce::Gnm::kEosCsDone, const_cast<uint64_t*>(label), 0x1 ); // tell the CP to write a 1 into the memory only when all compute shaders have finished
	gfxc->submit();
	//gfxc->waitOnAddress((void*)label, 0xffffffff,sce::Gnm::kWaitCompareFuncEqual, 0x1);
	while(*label != 0x1) {} 
}

static simul::base::DefaultFileLoader fl;
static simul::base::FileLoader *fileLoader=&fl;

namespace simul
{
	namespace orbis
	{
		void SetFileLoader(simul::base::FileLoader *l)
		{
			fileLoader=l;
		}
	}
}


void graphicsWaitForCompute( sce::Gnmx::LightweightGfxContext *gfxc )
{
	volatile uint64_t* label = (volatile uint64_t*)gfxc->allocateFromCommandBuffer( sizeof(uint64_t), sce::Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
	*label = 0x0; // set the memory to have the val 0
	gfxc->writeAtEndOfShader( sce::Gnm::kEosCsDone, const_cast<uint64_t*>(label), 0x1 ); // tell the CP to write a 1 into the memory only when all compute shaders have finished
	gfxc->waitOnAddress( const_cast<uint64_t*>(label), 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1
	gfxc->flushShaderCachesAndWait(sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 0
		//,sce::Gnm::kStallCommandBufferParserDisable); // This is what the Toolkit uses. What the hell does it mean?
		, sce::Gnm::kStallCommandBufferParserEnable ); // tell the CP to flush the L1$ and L2$
}
#define EVO_SYNC
void computeWaitForGraphics( sce::Gnmx::LightweightGfxContext *gfxc )
{
	volatile uint64_t* label = (volatile uint64_t*)gfxc->allocateFromCommandBuffer(sizeof(uint64_t), sce::Gnm::kEmbeddedDataAlignment8); // allocate memory from the command buffer
	*label = 0x0; // set the memory to have the val 0
	gfxc->writeAtEndOfShader( sce::Gnm::kEosPsDone, const_cast<uint64_t*>(label), 0x1 );
#ifdef EVO_SYNC
	gfxc->waitOnAddress(const_cast<uint64_t*>(label), 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 0x1 );
	gfxc->triggerEvent(sce::Gnm::kEventTypeFlushAndInvalidateCbPixelData);
	gfxc->triggerEvent(sce::Gnm::kEventTypeDbCacheFlushAndInvalidate); 
#else
	gfxc->writeAtEndOfPipe(
		sce::Gnm::kEopFlushCbDbCaches, 
		sce::Gnm::kEventWriteDestMemory, const_cast<uint64_t*>(label),
		sce::Gnm::kEventWriteSource64BitsImmediate, 0x1,
		sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, sce::Gnm::kCachePolicyLru
	);
	gfxc->waitOnAddress(const_cast<uint64_t*>(label), 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 0x1); // tell the CP to wait until the memory has the val 1
#endif
	//gfxc->validate();
}


simul::base::MemoryInterface	*Utilities::allocator=NULL;

void Utilities::Init(simul::base::MemoryInterface	*a)
{
	allocator=a;
}

void Utilities::RestoreDeviceObjects(void *g)
{
	RecompileShaders();
}

void Utilities::RecompileShaders()
{	
}

void Utilities::InvalidateDeviceObjects()
{
	ClearShaders();
}

void Utilities::ClearShaders()
{

}


int Utilities::screenWidth;
int Utilities::screenHeight;
void Utilities::DrawQuad(void *context,int X,int Y,int W,int H)
{
	DrawQuad(context,(float)X/(float)screenWidth,(float)(screenHeight-Y-H)/(float)screenHeight,(float)W/(float)screenWidth,(float)H/(float)screenHeight);
}

void Utilities::SetScreenSize(int W,int H)
{
	screenWidth=W;
	screenHeight=H;
}
class SimpleMeshVertex
{
public:
	vec3 m_position;
	vec3 m_normal;
	vec4 m_tangent;
	vec2 m_texture;
};

void SetMeshVertexBufferFormat(sce::Gnm::Buffer* buffer, SimpleMesh *destMesh, const MeshVertexBufferElement* element, uint32_t elements )
{
	while( elements-- )
	{
		switch( *element++ )
		{
		case kPosition:
			buffer->initAsVertexBuffer(static_cast<uint8_t*>(destMesh->m_vertexBuffer) + offsetof(SimpleMeshVertex,m_position), sce::Gnm::kDataFormatR32G32B32Float, sizeof(SimpleMeshVertex), destMesh->m_vertexCount);
			break;
		case kNormal:
			buffer->initAsVertexBuffer(static_cast<uint8_t*>(destMesh->m_vertexBuffer) + offsetof(SimpleMeshVertex,m_normal), sce::Gnm::kDataFormatR32G32B32Float, sizeof(SimpleMeshVertex), destMesh->m_vertexCount);
			break;
		case kTangent:
			buffer->initAsVertexBuffer(static_cast<uint8_t*>(destMesh->m_vertexBuffer) + offsetof(SimpleMeshVertex,m_tangent), sce::Gnm::kDataFormatR32G32B32A32Float, sizeof(SimpleMeshVertex), destMesh->m_vertexCount);
			break;
		case kTexture:
			buffer->initAsVertexBuffer(static_cast<uint8_t*>(destMesh->m_vertexBuffer) + offsetof(SimpleMeshVertex,m_texture), sce::Gnm::kDataFormatR32G32Float, sizeof(SimpleMeshVertex), destMesh->m_vertexCount);
			break;
		}
		++buffer;
	}
}

void SetMeshVertexBufferFormat( SimpleMesh *destMesh )
{
	const MeshVertexBufferElement element[] = { kPosition, kNormal, kTangent, kTexture };
	const uint32_t elements = sizeof(element)/sizeof(element[0]);
	SetMeshVertexBufferFormat( destMesh->m_buffer, destMesh, element, elements );
	destMesh->m_vertexAttributeCount = elements;
}

void SimpleMesh::SetVertexBuffer(sce::Gnmx::LightweightGfxContext *gfxc,sce::Gnm::ShaderStage stage )
{
	SCE_GNM_ASSERT( m_vertexAttributeCount < kMaximumVertexBufferElements );
	gfxc->setVertexBuffers(stage, 0, m_vertexAttributeCount, m_buffer);
}

void Utilities::BuildCubeMesh(simul::base::MemoryInterface* allocator,SimpleMesh *destMesh, float side)
{
	SIMUL_ASSERT(0);  // mem leak unless we destroy the buffers later.
	destMesh->m_vertexCount = 24;

	destMesh->m_vertexStride = sizeof(SimpleMeshVertex);

	destMesh->m_vertexAttributeCount = 4;
	destMesh->m_indexCount = 36;
	destMesh->m_indexType = sce::Gnm::kIndexSize16;
	destMesh->m_primitiveType = sce::Gnm::kPrimitiveTypeTriList;

	destMesh->m_vertexBufferSize = destMesh->m_vertexCount * sizeof(SimpleMeshVertex);
	destMesh->m_indexBufferSize = destMesh->m_indexCount * sizeof(uint16_t);

	destMesh->m_vertexBuffer = static_cast<uint8_t*>(allocator->Allocate(destMesh->m_vertexBufferSize, sce::Gnm::kAlignmentOfBufferInBytes));
	destMesh->m_indexBuffer = static_cast<uint8_t*>(allocator->Allocate(destMesh->m_indexBufferSize, sce::Gnm::kAlignmentOfBufferInBytes));

	memset(destMesh->m_vertexBuffer, 0, destMesh->m_vertexBufferSize);
	memset(destMesh->m_indexBuffer, 0, destMesh->m_indexBufferSize);

	SetMeshVertexBufferFormat( destMesh );

	// Everything else is just filling in the vertex and index buffer.
	SimpleMeshVertex *outV = static_cast<SimpleMeshVertex*>(destMesh->m_vertexBuffer);
	uint16_t *outI = static_cast<uint16_t*>(destMesh->m_indexBuffer);

	const float halfSide = side*0.5f;
	const SimpleMeshVertex verts[] = {
		{{-halfSide, -halfSide, -halfSide}, { -1.0000000, 0.00000000, 0.00000000 }, { 0.00000000, 0.00000000, 1.0000000, 1.0000000 }, { 0, 0 } }, // 0
		{{-halfSide, -halfSide, -halfSide}, { 0.00000000, -1.0000000, 0.00000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 0, 0 } }, // 1
		{{-halfSide, -halfSide, -halfSide}, { 0.00000000, 0.00000000, -1.0000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 1, 0 } }, // 2
		{{-halfSide, -halfSide, halfSide}, { -1.0000000, 0.00000000, 0.00000000 }, { 0.00000000, 0.00000000, 1.0000000, 1.0000000 }, { 0, 1 } }, // 3
		{{-halfSide, -halfSide, halfSide}, { 0.00000000, -1.0000000, 0.00000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 1, 0 } }, // 4
		{{-halfSide, -halfSide, halfSide}, { 0.00000000, 0.00000000, 1.0000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 0, 0 } }, // 5
		{{-halfSide, halfSide, -halfSide}, { -1.0000000, 0.00000000, 0.00000000 }, { 0.00000000, 0.00000000, 1.0000000, 1.0000000 }, { 1, 0 } }, // 6
		{{-halfSide, halfSide, -halfSide}, { 0.00000000, 0.00000000, -1.0000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 0, 0 } }, // 7
		{{-halfSide, halfSide, -halfSide}, { 0.00000000, 1.0000000, 0.00000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 1, 0 } }, // 8
		{{-halfSide, halfSide, halfSide}, { -1.0000000, 0.00000000, 0.00000000 }, { 0.00000000, 0.00000000, 1.0000000, 1.0000000 }, { 1, 1 } }, // 9
		{{-halfSide, halfSide, halfSide}, { 0.00000000, 0.00000000, 1.0000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 1, 0 } }, // 10
		{{-halfSide, halfSide, halfSide}, { 0.00000000, 1.0000000, 0.00000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 0, 0 } }, // 11
		{{halfSide, -halfSide, -halfSide}, { 0.00000000, -1.0000000, 0.00000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 0, 1 } }, // 12
		{{halfSide, -halfSide, -halfSide}, { 0.00000000, 0.00000000, -1.0000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 1, 1 } }, // 13
		{{halfSide, -halfSide, -halfSide}, { 1.0000000, 0.00000000, 0.00000000 }, { 0.00000000, 0.00000000, -1.0000000, 1.0000000 }, { 0, 1 } }, // 14
		{{halfSide, -halfSide, halfSide}, { 0.00000000, -1.0000000, 0.00000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 1, 1 } }, // 15
		{{halfSide, -halfSide, halfSide}, { 0.00000000, 0.00000000, 1.0000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 0, 1 } }, // 16
		{{halfSide, -halfSide, halfSide}, { 1.0000000, 0.00000000, 0.00000000 }, { 0.00000000, 0.00000000, -1.0000000, 1.0000000 }, { 0, 0 } }, // 17
		{{halfSide, halfSide, -halfSide}, { 0.00000000, 0.00000000, -1.0000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 0, 1 } }, // 18
		{{halfSide, halfSide, -halfSide}, { 0.00000000, 1.0000000, 0.00000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 1, 1 } }, // 19
		{{halfSide, halfSide, -halfSide}, { 1.0000000, 0.00000000, 0.00000000 }, { 0.00000000, 0.00000000, -1.0000000, 1.0000000 }, { 1, 1 } }, // 20
		{{halfSide, halfSide, halfSide}, { 0.00000000, 0.00000000, 1.0000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 1, 1 } }, // 21
		{{halfSide, halfSide, halfSide}, { 0.00000000, 1.0000000, 0.00000000 }, { 1.0000000, 0.00000000, 0.00000000, 1.0000000 }, { 0, 1 } }, // 22
		{{halfSide, halfSide, halfSide}, { 1.0000000, 0.00000000, 0.00000000 }, { 0.00000000, 0.00000000, -1.0000000, 1.0000000 }, { 1, 0 } }, // 23
	};
	memcpy(outV, verts, sizeof(verts));

	const uint16_t indices[] = {
		5, 16, 10,		10, 16, 21,
		11, 22, 8,		8, 22, 19,
		7, 18, 2,		2, 18, 13,
		1, 12, 4,		4, 12, 15,
		17, 14, 23,		23, 14, 20,
		0, 3, 6,		6, 3, 9,
	};
	memcpy(outI, indices, sizeof(indices));
}

void Utilities::DrawQuad(void *context,float x,float y,float w,float h)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	float* f =  (float*)(gfxc->allocateFromCommandBuffer(4*sizeof(float), sce::Gnm::kEmbeddedDataAlignment4 ));
	
	f[0]=x;
	f[1]=y;
	f[2]=w;
	f[3]=h;
	sce::Gnm::Buffer b;
	b.initAsConstantBuffer(f,4*sizeof(float) );
	gfxc->setConstantBuffers(sce::Gnm::kShaderStageVs,12,1,&b);
	gfxc->setPrimitiveType(sce::Gnm::kPrimitiveTypeRectList);
	gfxc->drawIndexAuto(3);
 	SIMUL_PS4_VALIDATE
}

void Utilities::DisableDepth(void *context)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::DepthStencilControl dsc;
	dsc.init();
	dsc.setDepthControl(sce::Gnm::kDepthControlZWriteDisable, sce::Gnm::kCompareFuncAlways);
	dsc.setDepthEnable(false);
	gfxc->setDepthStencilControl(dsc);
}

void Utilities::EnableDepthWriteOnly(void *context)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::DepthStencilControl dsc;
	dsc.init();
	dsc.setDepthControl(sce::Gnm::kDepthControlZWriteEnable, sce::Gnm::kCompareFuncAlways);
	dsc.setDepthEnable(true);
	gfxc->setDepthStencilControl(dsc);
}

void Utilities::EnableDepthTestOnly(void *context,sce::Gnm::CompareFunc compare)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::DepthStencilControl dsc;
	dsc.init();
	dsc.setDepthControl(sce::Gnm::kDepthControlZWriteDisable, compare);
	dsc.setDepthEnable(true);
	gfxc->setDepthStencilControl(dsc);
}

void Utilities::DisableBlend(void *context)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::BlendControl blendControl;
	blendControl.init();
	blendControl.setBlendEnable(false);
	gfxc->setBlendControl(0, blendControl);
}

void Utilities::EnableDepth(void *context,sce::Gnm::CompareFunc compare)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::DepthStencilControl dsc;
	dsc.init();
	dsc.setDepthControl(sce::Gnm::kDepthControlZWriteEnable, compare);
	dsc.setDepthEnable(true);
	gfxc->setDepthStencilControl(dsc);
}

void Utilities::EnableOverrideDepth(void *context)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::DepthStencilControl dsc;
	dsc.init();
	dsc.setDepthControl(sce::Gnm::kDepthControlZWriteEnable, sce::Gnm::kCompareFuncAlways);
	dsc.setDepthEnable(true);
	gfxc->setDepthStencilControl(dsc);
}


void Utilities::SetClampSamplerState(void *context,int slot,sce::Gnm::ShaderStage s)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::Sampler clampSamplerState;
	clampSamplerState.init();
	clampSamplerState.setMipFilterMode(sce::Gnm::kMipFilterModeLinear);
	clampSamplerState.setXyFilterMode(sce::Gnm::kFilterModeBilinear,sce::Gnm::kFilterModeBilinear);
	clampSamplerState.setZFilterMode(sce::Gnm::kZFilterModeLinear);
	clampSamplerState.setWrapMode(sce::Gnm::kWrapModeClampLastTexel,sce::Gnm::kWrapModeClampLastTexel,sce::Gnm::kWrapModeClampLastTexel);
	gfxc->setSamplers(s,slot,1,&clampSamplerState);
}

void Utilities::SetWrapSamplerState(void *context,int slot,sce::Gnm::ShaderStage s)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::Sampler wrapSamplerState;
	wrapSamplerState.init();
	wrapSamplerState.setMipFilterMode(sce::Gnm::kMipFilterModeLinear);
	wrapSamplerState.setXyFilterMode(sce::Gnm::kFilterModeBilinear, sce::Gnm::kFilterModeBilinear);
	wrapSamplerState.setZFilterMode(sce::Gnm::kZFilterModeLinear);
	wrapSamplerState.setWrapMode(sce::Gnm::kWrapModeWrap,sce::Gnm::kWrapModeWrap,sce::Gnm::kWrapModeWrap);
	gfxc->setSamplers(s,slot,1,&wrapSamplerState);
}

void Utilities::SetWrapWrapClampSamplerState(void *context,int slot,sce::Gnm::ShaderStage s)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::Sampler wwcSamplerState;
	wwcSamplerState.init();
	wwcSamplerState.setMipFilterMode(sce::Gnm::kMipFilterModeLinear);
	wwcSamplerState.setXyFilterMode(sce::Gnm::kFilterModeBilinear, sce::Gnm::kFilterModeBilinear);
	wwcSamplerState.setZFilterMode(sce::Gnm::kZFilterModeLinear);
	wwcSamplerState.setWrapMode(sce::Gnm::kWrapModeWrap,sce::Gnm::kWrapModeWrap,sce::Gnm::kWrapModeClampLastTexel);
	gfxc->setSamplers(s,slot,1,&wwcSamplerState);
}

void Utilities::SetClampMirrorClampSamplerState(void *context,int slot,sce::Gnm::ShaderStage s)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::Sampler samplerState;
	samplerState.init();
	samplerState.setMipFilterMode(sce::Gnm::kMipFilterModeLinear);
	samplerState.setXyFilterMode(sce::Gnm::kFilterModeBilinear, sce::Gnm::kFilterModeBilinear);
	samplerState.setZFilterMode(sce::Gnm::kZFilterModeLinear);
	samplerState.setWrapMode(sce::Gnm::kWrapModeClampLastTexel,sce::Gnm::kWrapModeMirror,sce::Gnm::kWrapModeClampLastTexel);
	gfxc->setSamplers(s,slot,1,&samplerState);
}

void Utilities::SetClampWrapSamplerState(void *context,int slot,sce::Gnm::ShaderStage s)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::Sampler samplerState;
	samplerState.init();
	samplerState.setMipFilterMode(sce::Gnm::kMipFilterModeLinear);
	samplerState.setXyFilterMode(sce::Gnm::kFilterModeBilinear, sce::Gnm::kFilterModeBilinear);
	samplerState.setZFilterMode(sce::Gnm::kZFilterModeLinear);
	samplerState.setWrapMode(sce::Gnm::kWrapModeClampLastTexel,sce::Gnm::kWrapModeWrap,sce::Gnm::kWrapModeClampLastTexel);
	gfxc->setSamplers(s,slot,1,&samplerState);
}

void Utilities::SetWrapClampSamplerState(void *context,int slot,sce::Gnm::ShaderStage s)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::Sampler samplerState;
	samplerState.init();
	samplerState.setMipFilterMode(sce::Gnm::kMipFilterModeLinear);
	samplerState.setXyFilterMode(sce::Gnm::kFilterModeBilinear, sce::Gnm::kFilterModeBilinear);
	samplerState.setZFilterMode(sce::Gnm::kZFilterModeLinear);
	samplerState.setWrapMode(sce::Gnm::kWrapModeWrap,sce::Gnm::kWrapModeClampLastTexel,sce::Gnm::kWrapModeClampLastTexel);
	gfxc->setSamplers(s,slot,1,&samplerState);
}
void Utilities::SetWrapMirrorSamplerState(void *context,int slot,sce::Gnm::ShaderStage s)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::Sampler samplerState;
	samplerState.init();
	samplerState.setMipFilterMode(sce::Gnm::kMipFilterModeLinear);
	samplerState.setXyFilterMode(sce::Gnm::kFilterModeBilinear, sce::Gnm::kFilterModeBilinear);
	samplerState.setZFilterMode(sce::Gnm::kZFilterModeLinear);
	samplerState.setWrapMode(sce::Gnm::kWrapModeWrap,sce::Gnm::kWrapModeMirror,sce::Gnm::kWrapModeClampLastTexel);
	gfxc->setSamplers(s,slot,1,&samplerState);
}
void Utilities::RenderAngledQuad(void *context,const float *dir,const float *v,const float *p,float half_angle_radians)
{
	struct AngledQuad
	{
		uniform mat4 worldViewProj;
		uniform mat4 invWorld;
		uniform float tanHalfAngleRadians;
		uniform float a,b,c;
	};
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	// If y is vertical, we have LEFT-HANDED rotations, otherwise right.
	// But D3DXMatrixRotationYawPitchRoll uses only left-handed, hence the change of sign below.
	float Yaw=atan2(dir[0],dir[1]);
	float Pitch=-asin(dir[2]);
	simul::math::Matrix4x4 view(v),proj(p),tmp1, wvp;
	simul::math::Matrix4x4 world;
	world=math::Matrix4x4::IdentityMatrix();
	{
		simul::geometry::SimulOrientation ori;
		ori.Rotate(3.14159f-Yaw,simul::math::Vector3(0,0,1.f));
		ori.LocalRotate(3.14159f/2.f+Pitch,simul::math::Vector3(1.f,0,0));
		world=*((const simul::math::Matrix4x4*)(ori.T4.RowPointer(0)));
	}
	//set up matrices
	view(3,0)=0.f;
	view(3,1)=0.f;
	view(3,2)=0.f;

	simul::math::Multiply4x4(tmp1,world,view);
	simul::math::Multiply4x4(wvp,tmp1,proj);
	 
	AngledQuad buf;
	buf.worldViewProj		=wvp;

	simul::math::Matrix4x4 inv_world;
	world.Inverse(inv_world);
	buf.invWorld			=inv_world;

	buf.tanHalfAngleRadians=tan(half_angle_radians);
	APPLY_CONSTANT_BUFFER(gfxc,sce::Gnm::kShaderStageVs,10,AngledQuad,buf);
	gfxc->setPrimitiveType(sce::Gnm::kPrimitiveTypeTriStrip);
	gfxc->drawIndexAuto(4);
 	SIMUL_PS4_VALIDATE
}


void synchronizeRenderTargetGraphicsToCompute(sce::Gnmx::LightweightGfxContext *gfxc, const sce::Gnm::RenderTarget* renderTarget)
{
	gfxc->waitForGraphicsWrites(renderTarget->getBaseAddress256ByteBlocks(), renderTarget->getSliceSizeInBytes()>>8,
		sce::Gnm::kWaitTargetSlotCb0 | sce::Gnm::kWaitTargetSlotCb1 | sce::Gnm::kWaitTargetSlotCb2 | sce::Gnm::kWaitTargetSlotCb3 |
		sce::Gnm::kWaitTargetSlotCb4 | sce::Gnm::kWaitTargetSlotCb5 | sce::Gnm::kWaitTargetSlotCb6 | sce::Gnm::kWaitTargetSlotCb7,
		sce::Gnm::kCacheActionNone, sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache
		,sce::Gnm::kStallCommandBufferParserEnable);
}

void Utilities::SetFaceCullMode(void *context, sce::Gnm::PrimitiveSetupCullFaceMode mode)
{
	sce::Gnmx::LightweightGfxContext *gfxc=(sce::Gnmx::LightweightGfxContext *)context;
	sce::Gnm::PrimitiveSetup primSetup;
	primSetup.init();
	primSetup.setVertexWindowOffsetEnable(false);
	primSetup.setProvokingVertex(sce::Gnm::kPrimitiveSetupProvokingVertexFirst);
	primSetup.setCullFace(mode);
	primSetup.setFrontFace(sce::Gnm::kPrimitiveSetupFrontFaceCw);

	gfxc->setPrimitiveSetup(primSetup);
}
