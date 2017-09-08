#define NOMINMAX
#include "Effect.h"
#include "Utilities.h"
#include "Texture.h"
#include "Simul/Base/RuntimeError.h"
#include "Simul/Base/StringFunctions.h"
#include "Simul/Platform/CrossPlatform/DeviceContext.h"
#include "Simul/Platform/CrossPlatform/RenderPlatform.h"
#include "Simul/Platform/PS4/Render/Utilities.h"
#include "Simul/Platform/PS4/Render/RenderPlatform.h"
#include "Simul/Platform/PS4/Render/ShaderLoader.h"
#include <string>
#include <gnmx/fetchshaderhelper.h>
#include <sdk_version.h>

using namespace simul;
using namespace orbis;
using namespace std;

static sce::Gnm::ShaderStage ShaderTypeToGnmShaderStage(crossplatform::ShaderType s)
{
	switch(s)
	{
	case crossplatform::SHADERTYPE_VERTEX:
		return sce::Gnm::kShaderStageVs;		// or what about kShaderStageEs??
	case crossplatform::SHADERTYPE_HULL:		// tesselation control.
		return sce::Gnm::kShaderStageHs;
	case crossplatform::SHADERTYPE_DOMAIN:		
		return sce::Gnm::kShaderStageLs;		// LDS=domain shader???
	case crossplatform::SHADERTYPE_GEOMETRY:
		return sce::Gnm::kShaderStageGs;
	case crossplatform::SHADERTYPE_PIXEL:
		return sce::Gnm::kShaderStagePs;
	case crossplatform::SHADERTYPE_COMPUTE:
		return sce::Gnm::kShaderStageCs;
	default:
		return sce::Gnm::kShaderStageCount;
	}

}
/*
D3D11_QUERY toGnmQueryType(crossplatform::QueryType t)
{
	switch(t)
	{
		case crossplatform::QUERY_OCCLUSION:
			return D3D11_QUERY_OCCLUSION;
		case crossplatform::QUERY_TIMESTAMP:
			return D3D11_QUERY_TIMESTAMP;
		case crossplatform::QUERY_TIMESTAMP_DISJOINT:
			return D3D11_QUERY_TIMESTAMP_DISJOINT;
		default:
			return D3D11_QUERY_EVENT;
	};
}
*/

crossplatform::EffectPass *EffectTechnique::AddPass(const char *name,int i)
{
	EffectPass *p=new orbis::EffectPass;
	passes_by_name[name]=passes_by_index[i]=p;
	return p;
}

Query::Query(crossplatform::QueryType t)
	:crossplatform::Query(t)	
	,renderPlatform(NULL)
{
	for(int i=0;i<QueryLatency;i++)
	{
		videomem[i]=0;
	}
}

void Query::RestoreDeviceObjects(crossplatform::RenderPlatform *r)
{
	renderPlatform=r;
	base::MemoryInterface *m=r->GetMemoryInterface();
	for(int i=0;i<QueryLatency;i++)
	{
		m->DeallocateVideoMemory(videomem[i]);
videomem[i]			=(unsigned char*)m->AllocateVideoMemory(4*sizeof(unsigned long),8);
	}
}

void Query::InvalidateDeviceObjects() 
{
	base::MemoryInterface *m=renderPlatform->GetMemoryInterface();
	for(int i=0;i<QueryLatency;i++)
	{
		m->DeallocateVideoMemory(videomem[i]);
videomem[i]=0;
	}
}

void Query::Begin(crossplatform::DeviceContext &deviceContext)
{
	if(type==crossplatform::QUERY_TIMESTAMP_DISJOINT)
	{
		sce::Gnmx::LightweightGfxContext*gfxc=deviceContext.asGfxContext();
		unsigned long &timestamp=*((unsigned long *)&(videomem[currFrame]));
		timestamp = 0x0; // set the memory to have the val 0
		gfxc->writeImmediateAtEndOfPipe(sce::Gnm::kEopFlushCbDbCaches,(void *)&timestamp,0x1,sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2);
    
		gfxc->writeTimestampAtEndOfPipe(	sce::Gnm::kEopCbDbReadsDone,
											&timestamp,
											sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2);
	}
}

void Query::End(crossplatform::DeviceContext &deviceContext)
{    
	if(type==crossplatform::QUERY_TIMESTAMP)
	{
		sce::Gnmx::LightweightGfxContext*gfxc=deviceContext.asGfxContext();
		gfxc->writeTimestampAtEndOfPipe(	sce::Gnm::kEopCbDbReadsDone,
										videomem[currFrame],
										sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2);
	}
}

bool Query::GetData(crossplatform::DeviceContext &deviceContext,void *data,size_t sz)
{
	if(type==crossplatform::QUERY_TIMESTAMP_DISJOINT)
	{
		if(sz==sizeof(crossplatform::DisjointQueryStruct))
		{
			crossplatform::DisjointQueryStruct ds;
			ds.Disjoint=0;
			uint64_t *t=(uint64_t*)(videomem[currFrame]);
			ds.Frequency=SCE_GNM_GPU_CORE_CLOCK_FREQUENCY;
			memcpy(data,&ds,sz);
			currFrame = (currFrame + 1) % QueryLatency;
		}
	}
	else
	{
		uint64_t *t=(uint64_t*)(videomem[currFrame]);
		memcpy(data,t,sz);
		currFrame = (currFrame + 1) % QueryLatency;
	}
	return true;
}

