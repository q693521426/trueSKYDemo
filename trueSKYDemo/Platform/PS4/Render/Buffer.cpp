#include "Buffer.h"
#include "Simul/Platform/PS4/Render/RenderPlatform.h"
#include "Simul/Platform/PS4/Render/Utilities.h"
using namespace simul;
using namespace orbis;

Buffer::Buffer()
	:gnmBuffers(NULL)
	,videoMemory(NULL)
	,renderPlatformPS4(NULL)
{
}


Buffer::~Buffer()
{
	InvalidateDeviceObjects();
	delete [] gnmBuffers;
}

sce::Gnm::Buffer *Buffer::AsGnmBuffer()
{
	return gnmBuffers;
}
void* Buffer::GetVideoMemory()
{
	return videoMemory;
}
void Buffer::InvalidateDeviceObjects()
{
	count=0;
	delete [] gnmBuffers;
	gnmBuffers=NULL;
	if(renderPlatformPS4)
	{
		simul::base::MemoryInterface *mem=renderPlatformPS4->GetMemoryInterface();
		mem->DeallocateVideoMemory(videoMemory);
	}
	videoMemory=NULL;
	renderPlatformPS4=NULL;
}

void Buffer::EnsureVertexBuffer(crossplatform::RenderPlatform *renderPlatform,int num_vertices,const crossplatform::Layout *layout,const void *data,bool cpu_access,bool streamout_target)
{
	InvalidateDeviceObjects();
	gnmBuffers=new sce::Gnm::Buffer[layout->GetDesc().size()];
	// copy data to gpu memory:
	
	size_t vertexBufferSize = num_vertices * layout->GetStructSize();
	renderPlatformPS4=static_cast<orbis::RenderPlatform*>(renderPlatform);
	uint8_t *vb = static_cast<uint8_t*>(renderPlatformPS4->GetMemoryInterface()->AllocateVideoMemoryTracked(vertexBufferSize, sce::Gnm::kAlignmentOfBufferInBytes,"Vertex Buffer"));
	if(data)
		memcpy(vb,data, vertexBufferSize);
	else
		memset(vb,0,vertexBufferSize);
	videoMemory=vb;

	stride = layout->GetStructSize();
	for(int i=0;i<layout->GetDesc().size();i++)
	{
		const crossplatform::LayoutDesc &d=layout->GetDesc()[i];
		gnmBuffers[i].initAsVertexBuffer(
			vb + d.alignedByteOffset	//offsetof(SimpleMeshVertex,m_position)
			, RenderPlatform::ToGnmFormat(d.format)//sce::Gnm::kDataFormatR32G32B32Float
			, stride
			, num_vertices);
	}
	count=layout->GetDesc().size();
}

void Buffer::EnsureIndexBuffer(crossplatform::RenderPlatform *renderPlatform,int num_indices,int index_size_bytes,const void *data)
{
	InvalidateDeviceObjects();
	gnmBuffers=new sce::Gnm::Buffer[1];
	// copy data to gpu memory:
	
	size_t indexBufferSize	=num_indices*index_size_bytes;
	renderPlatformPS4		=static_cast<orbis::RenderPlatform*>(renderPlatform);
	uint8_t *vb				=static_cast<uint8_t*>(renderPlatformPS4->GetMemoryInterface()->AllocateVideoMemory(indexBufferSize, sce::Gnm::kAlignmentOfBufferInBytes));
	if(data)
		memcpy(vb	,data	,indexBufferSize);
	else
		memset(vb	,0		,indexBufferSize);
	videoMemory=vb;
	count=num_indices;
	stride=index_size_bytes;
}

/*
void Buffer::ActualApply(crossplatform::DeviceContext &deviceContext,int slot)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	gfxc->setVertexBuffers(sce::Gnm::kShaderStageVs,slot,count,gnmBuffers);
}*/


void *Buffer::Map(crossplatform::DeviceContext &deviceContext)
{
	return videoMemory;
}

void Buffer::Unmap(crossplatform::DeviceContext &deviceContext)
{
}