RenderState::RenderState():num(0)
{
}
RenderState::~RenderState()
{
}

void PlatformStructuredBuffer::RestoreDeviceObjects(crossplatform::RenderPlatform *r,int ct,int unit_size,bool computable,void *init_data)
{
	renderPlatform=r;
//	crossplatform::DeviceContext &immediateContext=renderPlatform->GetImmediateContext();
//	SIMUL_PS4_VALIDATE2(immediateContext)
	void *videoMemoryPtr=renderPlatform->GetMemoryInterface()->AllocateVideoMemory(ct*unit_size,sce::Gnm::kAlignmentOfBufferInBytes);
//////	sce::Gnmx::LightweightGfxContext *gfxc=immediateContext.asGfxContext();
	if(init_data)
		memcpy(videoMemoryPtr,init_data,unit_size*ct);
	buffer.initAsRegularBuffer(videoMemoryPtr,unit_size,ct);
	buffer.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);

	num_elements=ct;
	element_bytesize=unit_size;
	for(int i=0;i<latency;i++)
	{
		if(staging_data[i])
			renderPlatform->GetMemoryInterface()->DeallocateVideoMemory(staging_data);
		staging_data[i]=NULL;
		staging_data[i]=(unsigned char *)(renderPlatform->GetMemoryInterface()->AllocateVideoMemory(ct*unit_size+8,8));
		staging_data[i][0]=(__int64_t)1;
	}
	currFrame=0;
	read_data=new unsigned char[ct*unit_size];
}

void *PlatformStructuredBuffer::GetBuffer(crossplatform::DeviceContext &deviceContext)
{
	void *ptr=buffer.getBaseAddress();
	return ptr;
}

const void *PlatformStructuredBuffer::OpenReadBuffer(crossplatform::DeviceContext &deviceContext)
{
	if(numCopies<latency)
		return nullptr;
	lastContext=&deviceContext;
    volatile uint32_t wait = 0;
	while(!(staging_data[currFrame][0]))
	{
		wait++;
		if(wait>=100)
		{
			while(!(staging_data[currFrame][0]))
			{
				wait++;
				if(wait>=1000)
				{
					SIMUL_CERR<<"Timed out after waiting "<<wait<<" times for PlatformStructuredBuffer"<<std::endl;
					return nullptr;
				}
			}
		}
	}
	if(wait>0)
	{
		SIMUL_CERR<<"Waited "<<wait<<" times for PlatformStructuredBuffer"<<std::endl;
	}
	memcpy(read_data,staging_data[currFrame]+8,num_elements*element_bytesize);
	return read_data;
}

void PlatformStructuredBuffer::CloseReadBuffer(crossplatform::DeviceContext &deviceContext)
{
	lastContext=NULL;
}

void PlatformStructuredBuffer::CopyToReadBuffer(crossplatform::DeviceContext &deviceContext)
{
	void *srcGpuAddr=buffer.getBaseAddress();
	deviceContext.asGfxContext()->copyData(
                staging_data[currFrame]+8,
                srcGpuAddr,
                num_elements*element_bytesize,
                sce::Gnm::DmaDataBlockingMode::kDmaDataBlockingEnable
            );
    *(staging_data[currFrame])= 0;
    deviceContext.asGfxContext()->writeImmediateAtEndOfPipe(sce::Gnm::kEopFlushCbDbCaches, (void*)staging_data[currFrame], 1, sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2);
	currFrame=(currFrame+1)%latency;
	// now currFrame will point to the oldest buffer, the one we should be ok to read from.
	numCopies++;
}

void PlatformStructuredBuffer::SetData(crossplatform::DeviceContext &deviceContext,void *data)
{
	void *videoMemoryPtr=buffer.getBaseAddress();
	if(data)
		memcpy(videoMemoryPtr,data,num_elements*element_bytesize);
}

void PlatformStructuredBuffer::LinkToEffect(crossplatform::Effect *effect,const char *name,int bindingIndex)
{
}

void PlatformStructuredBuffer::Apply(crossplatform::DeviceContext &deviceContext,crossplatform::Effect *effect,const char *name)
{
	orbis::RenderPlatform *r=(orbis::RenderPlatform *)deviceContext.renderPlatform;
	crossplatform::ContextState *cs=r->GetContextState(deviceContext);
	orbis::Effect *e=(orbis::Effect *)effect;
	int index=e->GetSlot(name);
	cs->applyStructuredBuffers[index]=this;
}

void PlatformStructuredBuffer::ActualApply(crossplatform::DeviceContext &deviceContext,crossplatform::EffectPass *currentEffectPass,int slot)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	if(!currentEffectPass->usesTextureSlotForSB(slot))
		return;
	crossplatform::Shader **sh=((orbis::EffectPass*)currentEffectPass)->shaders;
	if(slot<1000)
	{
		if(sh[crossplatform::SHADERTYPE_PIXEL]&&sh[crossplatform::SHADERTYPE_PIXEL]->usesTextureSlotForSB(slot))
			gfxc->setBuffers(sce::Gnm::kShaderStagePs,slot, 1, &buffer);
		if(sh[crossplatform::SHADERTYPE_GEOMETRY]&&sh[crossplatform::SHADERTYPE_GEOMETRY]->usesTextureSlotForSB(slot))
			gfxc->setBuffers(sce::Gnm::kShaderStageGs,slot, 1, &buffer);
		if(sh[crossplatform::SHADERTYPE_VERTEX]&&sh[crossplatform::SHADERTYPE_VERTEX]->usesTextureSlotForSB(slot))
		{
			if(sh[crossplatform::SHADERTYPE_GEOMETRY])
				gfxc->setBuffers(sce::Gnm::kShaderStageEs,slot, 1, &buffer);
			else
				gfxc->setBuffers(sce::Gnm::kShaderStageVs,slot, 1, &buffer);
		}
		if(sh[crossplatform::SHADERTYPE_COMPUTE]&&sh[crossplatform::SHADERTYPE_COMPUTE]->usesTextureSlotForSB(slot))
			gfxc->setBuffers(sce::Gnm::kShaderStageCs,slot, 1, &buffer);
	}
	else
	{
		if(sh[crossplatform::SHADERTYPE_COMPUTE]&&sh[crossplatform::SHADERTYPE_COMPUTE]->usesRwTextureSlotForSB(slot-1000))
			gfxc->setRwBuffers(sce::Gnm::kShaderStageCs,slot-1000, 1, &buffer);
	}
	// Maybe here we need to synchronize? Unlikely. We may have filled in values by CPU, but the GPU command won't even
	// be put on the Command buffer until the buffer's been filled so what would it need to wait for? Maybe the CPU cache?
	//deviceContext.asGfxContext()->flushShaderCachesAndWait(sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2,0,sce::Gnm::kStallCommandBufferParserEnable);
}

void PlatformStructuredBuffer::ApplyAsUnorderedAccessView(crossplatform::DeviceContext &deviceContext,crossplatform::Effect *effect,const char *name)
{
	orbis::RenderPlatform *r=(orbis::RenderPlatform *)deviceContext.renderPlatform;
	crossplatform::ContextState *cs=r->GetContextState(deviceContext);
	orbis::Effect *e=(orbis::Effect *)effect;
	int index=e->GetSlot(name)+1000;
	cs->applyStructuredBuffers[index]=this;
}

void PlatformStructuredBuffer::Unbind(crossplatform::DeviceContext &deviceContext)
{
}

void PlatformStructuredBuffer::InvalidateDeviceObjects()
{
	if(renderPlatform)
	{
		for(int i=0;i<latency;i++)
		{
			if(staging_data[i])
				renderPlatform->GetMemoryInterface()->DeallocateVideoMemory(staging_data[i]);
			staging_data[i]=NULL;
		}
		renderPlatform->GetMemoryInterface()->DeallocateVideoMemory(buffer.getBaseAddress());
	}
	delete [] read_data;
	read_data=NULL;
	renderPlatform=NULL;
}

PlatformConstantBuffer::PlatformConstantBuffer()
	:videoMemoryPtr(NULL)
	,addr(NULL)
	,size(0)
	,renderPlatform(NULL)
{

}

void orbis::PlatformConstantBuffer::RestoreDeviceObjects(crossplatform::RenderPlatform *r,size_t s,void *a)
{
	InvalidateDeviceObjects();
	renderPlatform=r;
	size=s;
	addr=a;
}

//! Find the constant buffer in the given effect, and link to it.
void orbis::PlatformConstantBuffer::LinkToEffect(crossplatform::Effect *effect,const char *name,int idx)
{
}

void orbis::PlatformConstantBuffer::InvalidateDeviceObjects()
{
	if(renderPlatform)
	{
	}
	videoMemoryPtr=NULL;
	renderPlatform=NULL;
}


void Effect::SetConstantBuffer(crossplatform::DeviceContext &deviceContext, const char *name, crossplatform::ConstantBufferBase *s)
{
	RenderPlatform *r = (RenderPlatform *)deviceContext.renderPlatform;
	s->GetPlatformConstantBuffer()->Apply(deviceContext, s->GetSize(), s->GetAddr());
	crossplatform::Effect::SetConstantBuffer(deviceContext, name, s);
}


void orbis::PlatformConstantBuffer::ActualApply(simul::crossplatform::DeviceContext &deviceContext,crossplatform::EffectPass *currentEffectPass,int index)
{
	if(!currentEffectPass->usesBufferSlot(index))
		return;
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();

	// This is the potentially slow part. We should only do this on the first ActualApply per commandbuffer, e.g. per-frame,
	// or when Apply has been called on the parent ConstantBuffer.
	if(changed)
	{
		void *videoMemoryPtr = gfxc->allocateFromCommandBuffer(size,sce::Gnm::kEmbeddedDataAlignment16);
		memcpy(videoMemoryPtr,addr,size);
		b.initAsConstantBuffer(videoMemoryPtr,size);
		changed=false;
	}

	Shader **sh=(Shader**)currentEffectPass->shaders;
	if(sh[crossplatform::SHADERTYPE_PIXEL]&&sh[crossplatform::SHADERTYPE_PIXEL]->usesBufferSlot(index))
		gfxc->setConstantBuffers(sce::Gnm::kShaderStagePs,index, 1, &b);
	if(sh[crossplatform::SHADERTYPE_GEOMETRY]&&sh[crossplatform::SHADERTYPE_GEOMETRY]->usesBufferSlot(index))
	{
		gfxc->setConstantBuffers(sce::Gnm::kShaderStageGs,index, 1, &b);
	}
	if(sh[crossplatform::SHADERTYPE_VERTEX]&&sh[crossplatform::SHADERTYPE_VERTEX]->usesBufferSlot(index))
	{
		if(sh[crossplatform::SHADERTYPE_GEOMETRY])
			gfxc->setConstantBuffers(sce::Gnm::kShaderStageEs,index, 1, &b);
		else
			gfxc->setConstantBuffers(sce::Gnm::kShaderStageVs,index, 1, &b);
	}
	if(sh[crossplatform::SHADERTYPE_COMPUTE]&&sh[crossplatform::SHADERTYPE_COMPUTE]->usesBufferSlot(index))
		gfxc->setConstantBuffers(sce::Gnm::kShaderStageCs,index, 1, &b);
}

void orbis::PlatformConstantBuffer::Unbind(simul::crossplatform::DeviceContext &deviceContext)
{
}

void Shader::release()
{
	textureSlots=0;
	bufferSlots=0;
	samplerSlots=0;
	rwTextureSlots=0;
}

void Shader::load(crossplatform::RenderPlatform *renderPlatform,const char *filename_utf8,crossplatform::ShaderType t)
{
	simul::base::MemoryInterface *allocator=renderPlatform->GetMemoryInterface();
	sce::Gnm::ResourceHandle ownerHandle=((orbis::RenderPlatform *)renderPlatform)->GetResourceHandle();
	void* pointer;
	uint32_t bytes;
	simul::base::FileLoader *fileLoader=simul::base::FileLoader::GetFileLoader();
	std::string filenameUtf8=renderPlatform->GetShaderBinaryPath();
	filenameUtf8+="/";
	filenameUtf8+=filename_utf8;
	fileLoader->AcquireFileContents(pointer,bytes,filenameUtf8.c_str(),false);
	if(!pointer)
	{
		// Some engines force filenames to lower case because reasons:
		std::transform(filenameUtf8.begin(), filenameUtf8.end(), filenameUtf8.begin(), ::tolower);
		fileLoader->AcquireFileContents(pointer,bytes,filenameUtf8.c_str(),false);
		if(!pointer)
			return;
	}
	memset(&sbProgram,0,sizeof(sbProgram));
	sbProgram.loadFromMemory(pointer,bytes);
	type=t;
	const sce::Gnmx::ShaderCommonData *shaderCommonData=nullptr;
	const sce::Gnm::InputUsageSlot *inputUsageSlots=nullptr;

	if(t==crossplatform::SHADERTYPE_VERTEX)
	{
		if(filenameUtf8.find("_ve.sb")<filenameUtf8.length())
		{
			//std::cout<<"export shader "<<sbProgram.m_header->m_shaderType<<std::endl;
			
			const sce::Gnm::SizeAlign sizeAlign			=simul::orbis::shaderloader::calculateMemoryRequiredForEsShader(pointer);
			const sce::Gnm::SizeAlign fetchSizeAlign	=simul::orbis::shaderloader::calculateMemoryRequiredForEsFetchShader(pointer);
			
			void *dest							=allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
			exportShader						=simul::orbis::shaderloader::parseEsShader(dest, pointer);
			fetchShader							= allocator->AllocateVideoMemory(fetchSizeAlign.m_size,fetchSizeAlign.m_align);
			sce::Gnmx::generateEsFetchShader(fetchShader,&shaderModifier, exportShader, 0);
			exportShader->applyFetchShaderModifier(shaderModifier);
			sce::Gnmx::generateInputResourceOffsetTable(&resourceOffsets,sce::Gnm::kShaderStageEs,exportShader);
			
			shaderCommonData=&exportShader->m_common;
			inputUsageSlots=exportShader->getInputUsageSlotTable();
			sce::Gnm::registerResource(nullptr, ownerHandle, exportShader->getBaseAddress(), sizeAlign.m_size,filename_utf8,sce::Gnm::kResourceTypeShaderBaseAddress, 0);
		}
		else
		{
			//std::cout<<"vertex shader "<<sbProgram.m_header->m_shaderType<<std::endl;
			const sce::Gnm::SizeAlign vsSizeAlign		=simul::orbis::shaderloader::calculateMemoryRequiredForVsShader(pointer);
			const sce::Gnm::SizeAlign fetchSizeAlign	=simul::orbis::shaderloader::calculateMemoryRequiredForVsFetchShader(pointer);
			sce::Gnm::SizeAlign sizeAlign;
			sizeAlign.m_align							=vsSizeAlign.m_align;
			sizeAlign.m_size							=vsSizeAlign.m_size;//+fetchSizeAlign.m_size;
			void *dest									=allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
			vertexShader								=simul::orbis::shaderloader::parseVsShader(dest, pointer);
	
			fetchShader									= allocator->AllocateVideoMemory(fetchSizeAlign.m_size,fetchSizeAlign.m_align);
			sce::Gnmx::generateVsFetchShader(fetchShader,&shaderModifier, vertexShader, 0);
			vertexShader->applyFetchShaderModifier(shaderModifier);
			sce::Gnmx::generateInputResourceOffsetTable(&resourceOffsets,sce::Gnm::kShaderStageVs,vertexShader);
			shaderCommonData=&vertexShader->m_common;
			inputUsageSlots=vertexShader->getInputUsageSlotTable();
			sce::Gnm::registerResource(nullptr, ownerHandle, vertexShader->getBaseAddress(), sizeAlign.m_size,filename_utf8,sce::Gnm::kResourceTypeShaderBaseAddress, 0);

		}
	}
	else if(t==crossplatform::SHADERTYPE_GEOMETRY)
	{
		const sce::Gnm::SizeAlign sizeAlign		=simul::orbis::shaderloader::calculateMemoryRequiredForGsShader(pointer);
		void * dest								=allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
		geometryShader							=simul::orbis::shaderloader::parseGsShader(dest, pointer);
		sce::Gnmx::generateInputResourceOffsetTable(&resourceOffsets,sce::Gnm::kShaderStageGs,geometryShader);
		shaderCommonData=&geometryShader->m_common;
		inputUsageSlots=geometryShader->getInputUsageSlotTable();
		sce::Gnm::registerResource(nullptr, ownerHandle, geometryShader->getBaseAddress(), sizeAlign.m_size,filename_utf8,sce::Gnm::kResourceTypeShaderBaseAddress, 0);



	}
	else if(t==crossplatform::SHADERTYPE_PIXEL)
	{
		const sce::Gnm::SizeAlign sizeAlign		=simul::orbis::shaderloader::calculateMemoryRequiredForPsShader(pointer);
		void * dest								=allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
		pixelShader								=simul::orbis::shaderloader::parsePsShader(dest, pointer);
		sce::Gnmx::generateInputResourceOffsetTable(&resourceOffsets,sce::Gnm::kShaderStagePs,pixelShader);
		shaderCommonData=&pixelShader->m_common;
		inputUsageSlots=pixelShader->getInputUsageSlotTable();
		sce::Gnm::registerResource(nullptr, ownerHandle, pixelShader->getBaseAddress(), sizeAlign.m_size,filename_utf8,sce::Gnm::kResourceTypeShaderBaseAddress, 0);

		//or sce::Gnm::kResourceTypeFetchShaderBaseAddress
	}
	else if(t==crossplatform::SHADERTYPE_COMPUTE)
	{
		const sce::Gnm::SizeAlign sizeAlign		= simul::orbis::shaderloader::calculateMemoryRequiredForCsShader(pointer);
		void * dest								= allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
		computeShader							= simul::orbis::shaderloader::parseCsShader(dest, pointer);
		sce::Gnmx::generateInputResourceOffsetTable(&resourceOffsets,sce::Gnm::kShaderStageCs,computeShader);
		shaderCommonData=&computeShader->m_common;
		inputUsageSlots=computeShader->getInputUsageSlotTable();
	}
	using namespace sce::Shader::Binary;
	//Gnmx holds global tables of T# resources for textures
	// , V# resources for vertex buffers and constant buffers
	// , S# samplers, T# resources for read-write resources
	for(int i=0;i<sbProgram.m_numBuffers;i++)
	{
		sce::Shader::Binary::Buffer*			b	=&(sbProgram.m_buffers[i]);
		PsslBufferType						type	=(PsslBufferType)b->m_langType;
		/*
typedef enum PsslInternalBufferType
{
	kInternalBufferTypeUav,
	kInternalBufferTypeSrv,
	kInternalBufferTypeLds,
	kInternalBufferTypeGds,
	kInternalBufferTypeCbuffer,
	kInternalBufferTypeTextureSampler,
	kInternalBufferTypeInternal,
	kInternalBufferTypeInternalBufferTypeLast 
		*/
		int slot									=b->m_resourceIndex;
		switch(type)
		{
		case PsslBufferType::kBufferTypeConstantBuffer:
			setUsesConstantBufferSlot(slot);
			break;
		case PsslBufferType::kBufferTypeDataBuffer:
		case PsslBufferType::kBufferTypeTexture1d:
		case PsslBufferType::kBufferTypeTexture2d:
		case PsslBufferType::kBufferTypeTexture3d:
		case PsslBufferType::kBufferTypeTexture1dArray:
		case PsslBufferType::kBufferTypeTexture2dArray:
		case PsslBufferType::kBufferTypeTextureCube:
		case PsslBufferType::kBufferTypeTextureCubeArray:
		case PsslBufferType::kBufferTypeMsTexture2d:
		case PsslBufferType::kBufferTypeMsTexture2dArray:
		case PsslBufferType::kBufferTypeByteBuffer:
		case PsslBufferType::kBufferTypeTexture1dR128:
		case PsslBufferType::kBufferTypeTexture2dR128:
		case PsslBufferType::kBufferTypeMsTexture2dR128:
			setUsesTextureSlot(slot);
			break;
		case PsslBufferType::kBufferTypeRegularBuffer:
			setUsesTextureSlotForSB(slot);
			break;
		case PsslBufferType::kBufferTypeRwDataBuffer:
		case PsslBufferType::kBufferTypeRwTexture1d:
		case PsslBufferType::kBufferTypeRwTexture2d:
		case PsslBufferType::kBufferTypeRwTexture3d:
		case PsslBufferType::kBufferTypeRwTexture1dArray:
		case PsslBufferType::kBufferTypeRwTexture2dArray:
		case PsslBufferType::kBufferTypeRwByteBuffer:
		case PsslBufferType::kBufferTypeRwTextureCube:
		case PsslBufferType::kBufferTypeRwTextureCubeArray:
		case PsslBufferType::kBufferTypeRwMsTexture2d:
		case PsslBufferType::kBufferTypeRwMsTexture2dArray:
		case PsslBufferType::kBufferTypeRwTexture1dR128:
		case PsslBufferType::kBufferTypeRwTexture2dR128:
		case PsslBufferType::kBufferTypeRwMsTexture2dR128:
			setUsesRwTextureSlot(slot);
			break;
		case PsslBufferType::kBufferTypeRwRegularBuffer:
			setUsesRwTextureSlotForSB(slot);
			break;
		case PsslBufferType::kBufferTypeLineBuffer:
		case PsslBufferType::kBufferTypePointBuffer:
		case PsslBufferType::kBufferTypeTriangleBuffer:
			// I THINK we don't have to do anything about this.
			break;
		default:
			SIMUL_BREAK(base::QuickFormat("Unknown buffer type %d",type));
			break;
		};
	}
	// Which sampler states are needed?
	for(int i=0;i<sbProgram.m_numSamplerStates;i++)
	{
		sce::Shader::Binary::SamplerState &st=sbProgram.m_samplerStates[i];
		crossplatform::SamplerState *ss=renderPlatform->GetOrCreateSamplerStateByName(st.getName());
		samplerStates[ss]=st.m_resourceIndex;
	}
	fileLoader->ReleaseFileContents(pointer);
}

crossplatform::EffectTechnique *Effect::CreateTechnique()
{
	return new orbis::EffectTechnique();
}

void Effect::ParseBuffer(const sce::Shader::Binary::Program &program)
{
	for(int i=0;i<program.m_numBuffers;i++)
	{
		const sce::Shader::Binary::Buffer	&buffer = program.m_buffers[i];
		const char							*name = buffer.getName();
		std::cout<<name;
	}
}

#if 1
Effect::Effect()
{
}

Effect::~Effect()
{
	InvalidateDeviceObjects();
}

void Effect::InvalidateDeviceObjects()
{
	if(renderPlatform)
	{
		//crossplatform::DeviceContext &immediateContext=renderPlatform->GetImmediateContext();
		//SIMUL_PS4_VALIDATE2(immediateContext)
	}
}

crossplatform::EffectTechnique *Effect::GetTechniqueByName(const char *name)
{
	return groupCharMap[0]->GetTechniqueByName(name);
}

crossplatform::EffectTechnique *Effect::GetTechniqueByIndex(int index)
{
	if(groupCharMap[0]->techniques_by_index.find(index)!=groupCharMap[0]->techniques_by_index.end())
	{
		return groupCharMap[0]->techniques_by_index[index];
	}
	return NULL;
}


void Effect::SetConstantBuffer(crossplatform::DeviceContext &deviceContext,const char *name	,crossplatform::ConstantBufferBase *s)
{
	orbis::RenderPlatform *r=(orbis::RenderPlatform *)deviceContext.renderPlatform;
	crossplatform::ContextState *cs=r->GetContextState(deviceContext);
	orbis::PlatformConstantBuffer *pcb=(orbis::PlatformConstantBuffer*)s->GetPlatformConstantBuffer();

	cs->applyBuffers[s->GetIndex()]=s;
	cs->buffersValid=false;
	pcb->SetChanged();
}
		
crossplatform::ShaderResource Effect::GetShaderResource(const char *name)
{
	crossplatform::ShaderResource res;
	auto i=GetTextureDetails(name);
	if(!i)
	{
		res.valid=false;
		SIMUL_CERR<<"Invalid Shader resource name: "<<(name?name:"")<<std::endl;
		SIMUL_BREAK_ONCE("")
		return res;
	}
	else
		res.valid=true;
	unsigned slot = GetSlot(name);
	unsigned dim = GetDimensions(name);
	res.platform_shader_resource=(void*)nullptr;
	res.slot		=slot;
	res.shaderResourceType=GetResourceType(name);
	return res;
}

void Effect::SetSamplerState(crossplatform::DeviceContext &deviceContext,const char *name,crossplatform::SamplerState *s)
{
	crossplatform::ContextState *cs=renderPlatform->GetContextState(deviceContext);
	auto i= samplerStates.find(name);
	if(i==samplerStates.end())
	{
		SIMUL_CERR<<"Sampler state "<<name<<" not found."<<std::endl;
		return;
	}
	if (!s)
	{
		SIMUL_CERR<<"Sampler state "<<name<<" null."<<std::endl;
		s=renderPlatform->GetOrCreateSamplerStateByName("clampSamplerState");
	}
	crossplatform::SamplerState *ss=i->second;
	cs->samplerStateOverrides[ss->default_slot]=s;
	cs->samplerStateOverridesValid=false;
}


void Effect::Apply(crossplatform::DeviceContext &deviceContext,crossplatform::EffectTechnique *effectTechnique,int pass_num)
{
	if(apply_count!=0)
		SIMUL_BREAK("Effect::Apply without a corresponding Unapply!")
	apply_count++;
	currentTechnique				=effectTechnique;
	if(effectTechnique)
	{
		EffectPass *p				=(orbis::EffectPass*)(effectTechnique)->GetPass(pass_num>=0?pass_num:0);
		if(p)
		{
			crossplatform::ContextState *cs=renderPlatform->GetContextState(deviceContext);
			cs->invalidate();
			cs->currentEffectPass=p;
			cs->currentTechnique=effectTechnique;
			cs->currentEffect=this;
		}
		else
			SIMUL_BREAK("No pass found");
		deviceContext.asGfxContext()->pushMarker(effectTechnique->name.c_str());
	}
	currentPass=pass_num;
	//crossplatform::Effect::Apply(deviceContext,effectTechnique,pass_num);
}

void EffectPass::Apply(crossplatform::DeviceContext &deviceContext,bool test)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	// Apply the shaders
	orbis::Shader *c =(orbis::Shader*)shaders[crossplatform::SHADERTYPE_COMPUTE];
	orbis::Shader *v =(orbis::Shader*)shaders[crossplatform::SHADERTYPE_VERTEX];
	Shader *g=NULL;
	if(shaders[crossplatform::SHADERTYPE_GEOMETRY])
	{
		Shader *exp=(orbis::Shader*)shaders[crossplatform::SHADERTYPE_VERTEX];
		SIMUL_ASSERT(exp->exportShader!=NULL);
		if(exp)
			gfxc->setEsShader(exp->exportShader,exp->shaderModifier,exp->fetchShader,&exp->resourceOffsets);
		g=(orbis::Shader*)shaders[crossplatform::SHADERTYPE_GEOMETRY];
		gfxc->setGsVsShaders(g->geometryShader,&g->resourceOffsets);
		gfxc->setActiveShaderStages(sce::Gnm::ActiveShaderStages::kActiveShaderStagesEsGsVsPs);
	}
	else if (v)
	{
		// TODO: Why is this needed for DrawCubemap?
		//gfxc->m_lwcue.invalidateShaderStage(sce::Gnm::ShaderStage::kShaderStageVs);
		if(v)
			gfxc->setVsShader(v->vertexShader,v->shaderModifier,v->fetchShader,&v->resourceOffsets);
		else
			gfxc->setVsShader(NULL,0,NULL,NULL);
		gfxc->setActiveShaderStages(sce::Gnm::ActiveShaderStages::kActiveShaderStagesVsPs);
	if(test)
		return;
	}
	if(test)
		return;
	// What output format?
	orbis::RenderPlatform *rp=(orbis::RenderPlatform*)deviceContext.renderPlatform;
	if(rp&&(g||v))
	{
		crossplatform::PixelOutputFormat pfm=rp->GetCurrentPixelOutputFormat(deviceContext);
		Shader *p = (Shader*)pixelShaders[pfm];
		shaders[crossplatform::SHADERTYPE_PIXEL]=p;
		if(p)
		{
			SIMUL_ASSERT(p->pixelShader!=NULL);
			if(p->pixelShader)
				gfxc->setPsShader(p->pixelShader,&p->resourceOffsets);
			else
				gfxc->setPsShader(NULL,NULL);
			gfxc->setVsShaderStreamoutEnable(false);
		}
		else
		{
			// Maybe we're doing streamout.
			gfxc->setVsShaderStreamoutEnable(true);
		////	SIMUL_BREAK("Can't find valid pixel shader for output format.");
		//	PixelOutputFormat pfm=rp->GetCurrentPixelOutputFormat(deviceContext);
		}
	}
	if(c)
	{
		SIMUL_ASSERT(c->computeShader!=NULL);
		if(c->computeShader)
			gfxc->setCsShader(c->computeShader,&c->resourceOffsets);
		else
			gfxc->setCsShader(NULL,NULL);
	}
	for(int i=0;i<crossplatform::SHADERTYPE_COUNT;i++)
	{
		Shader *s= (Shader*)shaders[i];
		if(s&&s->samplerStates.size())
		{
			sce::Gnm::ShaderStage ss=ShaderTypeToGnmShaderStage((crossplatform::ShaderType)i);
			if(i==crossplatform::SHADERTYPE_VERTEX&&(shaders[crossplatform::SHADERTYPE_GEOMETRY]!=nullptr))
				ss=sce::Gnm::kShaderStageEs;
			for(auto i=s->samplerStates.begin();i!=s->samplerStates.end();i++)
			{
				gfxc->setSamplers(ss,i->second,1,i->first->AsGnmSampler());
			}
		}
	}
	if(v!=nullptr||g!=nullptr)
	{
		sce::Gnm::ShaderStage ss=sce::Gnm::kShaderStagePs;
		crossplatform::PixelOutputFormat pfm=rp->GetCurrentPixelOutputFormat(deviceContext);
		Shader *s = (Shader*) pixelShaders[pfm];
		if(s)
		for(unordered_map<crossplatform::SamplerState *,int>::iterator j=s->samplerStates.begin();j!=s->samplerStates.end();j++)
		{
			gfxc->setSamplers(ss,j->second,1,j->first->AsGnmSampler());
		}
	}
	if(blendState)
	{
		deviceContext.renderPlatform->SetRenderState(deviceContext,blendState);
	}
	if(depthStencilState)
	{
		deviceContext.renderPlatform->SetRenderState(deviceContext,depthStencilState);
	}
	if(rasterizerState)
	{
		deviceContext.renderPlatform->SetRenderState(deviceContext,rasterizerState);
	}
}

void Effect::Apply(crossplatform::DeviceContext &deviceContext,crossplatform::EffectTechnique *effectTechnique,const char *passname)
{
	if(apply_count!=0)
		SIMUL_BREAK("Effect::Apply without a corresponding Unapply!")
	apply_count++;
	currentTechnique				=effectTechnique;
	if (effectTechnique)
	{
		EffectPass *p = NULL;
		if(passname)
			p=(orbis::EffectPass*)(effectTechnique->GetPass(passname));
		else
			p=(orbis::EffectPass*)(effectTechnique->GetPass(0));
		if(p)
		{
			orbis::RenderPlatform *rp=(orbis::RenderPlatform*)renderPlatform;
			crossplatform::ContextState *cs=rp->GetContextState(deviceContext);
			cs->invalidate();
			cs->currentTechnique=effectTechnique;
			cs->currentEffectPass=p;
			cs->currentEffect=this;
		}
		else
			SIMUL_BREAK("No pass found");
		deviceContext.asGfxContext()->pushMarker(effectTechnique->name.c_str());
	}
	//crossplatform::Effect::Apply(deviceContext,effectTechnique,passname);
}

void Effect::Reapply(crossplatform::DeviceContext &deviceContext)
{
	orbis::RenderPlatform *rp=(orbis::RenderPlatform*)renderPlatform;
	crossplatform::ContextState *cs=rp->GetContextState(deviceContext);
	cs->textureAssignmentMapValid=false;
}

void Effect::Unapply(crossplatform::DeviceContext &deviceContext)
{
	orbis::RenderPlatform *rp=(orbis::RenderPlatform*)renderPlatform;
	crossplatform::ContextState *cs=rp->GetContextState(deviceContext);
	orbis::EffectPass *pass=static_cast<orbis::EffectPass*>(cs->currentEffectPass);
	if (pass->shaders[crossplatform::SHADERTYPE_GEOMETRY])
	{
		sce::Gnmx::LightweightGfxContext*gfxc=deviceContext.asGfxContext();
		gfxc->setActiveShaderStages(sce::Gnm::kActiveShaderStagesVsPs);
		gfxc->setGsModeOff();
#if SCE_ORBIS_SDK_VERSION>=0x04000000u
		gfxc->resetVgtControl();
#endif
	}
	cs->currentEffectPass=NULL;
	cs->currentEffect=NULL;
	if(apply_count<=0)
		SIMUL_BREAK("Effect::Unapply without a corresponding Apply!")
	else if(apply_count>1)
		SIMUL_BREAK("Effect::Apply has been called too many times!")
	apply_count--;
	currentTechnique = NULL;
	//deviceContext.asGfxContext()->flushShaderCachesAndWait(sce::Gnm::kCacheActionWriteBackAndInvalidateL2Volatile,0,sce::Gnm::kStallCommandBufferParserEnable);
	deviceContext.asGfxContext()->popMarker();
	
/*	if((&deviceContext)==&(renderPlatform->GetImmediateContext()))
	{
		sce::Gnmx::LightweightGfxContext*gfxc=deviceContext.asGfxContext();
		gfxc->setMarker("Immediate submit");
		((orbis::RenderPlatform*)renderPlatform)->WaitForGpu(deviceContext);
	}*/
}
#endif