#define NOMINMAX
#include "Simul/Base/RuntimeError.h"
#include "Simul/Platform/PS4/Render/RenderPlatform.h"
#include "Simul/Platform/PS4/Render/Material.h"
#include "Simul/Platform/PS4/Render/Mesh.h"
#include "Simul/Platform/PS4/Render/Texture.h"
#include "Simul/Platform/PS4/Render/Light.h"
#include "Simul/Platform/PS4/Render/Effect.h"
#include "Simul/Platform/PS4/Render/Buffer.h"
#include "Simul/Platform/PS4/Render/FrameBuffer.h"
#include "Simul/Platform/PS4/Render/Layout.h"
#include "Simul/Platform/PS4/Render/GpuProfiler.h"
#include "Simul/Platform/PS4/Render/Utilities.h"
#include "Simul/Platform/CrossPlatform/DeviceContext.h"
#include "Simul/Platform/CrossPlatform/Macros.h"
#include "Simul/Platform/CrossPlatform/Camera.h"

#include "Simul/Math/Matrix4x4.h"
#include "Mesh.h"

using namespace simul;
using namespace orbis;
using namespace std;

crossplatform::PixelFormat RenderPlatform::FromGnmFormat(sce::Gnm::DataFormat d)
{
	using namespace sce::Gnm;
	using namespace simul::crossplatform;
	if(d.m_asInt==sce::Gnm::kDataFormatInvalid.m_asInt)
		return UNKNOWN;
	if(d.m_asInt==kDataFormatR16Float.m_asInt)
		return R_16_FLOAT;
	if(d.m_asInt==kDataFormatR16G16B16A16Float.m_asInt)
		return RGBA_16_FLOAT;
	if(d.m_asInt==kDataFormatR32G32B32A32Float.m_asInt)
		return RGBA_32_FLOAT;
	if(d.m_asInt==kDataFormatR32G32B32Float.m_asInt)
		return RGB_32_FLOAT;
	if(d.m_asInt==kDataFormatR32G32Float.m_asInt)
		return RG_32_FLOAT;
	if(d.m_asInt==kDataFormatR32Float.m_asInt)
		return R_32_FLOAT;
	if(d.m_asInt==kDataFormatR32Float.m_asInt)
		return LUM_32_FLOAT;
	if(d.m_asInt==kDataFormatR32Float.m_asInt)
		return INT_32_FLOAT;
	if(d.m_asInt==kDataFormatR8G8B8A8Unorm.m_asInt)
		return RGBA_8_UNORM;
	if(d.m_asInt==kDataFormatR8G8B8A8Snorm.m_asInt)
		return RGBA_8_SNORM;
	if(d.m_asInt==kDataFormatR8Unorm.m_asInt)
		return R_8_UNORM;
	if(d.m_asInt==kDataFormatR8Snorm.m_asInt)
		return R_8_SNORM;
	if(d.m_asInt==kDataFormatR32Uint.m_asInt)
		return R_32_UINT;
	if(d.m_asInt==kDataFormatR32G32Uint.m_asInt)
		return RG_32_UINT;
	if(d.m_asInt==kDataFormatR32G32B32Uint.m_asInt)
		return RGB_32_UINT;
	if(d.m_asInt==kDataFormatR32G32B32A32Uint.m_asInt)
		return RGBA_32_UINT;
	return UNKNOWN;
}

	/*		FMT_UNKNOWN
			,FMT_32_GR
			,FMT_32_AR 
			,FMT_FP16_ABGR 
			,FMT_UNORM16_ABGR 
			,FMT_SNORM16_ABGR 
			,FMT_UINT16_ABGR 
			,FMT_SINT16_ABGR 
			,FMT_32_ABGR 
			,OUTPUT_FORMAT_COUNT*/
crossplatform::PixelOutputFormat PixelOutputFormatFromGnmFormat(sce::Gnm::DataFormat d)
{
	using namespace sce::Gnm;
	using namespace simul::crossplatform;
	if(d.m_asInt==sce::Gnm::kDataFormatInvalid.m_asInt)
		return FMT_UNKNOWN;
	if(d.m_asInt==kDataFormatR16G16B16A16Float.m_asInt)
		return FMT_FP16_ABGR;
	if(d.m_asInt==kDataFormatR32G32B32A32Float.m_asInt)
		return FMT_32_ABGR;
	if(d.m_asInt==kDataFormatR32G32Float.m_asInt)
		return FMT_32_ABGR;
	if(d.m_asInt==kDataFormatR32Float.m_asInt)
		return FMT_32_ABGR;
	if(d.m_asInt==kDataFormatR8G8B8A8Unorm.m_asInt)
		return FMT_FP16_ABGR;
	if(d.m_asInt==kDataFormatR8G8B8A8Snorm.m_asInt)
		return FMT_FP16_ABGR;
	if(d.m_asInt==kDataFormatR8Unorm.m_asInt)
		return FMT_FP16_ABGR;
	if(d.m_asInt==kDataFormatR8Snorm.m_asInt)
		return FMT_FP16_ABGR;
	if(d.m_asInt==kDataFormatR11G11B10Float.m_asInt)
		return FMT_FP16_ABGR;
	SIMUL_CERR<<"Unknown pixel output format: "<<d.m_asInt<<std::endl;
	return FMT_UNKNOWN;
}
crossplatform::PixelFormat RenderPlatform::FromGnmDepthFormat(sce::Gnm::ZFormat d)
{
	using namespace sce::Gnm;
	using namespace simul::crossplatform;
	if(d==sce::Gnm::kZFormat32Float)
		return D_32_FLOAT;
	if(d==sce::Gnm::kZFormat16)
		return D_16_UNORM;
	return UNKNOWN;
}

sce::Gnm::DataFormat RenderPlatform::DepthToEquivalentGnmFormat(crossplatform::PixelFormat f)
{
	switch (f)
	{
	case simul::crossplatform::D_32_FLOAT:
		return sce::Gnm::kDataFormatR32Float;
	case simul::crossplatform::D_16_UNORM:
		return sce::Gnm::kDataFormatR16Uint;
	default:
		return sce::Gnm::kDataFormatInvalid;
		break;
	}
}

sce::Gnm::PrimitiveType RenderPlatform::ToGnmTopology(crossplatform::Topology t)
{
	using namespace crossplatform;
	using namespace sce::Gnm;
	switch(t)
	{		
	case POINTLIST	:		
		return kPrimitiveTypePointList;
	case LINELIST	:		
		return kPrimitiveTypeLineList;
	case LINESTRIP		:	
		return kPrimitiveTypeLineStrip;
	case TRIANGLELIST	:	
		return kPrimitiveTypeTriList;
	case TRIANGLESTRIP	:	
		return kPrimitiveTypeTriStrip;
	case LINELIST_ADJ		:
		return kPrimitiveTypeLineListAdjacency;
	case LINESTRIP_ADJ		:
		return  kPrimitiveTypeLineStripAdjacency;
	case TRIANGLELIST_ADJ	:
		return kPrimitiveTypeTriListAdjacency;
	case TRIANGLESTRIP_ADJ:
		return kPrimitiveTypeTriStripAdjacency;
	default:
		return kPrimitiveTypeNone;
	};
}

sce::Gnm::DataFormat RenderPlatform::ToGnmFormat(crossplatform::PixelFormat f)
{
	switch (f)
	{
	case simul::crossplatform::UNKNOWN:
		return sce::Gnm::kDataFormatInvalid;
		break;
	case simul::crossplatform::R_16_FLOAT:
		return sce::Gnm::kDataFormatR16Float;
		break;
	case simul::crossplatform::RGBA_16_FLOAT:
		return sce::Gnm::kDataFormatR16G16B16A16Float;
		break;
	case simul::crossplatform::RGBA_32_FLOAT:
		return sce::Gnm::kDataFormatR32G32B32A32Float;
		break;
	case simul::crossplatform::RGB_32_FLOAT:
		return sce::Gnm::kDataFormatR32G32B32Float;
		break;
	case simul::crossplatform::RGB_11_11_10_FLOAT:
		return sce::Gnm::kDataFormatR11G11B10Float;
		break;
	case simul::crossplatform::RG_32_FLOAT:
		return sce::Gnm::kDataFormatR32G32Float;
		break;
	case simul::crossplatform::RG_16_FLOAT:
		return sce::Gnm::kDataFormatR16G16Float;
		break;
	case simul::crossplatform::R_32_FLOAT:
		return sce::Gnm::kDataFormatR32Float;
		break;
	case simul::crossplatform::LUM_32_FLOAT:
		return sce::Gnm::kDataFormatR32Float;
		break;
	case simul::crossplatform::INT_32_FLOAT:
		return sce::Gnm::kDataFormatR32Float;
		break;
	case simul::crossplatform::RGBA_8_UNORM:
		return sce::Gnm::kDataFormatR8G8B8A8Unorm;
		break;
	case simul::crossplatform::RGBA_8_SNORM:
		return sce::Gnm::kDataFormatR8G8B8A8Snorm;
		break;
	case simul::crossplatform::R_8_UNORM:
		return sce::Gnm::kDataFormatR8Unorm;
		break;
	case simul::crossplatform::R_8_SNORM:
		return sce::Gnm::kDataFormatR8Snorm;
		break;
	case simul::crossplatform::R_32_UINT:
		return sce::Gnm::kDataFormatR32Uint;
		break;
	case simul::crossplatform::RG_32_UINT:
		return sce::Gnm::kDataFormatR32G32Uint;
		break;
	case simul::crossplatform::RGB_32_UINT:
		return sce::Gnm::kDataFormatR32G32B32Uint;
		break;
	case simul::crossplatform::RGBA_32_UINT:
		return sce::Gnm::kDataFormatR32G32B32A32Uint;
		break;
	case simul::crossplatform::D_32_FLOAT:
		return sce::Gnm::kDataFormatR32Float;
		break;
	case simul::crossplatform::D_16_UNORM:
		return sce::Gnm::kDataFormatR16Uint;
		break;
	default:
		return sce::Gnm::kDataFormatInvalid;
		break;
	}
}

sce::Gnm::ZFormat RenderPlatform::ToGnmDepthFormat(crossplatform::PixelFormat f)
{
	switch (f)
	{
	case simul::crossplatform::D_32_FLOAT:
		return sce::Gnm::kZFormat32Float;
	case simul::crossplatform::D_16_UNORM:
		return sce::Gnm::kZFormat16;
	default:
		break;
	}
	return sce::Gnm::kZFormatInvalid;
}

RenderPlatform::RenderPlatform(simul::base::MemoryInterface *m,sce::Gnmx::LightweightGfxContext *immediatecontext)
	:crossplatform::RenderPlatform(m)
	,device(NULL)
	,gfxc_i(nullptr)
{
	/*if(m)
		gfxc_i=(sce::Gnmx::LightweightGfxContext*)m->AllocateVideoMemory(sizeof(sce::Gnmx::LightweightGfxContext),sce::Gnm::kAlignmentOfBufferInBytes);
	else
		gfxc_i=new sce::Gnmx::LightweightGfxContext;*/
/*	enum {kCommandBufferSizeInBytes = 1 * 1024 * 1024};
	const uint32_t kNumBatches = 100; // Estimated number of batches to support within the resource buffer
		const uint32_t resourceBufferSize = sce::Gnmx::LightweightConstantUpdateEngine::computeResourceBufferSize(sce::Gnmx::LightweightConstantUpdateEngine::kResourceBufferGraphics, kNumBatches);
		gfxc_i->init
			(		
			m->AllocateVideoMemory(kCommandBufferSizeInBytes,sce::Gnm::kAlignmentOfBufferInBytes), kCommandBufferSizeInBytes,		// Draw command buffer
			m->AllocateVideoMemory(resourceBufferSize, sce::Gnm::kAlignmentOfBufferInBytes), resourceBufferSize,					// Resource buffer
			NULL																															// Global resource table 
			);*/
	shaderBinaryPathUtf8=std::string("/app0/Render/shaderbin/");
	texturePathsUtf8.push_back(std::string("/app0/Render/textures"));
	immediateContext.platform_context=gfxc_i;
	//SynchronizeCacheAndState(immediateContext);
	gpuProfiler=new orbis::GpuProfiler;
	resourceHandle=sce::Gnm::registerOwner(nullptr,"Simul PS4 RenderPlatform");
}

RenderPlatform::~RenderPlatform()
{
	InvalidateDeviceObjects();
	//memoryInterface->DeallocateVideoMemory(gfxc_i);
}

void RenderPlatform::RestoreDeviceObjects(void *d)
{
	device=(sce::Gnmx::LightweightGfxContext*)d;
	immediateContext.platform_context=d;
	crossplatform::RenderPlatform::RestoreDeviceObjects(d);
	RecompileShaders();

	sce::Gnm::DataFormat rformat=sce::Gnm::kDataFormatB8G8R8A8Unorm;
	
	{
		sce::Gnm::SizeAlign sizeAlign = dummytexture2d.initAs2d(1, 1, 1, rformat
			, sce::Gnm::kTileModeThin_1dThin/*kTileModeDisplay_LinearAligned*/, sce::Gnm::kNumFragments1);
		dummytexture2d.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
		uint32_t ls				=rformat.getBytesPerElement();
		uint8_t *finalTexels	=(uint8_t*)GetMemoryInterface()->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
		
		memset(finalTexels,0x255,sizeAlign.m_size);
		
		sce::GpuAddress::TilingParameters tp;
		tp.initFromTexture(&dummytexture2d,0,0);
		// Update tiling params
		tp.m_mipLevel		=0;
		tp.m_linearWidth	=1;
		tp.m_linearHeight	=1;
		tp.m_linearDepth	=1;
		tp.m_baseTiledPitch =0;
		uint64_t mipOffset = 0;
		uint64_t mipSize = 0;
		uint8_t *linearTexels=new uint8_t[sizeAlign.m_size];
		memset(linearTexels,0x00,sizeAlign.m_size);
		sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&mipOffset, &mipSize, &dummytexture2d,0, 0);
		sce::GpuAddress::tileSurface(finalTexels+mipOffset,linearTexels, &tp);
		dummytexture2d.setBaseAddress(finalTexels);
		delete [] linearTexels;
	}
	{
		sce::Gnm::TileMode offScreenTileMode;
		sce::GpuAddress::computeSurfaceTileMode(&offScreenTileMode,sce::GpuAddress::kSurfaceTypeRwTextureVolume, rformat, 1);
		sce::Gnm::SizeAlign sizeAlign = dummytexture3d.initAs3d(1,1,1,1,rformat
												,offScreenTileMode/*sce::Gnm::kTileModeThick_1dThick*/);//kTileModeDisplay_LinearAligned);
		dummytexture3d.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
		uint32_t ls				=rformat.getBytesPerElement();
		uint8_t *finalTexels	=(uint8_t*)GetMemoryInterface()->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
		
		memset(finalTexels,0x255,sizeAlign.m_size);
		
		sce::GpuAddress::TilingParameters tp;
		tp.initFromTexture(&dummytexture3d,0,0);
		// Update tiling params
		tp.m_mipLevel		=0;
		tp.m_linearWidth	=1;
		tp.m_linearHeight	=1;
		tp.m_linearDepth	=1;
		tp.m_baseTiledPitch =0;
		uint64_t mipOffset = 0;
		uint64_t mipSize = 0;
		uint8_t *linearTexels=new uint8_t[sizeAlign.m_size];
		memset(linearTexels,0x00,sizeAlign.m_size);
		sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&mipOffset, &mipSize, &dummytexture3d,0, 0);
		sce::GpuAddress::tileSurface(finalTexels+mipOffset,linearTexels, &tp);
		dummytexture3d.setBaseAddress(finalTexels);
		delete [] linearTexels;
	}

	{
		// Geometry shader needs ring buffers.
		// Use the default size for the ES->GS and GS->VS rings.
		kEsGsRingSizeInBytes =sce::Gnm::kGsRingSizeSetup4Mb;
		kGsVsRingSizeInBytes =sce::Gnm::kGsRingSizeSetup4Mb;

		// Allocate the ES->GS and the GS->VS ring buffers.
		// For performance reasons, align the ring buffers to cache line boundaries
		esgsRingBuffer = memoryInterface->AllocateVideoMemory(
			kEsGsRingSizeInBytes,
			sce::Gnm::kCacheLineSizeInBytes);
		gsvsRingBuffer = memoryInterface->AllocateVideoMemory(
			kGsVsRingSizeInBytes,
			sce::Gnm::kCacheLineSizeInBytes);
		// Allocate the global resource table, whatever the hell that is.
		globalResourceTable = memoryInterface->AllocateVideoMemory(
			SCE_GNM_SHADER_GLOBAL_TABLE_SIZE,
			sce::Gnm::kAlignmentOfBufferInBytes);
	}
}

void RenderPlatform::InvalidateDeviceObjects()
{
	contextState.clear();
	SAFE_DELETE(solidEffect);
	for(std::set<crossplatform::Material*>::iterator i=materials.begin();i!=materials.end();i++)
	{
		orbis::Material *mat=(orbis::Material*)(*i);
		mat->effect=solidEffect;
		delete mat;
	}
	materials.clear();
	{
		if(GetMemoryInterface())
		{
			if(dummytexture2d.getBaseAddress())
				GetMemoryInterface()->DeallocateVideoMemory(dummytexture2d.getBaseAddress());
			if(dummytexture3d.getBaseAddress())
				GetMemoryInterface()->DeallocateVideoMemory(dummytexture3d.getBaseAddress());
		}
		dummytexture2d.setBaseAddress(nullptr);
		dummytexture3d.setBaseAddress(nullptr);
	}
}

void RenderPlatform::RecompileShaders()
{
	std::map<std::string,std::string> defines;
	SAFE_DELETE(solidEffect);
	if(!device)
		return;
	solidEffect=CreateEffect("solid",defines);
	solidConstants.LinkToEffect(solidEffect,"SolidConstants");
	//solidConstants.LinkToProgram(solid_program,"SolidConstants",1);
	for(std::set<crossplatform::Material*>::iterator i=materials.begin();i!=materials.end();i++)
	{
		orbis::Material *mat=(orbis::Material*)(*i);
		mat->effect=solidEffect;
	}
	crossplatform::RenderPlatform::RecompileShaders();
}

void RenderPlatform::SynchronizeCacheAndState(crossplatform::DeviceContext &deviceContext)
{
	if(!deviceContext.platform_context)
		return;
	crossplatform::ContextState *cs=GetContextState(deviceContext);
}

void RenderPlatform::StartRender(crossplatform::DeviceContext &deviceContext)
{
	/*glPushAttrib(GL_ENABLE_BIT);
	glPushAttrib(GL_LIGHTING_BIT);
	glEnable(GL_DEPTH_TEST);
	// Draw the front face only, except for the texts and lights.
	glEnable(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);
	glUseProgram(solid_program);*/
}

void RenderPlatform::EndRender(crossplatform::DeviceContext &deviceContext)
{
	/*glUseProgram(0);
	glPopAttrib();
	glPopAttrib();*/
}

void RenderPlatform::IntializeLightingEnvironment(const float pAmbientLight[3])
{
 /*   glLightfv(GL_LIGHT0, GL_POSITION, DEFAULT_DIRECTION_LIGHT_POSITION);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, DEFAULT_LIGHT_COLOR);
    glLightfv(GL_LIGHT0, GL_SPECULAR, DEFAULT_LIGHT_COLOR);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, DEFAULT_LIGHT_SPOT_CUTOFF);
    glEnable(GL_LIGHT0);
    // Set ambient light.
    GLfloat lAmbientLight[] = {static_cast<GLfloat>(pAmbientLight[0]), static_cast<GLfloat>(pAmbientLight[1]),static_cast<GLfloat>(pAmbientLight[2]), 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lAmbientLight);*/
}

void RenderPlatform::BeginEvent	(crossplatform::DeviceContext &deviceContext,const char *name)
{
	deviceContext.asGfxContext()->pushMarker(
                name,
                *(uint32_t*)name
            );

}

void RenderPlatform::EndEvent(crossplatform::DeviceContext &deviceContext)
{
	deviceContext.asGfxContext()->popMarker();
}

bool RenderPlatform::ApplyContextState(crossplatform::DeviceContext &deviceContext,bool error_checking)
{
	crossplatform::ContextState *cs=GetContextState(deviceContext);
	if(!cs||!cs->currentEffectPass)
	{
		SIMUL_BREAK("No valid shader pass in ApplyContextState");
		return;
	}
	sce::Gnmx::LightweightGfxContext*gfxc=deviceContext.asGfxContext();

	// NULL ptr here if we've not applied a valid shader..
	orbis::EffectPass *pass=static_cast<orbis::EffectPass*>(cs->currentEffectPass);
	if(!cs->effectPassValid)
	{
		if(cs->last_action_was_compute&&pass->shaders[crossplatform::SHADERTYPE_VERTEX]!=nullptr)
		{
			graphicsWaitForCompute(deviceContext.asGfxContext());
			cs->last_action_was_compute=false;
		}
		else if((!cs->last_action_was_compute)&&pass->shaders[crossplatform::SHADERTYPE_COMPUTE]!=nullptr)
		{
			computeWaitForGraphics(deviceContext.asGfxContext());
			cs->last_action_was_compute=true;
		}
		// This applies the pass, and also any associated state: Blend, Depth and Rasterizer states:
		pass->Apply(deviceContext,false);
		cs->effectPassValid=true;
	}
	crossplatform::PixelOutputFormat pfm=GetCurrentPixelOutputFormat(deviceContext);

	if(pass->shaders[crossplatform::SHADERTYPE_GEOMETRY])
	{
		orbis::Shader *es=(orbis::Shader*)pass->shaders[crossplatform::SHADERTYPE_VERTEX];
		orbis::Shader *gs=(orbis::Shader*)pass->shaders[crossplatform::SHADERTYPE_GEOMETRY];
		gfxc->setGlobalResourceTableAddr(globalResourceTable);
		gfxc->setEsGsRingBuffer(
			esgsRingBuffer, kEsGsRingSizeInBytes,
			es->exportShader->m_memExportVertexSizeInDWord);
		gfxc->setGsVsRingBuffers(
			gsvsRingBuffer, kGsVsRingSizeInBytes,
			gs->geometryShader->m_memExportVertexSizeInDWord,
			gs->geometryShader->m_maxOutputVertexCount);
	//	gfxc->waitUntilSafeForRendering(videoOutHandle, backBuffer->displayIndex);
	// why in god's name would we have a videoOutHandle here?
	}
	// Override the above where requested:
	if(!cs->samplerStateOverridesValid)
	{
		cs->samplerStateOverridesValid=true;
		for(auto i=cs->samplerStateOverrides.begin();i!=cs->samplerStateOverrides.end();i++)
		{
			int slot = i->first;
			if (slot >=0&&i->second>0)
			{
				if(pass->shaders[crossplatform::SHADERTYPE_GEOMETRY])
				{
					deviceContext.asGfxContext()->setSamplers(sce::Gnm::kShaderStageEs,slot,1,i->second->AsGnmSampler());
					deviceContext.asGfxContext()->setSamplers(sce::Gnm::kShaderStageGs,slot,1,i->second->AsGnmSampler());
				}
				else if(pass->shaders[crossplatform::SHADERTYPE_VERTEX])
					deviceContext.asGfxContext()->setSamplers(sce::Gnm::kShaderStageVs,slot,1,i->second->AsGnmSampler());
				if(pass->shaders[crossplatform::SHADERTYPE_PIXEL])
					deviceContext.asGfxContext()->setSamplers(sce::Gnm::kShaderStagePs,slot,1,i->second->AsGnmSampler());
				if(pass->shaders[crossplatform::SHADERTYPE_COMPUTE])
					deviceContext.asGfxContext()->setSamplers(sce::Gnm::kShaderStageCs,slot,1,i->second->AsGnmSampler());
			}
		}
	}
	// Apply buffers:
	if(!cs->buffersValid&&pass->usesBuffers())
	{
		cs->bufferSlots=0;
		for(auto i=cs->applyBuffers.begin();i!=cs->applyBuffers.end();i++)
		{
			int slot=i->first;
			if(slot!=i->second->GetIndex())
			{
				SIMUL_BREAK_ONCE("This buffer assigned to the wrong index.");
			}
			i->second->GetPlatformConstantBuffer()->ActualApply(deviceContext,pass,i->second->GetIndex());
			if(error_checking&&pass->usesBufferSlot(slot))
				cs->bufferSlots|=(1<<slot);;
		}
		if(error_checking)
		{
			unsigned required_buffer_slots=pass->GetConstantBufferSlots();
			if((cs->bufferSlots&required_buffer_slots)!=required_buffer_slots)
			{
				SIMUL_BREAK_ONCE("Not all constant buffers are assigned.");
				unsigned missing_slots=required_buffer_slots&(~cs->bufferSlots);
				for(unsigned i=0;i<32;i++)
				{
					unsigned slot=1<<i;
					if(slot&missing_slots)
						SIMUL_CERR<<"Slot "<<i<<" was not set."<<std::endl;
				}
			}
		}
		//else
		//	cs->buffersValid=true;
		// Constant buffers allocated from the command buffer, therefore never mark "valid",
		// because the old value will be consumed.
	}
	if(!cs->streamoutTargetsValid)
	{
		for(auto i=cs->streamoutTargets.begin();i!=cs->streamoutTargets.end();i++)
		{
			int slot=i->first;
			orbis::Buffer *vertexBuffer=(orbis::Buffer*)i->second;
			if(!vertexBuffer)
				continue;
			
			sce::Gnm::Buffer *b=vertexBuffer->AsGnmBuffer();
			if(!b)
				continue;
			// TODO: nonzero offsets.
			unsigned offset = 0;//vertexBuffer?(vertexBuffer->stride*start_index):0;

			// Setup streamout config:
	
			sce::Gnm::StreamoutBufferMapping bufferBinding;
			bufferBinding.init();
			bufferBinding.bindStream(sce::Gnm::kStreamoutBuffer0,sce::Gnm::StreamoutBufferMapping::kGsStreamBuffer0);
			sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
			// Setting partial vs wave, needed for streamout, the rest of the parameters are default values
			/*if(gpuMode==sce::Gnm::GpuMode::kGpuModeBase)
				gfxc->setVgtControlForBase(255,sce::Gnm::VgtPartialVsWaveMode::kVgtPartialVsWaveDisable);
			else
			{
				/// God knows what all this means:
				gfxc->setVgtControlForNeo, sce::Gnm::kWdSwitchOnlyOnEopEnable, sce::Gnm::VgtPartialVsWaveMode::kVgtPartialVsWaveDisable);
			}
			*/
			gfxc->setVgtControl(255,sce::Gnm::VgtPartialVsWaveMode::kVgtPartialVsWaveDisable);
			// Setting streamout parameters
			gfxc->flushStreamout();
			gfxc->setStreamoutMapping(&bufferBinding);
			unsigned bufferSizeDW=b->getSize()/4;;
			unsigned bufferStrideDW=b->getStride()/4;;
			gfxc->setStreamoutBufferDimensions(sce::Gnm::kStreamoutBuffer0,bufferSizeDW,bufferStrideDW);
			gfxc->writeStreamoutBufferOffset(sce::Gnm::kStreamoutBuffer0,0);
		}
		cs->streamoutTargetsValid=true;
	}
	if(!cs->vertexBuffersValid)
	{
		for(auto i:cs->applyVertexBuffers)
		{
			//if(pass->UsesBufferSlot(i.first))
			orbis::Buffer* b=(orbis::Buffer*)(i.second);
			sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
			
			if(pass->shaders[crossplatform::SHADERTYPE_GEOMETRY]!=nullptr)
			{
				gfxc->setVertexBuffers(sce::Gnm::kShaderStageEs,i.first,b->count,b->AsGnmBuffer());
				gfxc->setVertexBuffers(sce::Gnm::kShaderStageGs,i.first,b->count,b->AsGnmBuffer());
			}
			else if(pass->shaders[crossplatform::SHADERTYPE_VERTEX]!=nullptr)
				gfxc->setVertexBuffers(sce::Gnm::kShaderStageVs,i.first,b->count,b->AsGnmBuffer());
		}
		cs->vertexBuffersValid=true;
	}
	cs->textureSlotsForSB=0;
	cs->rwTextureSlotsForSB=0;
	if(pass->usesSBs())
	{
		for(auto i=cs->applyStructuredBuffers.begin();i!=cs->applyStructuredBuffers.end();i++)
		{
			int slot=i->first;
			if(!pass->usesTextureSlotForSB(slot))
				continue;
			i->second->ActualApply(deviceContext,pass,slot);
			Shader **sh=(orbis::Shader**)pass->shaders;
			if(slot<1000)
			{
				if(sh[crossplatform::SHADERTYPE_VERTEX]&&sh[crossplatform::SHADERTYPE_VERTEX]->usesTextureSlotForSB(slot))
					cs->textureSlotsForSB|=(1<<slot);
				if(sh[crossplatform::SHADERTYPE_PIXEL]&&sh[crossplatform::SHADERTYPE_PIXEL]->usesTextureSlotForSB(slot))
					cs->textureSlotsForSB|=(1<<slot);
				if(sh[crossplatform::SHADERTYPE_GEOMETRY]&&sh[crossplatform::SHADERTYPE_GEOMETRY]->usesTextureSlotForSB(slot))
					cs->textureSlotsForSB|=(1<<slot);
				if(sh[crossplatform::SHADERTYPE_COMPUTE]&&sh[crossplatform::SHADERTYPE_COMPUTE]->usesTextureSlotForSB(slot))
					cs->textureSlotsForSB|=(1<<slot);
			}
			else
			{
				if(sh[crossplatform::SHADERTYPE_COMPUTE]&&sh[crossplatform::SHADERTYPE_COMPUTE]->usesRwTextureSlotForSB(slot-1000))
					cs->rwTextureSlotsForSB|=(1<<(slot-1000));
			}
		}
		if(error_checking)
		{
			unsigned required_sb_slots=pass->GetStructuredBufferSlots();
			if((cs->textureSlotsForSB&required_sb_slots)!=required_sb_slots)
			{
				SIMUL_BREAK_ONCE("Not all texture slots are assigned.");
			}
			unsigned required_rw_sb_slots=pass->GetRwStructuredBufferSlots();
			if((cs->rwTextureSlotsForSB&required_rw_sb_slots)!=required_rw_sb_slots)
			{
				SIMUL_BREAK_ONCE("Not all texture slots are assigned.");
			}
		}
	}
	// Apply textures:
	if(!cs->textureAssignmentMapValid&&pass->usesTextures())
	{
		cs->textureSlots=0;
		cs->rwTextureSlots=0;
		cs->textureAssignmentMapValid=true;
		for(auto i=cs->textureAssignmentMap.begin();i!=cs->textureAssignmentMap.end();i++)
		{
			int slot=i->first;
			if (slot<0)
				continue;
			const crossplatform::TextureAssignment &ta=i->second;
			{
				sce::Gnm::Texture *t;
				if(ta.texture&&ta.texture->IsValid())
					t=ta.texture->AsGnmTexture(ta.resourceType,ta.index,ta.mip);
				else
					t=GetDummyTexture(ta.dimensions);
				if(!t)
					continue;
				Shader **sh=(Shader**)pass->shaders;
				if(ta.uav)
				{
					SIMUL_ASSERT_WARN_ONCE(slot>=1000,"Bad slot number");
					SIMUL_ASSERT_WARN_ONCE(slot-1000<sce::Gnmx::LightweightConstantUpdateEngine::kMaxRwResourceCount,"Bad slot number - check whether sce::Gnmx::LightweightConstantUpdateEngine::kMaxRwResourceCount is too small.");
					if(sh[crossplatform::SHADERTYPE_COMPUTE]
						&&sh[crossplatform::SHADERTYPE_COMPUTE]->usesRwTextureSlot(slot-1000))
					{
						deviceContext.asGfxContext()->setRwTextures(sce::Gnm::kShaderStageCs, slot-1000, 1, t);
						cs->rwTextureSlots|=1<<(slot-1000);
					}
				}
				else
				{
					SIMUL_ASSERT_WARN_ONCE(slot<1000,"Bad slot number");
					SIMUL_ASSERT_WARN_ONCE(slot<sce::Gnmx::LightweightConstantUpdateEngine::kMaxResourceCount,"Bad slot number - check whether sce::Gnmx::LightweightConstantUpdateEngine::kMaxResourceCount is too small.");
					if(sh[crossplatform::SHADERTYPE_GEOMETRY]&&sh[crossplatform::SHADERTYPE_GEOMETRY]->usesTextureSlot(slot))
					{
						deviceContext.asGfxContext()->setTextures(sce::Gnm::kShaderStageGs, slot, 1, t);
						cs->textureSlots|=(1<<slot);
					}
					if(sh[crossplatform::SHADERTYPE_VERTEX]&&sh[crossplatform::SHADERTYPE_VERTEX]->usesTextureSlot(slot))
					{
						if(sh[crossplatform::SHADERTYPE_GEOMETRY])
							deviceContext.asGfxContext()->setTextures(sce::Gnm::kShaderStageEs, slot, 1, t);
						else
							deviceContext.asGfxContext()->setTextures(sce::Gnm::kShaderStageVs, slot, 1, t);
						cs->textureSlots|=(1<<slot);
					}
					Shader *p = (orbis::Shader*)((orbis::EffectPass*)cs->currentEffectPass)->pixelShaders[pfm];
					if(p&&p->usesTextureSlot(slot))
					{
						deviceContext.asGfxContext()->setTextures(sce::Gnm::kShaderStagePs, slot, 1, t);
						cs->textureSlots|=(1<<slot);
					}
					if(sh[crossplatform::SHADERTYPE_COMPUTE]&&sh[crossplatform::SHADERTYPE_COMPUTE]->usesTextureSlot(slot))
					{
						deviceContext.asGfxContext()->setTextures(sce::Gnm::kShaderStageCs, slot, 1, t);
						cs->textureSlots|=(1<<slot);
					}
				}
			}
		}
		// Now verify that ALL resource are set:
		if(error_checking)
		{
			unsigned required_slots=pass->GetTextureSlots();
			if((cs->textureSlots&required_slots)!=required_slots)
			{
				static int count=10;
				count--;
				if(count>0)
				{
					SIMUL_CERR<<"Not all texture slots are assigned:"<<std::endl;
					unsigned missing_slots=required_slots&(~cs->textureSlots);
					for(unsigned i=0;i<32;i++)
					{
						unsigned slot=1<<i;
						if(slot&missing_slots)
						{
							std::string name;
							if(cs->currentEffect)
								name=cs->currentEffect->GetTextureForSlot(i);
							SIMUL_CERR<<"\tSlot "<<i<<": "<<name.c_str()<<", was not set."<<std::endl;
						}
					}
					SIMUL_BREAK_ONCE("Many API's require all used textures to have valid data.");
				}
			}
			unsigned required_rw_slots=pass->GetRwTextureSlots();
			if((cs->rwTextureSlots&required_rw_slots)!=required_rw_slots)
			{
				static int count=10;
				count--;
				if(count>0)
				{
					SIMUL_BREAK_ONCE("Not all rw texture slots are assigned.");
					required_rw_slots=required_rw_slots&(~cs->rwTextureSlots);
					for(unsigned i=0;i<32;i++)
					{
						unsigned slot=1<<i;
						if(slot&required_rw_slots)
						{
							std::string name;
							if(cs->currentEffect)
								name=cs->currentEffect->GetTextureForSlot(1000+i);
							SIMUL_CERR<<"RW Slot "<<i<<" was not set ("<<name.c_str()<<")."<<std::endl;
						}
					}
				}
			}
		}
	}
	return true;
}
#include "Simul/Base/StringFunctions.h"
// sce::Gnmx::Toolkit::SurfaceUtil::copyTexture:
void RenderPlatform::CopyGnmTexture(crossplatform::DeviceContext &deviceContext, const sce::Gnm::Texture* textureDst, const sce::Gnm::Texture* textureSrc)
{
	SIMUL_ASSERT_WARN_ONCE(textureDst != 0 && textureSrc != 0, "textureDst and textureSrc must not be NULL.");
	SIMUL_ASSERT_WARN_ONCE(textureDst->getWidth()  == textureSrc->getWidth(),base::QuickFormat("source and destination texture widths do not match (dest=%d, source=%d).", textureDst->getWidth(), textureSrc->getWidth()));
//	SIMUL_ASSERT_WARN_ONCE(textureDst->getLastMipLevel() - textureDst->getBaseMipLevel() <= textureSrc->getLastMipLevel() - textureSrc->getBaseMipLevel(),
	//	"textureDst can not have more mip levels than textureSrc.");
	SIMUL_ASSERT_WARN_ONCE(textureDst->getLastArraySliceIndex() - textureDst->getBaseArraySliceIndex() <= textureSrc->getLastArraySliceIndex() - textureSrc->getBaseArraySliceIndex(),
		"textureDst can not have more array slices than textureSrc.");

	// If the data formats of the two textures are identical, we use a different shader that loads and stores the raw pixel bits directly, without any format conversion.
	// This not only preserves precision, but allows some additional tricks (such as copying otherwise-unwritable block-compressed formats by "spoofing" them as writable formats with identical
	// per-pixel sizes).
	bool copyRawPixels = (textureDst->getDataFormat().m_asInt == textureSrc->getDataFormat().m_asInt);

	sce::Gnm::Texture textureDstCopy = *textureDst;
	sce::Gnm::Texture textureSrcCopy = *textureSrc;

	if(copyRawPixels)
	{
		sce::Gnm::DataFormat dataFormat = textureDstCopy.getDataFormat();
		switch(dataFormat.getSurfaceFormat())
		{
			case sce::Gnm::kSurfaceFormatBc1: 
			case sce::Gnm::kSurfaceFormatBc4:
				dataFormat.m_bits.m_channelType = sce::Gnm::kTextureChannelTypeUInt;
				dataFormat.m_bits.m_surfaceFormat = sce::Gnm::kSurfaceFormat32_32;
				textureDstCopy.setWidthMinus1((textureDstCopy.getWidth() + 3) / 4 - 1);
				textureDstCopy.setHeightMinus1((textureDstCopy.getHeight() + 3) / 4 - 1);
				textureSrcCopy.setWidthMinus1((textureSrcCopy.getWidth() + 3) / 4 - 1);
				textureSrcCopy.setHeightMinus1((textureSrcCopy.getHeight() + 3) / 4 - 1);
				break;
			case sce::Gnm::kSurfaceFormatBc2: 
			case sce::Gnm::kSurfaceFormatBc3:
			case sce::Gnm::kSurfaceFormatBc5:
			case sce::Gnm::kSurfaceFormatBc6:
			case sce::Gnm::kSurfaceFormatBc7:
				dataFormat.m_bits.m_channelType = sce::Gnm::kTextureChannelTypeUInt;
				dataFormat.m_bits.m_surfaceFormat = sce::Gnm::kSurfaceFormat32_32_32_32;
				textureDstCopy.setWidthMinus1((textureDstCopy.getWidth() + 3) / 4 - 1);
				textureDstCopy.setHeightMinus1((textureDstCopy.getHeight() + 3) / 4 - 1);
				textureSrcCopy.setWidthMinus1((textureSrcCopy.getWidth() + 3) / 4 - 1);
				textureSrcCopy.setHeightMinus1((textureSrcCopy.getHeight() + 3) / 4 - 1);
				break;
		/*	case sce::Gnm::kSurfaceFormat16_16_16_16:
				dataFormat.m_bits.m_channelType = sce::Gnm::kTextureChannelTypeUInt;
				dataFormat.m_bits.m_surfaceFormat = sce::Gnm::kSurfaceFormat32_32;
				textureDstCopy.setWidthMinus1((textureDstCopy.getWidth() + 1) / 2 - 1);
				textureDstCopy.setHeightMinus1((textureDstCopy.getHeight() + 1) / 2 - 1);
				textureSrcCopy.setWidthMinus1((textureSrcCopy.getWidth() + 1) / 2 - 1);
				textureSrcCopy.setHeightMinus1((textureSrcCopy.getHeight() + 1) / 2 - 1);
				break;*/
			default:
				break;
		}
		textureDstCopy.setDataFormat(dataFormat);
		textureSrcCopy.setDataFormat(dataFormat);
	}
	crossplatform::EffectTechnique *tech=nullptr;
	const char *techname=nullptr;
	switch(textureDst->getTextureType())
	{
	case sce::Gnm::kTextureType1d:
	case sce::Gnm::kTextureType1dArray:
		SIMUL_ASSERT_WARN(textureSrc->getTextureType() == sce::Gnm::kTextureType1d || textureSrc->getTextureType() == sce::Gnm::kTextureType1dArray,
			base::QuickFormat("textureDst and textureSrc must have the same dimensionality (dst=0x%02X, src=0x%02X).", textureDst->getTextureType(), textureSrc->getTextureType()));
		techname = copyRawPixels ? "copy_1d_int4" : "copy_1d";
		break;
	case sce::Gnm::kTextureTypeCubemap:
		// Spoof the cubemap textures as 2D texture arrays.
		textureDstCopy.initAs2dArray(textureDstCopy.getWidth(), textureDstCopy.getHeight(), textureDstCopy.getLastArraySliceIndex()+1, textureDstCopy.getLastMipLevel()+1, textureDstCopy.getDataFormat(), textureDstCopy.getTileMode(), textureDstCopy.getNumFragments(), false);
		textureSrcCopy.initAs2dArray(textureSrcCopy.getWidth(), textureSrcCopy.getHeight(), textureSrcCopy.getLastArraySliceIndex()+1, textureSrcCopy.getLastMipLevel()+1, textureSrcCopy.getDataFormat(), textureSrcCopy.getTileMode(), textureSrcCopy.getNumFragments(), false);
		textureDstCopy.setBaseAddress(textureDst->getBaseAddress());
		textureSrcCopy.setBaseAddress(textureSrc->getBaseAddress());
		// Intentional fall-through
	case sce::Gnm::kTextureType2d:
	case sce::Gnm::kTextureType2dArray:
		SIMUL_ASSERT_WARN(textureDst->getHeight() == textureSrc->getHeight(),base::QuickFormat("source and destination texture heights do not match (dest=%d, source=%d).", textureDst->getHeight(), textureSrc->getHeight()));
		SIMUL_ASSERT_WARN(textureSrc->getTextureType() == sce::Gnm::kTextureType2d || textureSrc->getTextureType() == sce::Gnm::kTextureType2dArray || textureSrc->getTextureType() == sce::Gnm::kTextureTypeCubemap,
			base::QuickFormat("textureDst and textureSrc must have the same dimensionality (dst=0x%02X, src=0x%02X).", textureDst->getTextureType(), textureSrc->getTextureType()));
		techname = copyRawPixels ?"copy_2d_int4" : "copy_2d";
		break;
	case sce::Gnm::kTextureType3d:
		SIMUL_ASSERT_WARN(textureDst->getHeight() == textureSrc->getHeight(),base::QuickFormat("source and destination texture heights do not match (dest=%d, source=%d).", textureDst->getHeight(), textureSrc->getHeight()));
		SIMUL_ASSERT_WARN(textureDst->getDepth() == textureSrc->getDepth(),base::QuickFormat("source and destination texture depths do not match (dest=%d, source=%d).", textureDst->getDepth(), textureSrc->getDepth()));
		SIMUL_ASSERT_WARN(textureSrc->getTextureType() == sce::Gnm::kTextureType3d,base::QuickFormat(
			"textureDst and textureSrc must have the same dimensionality (dst=0x%02X, src=0x%02X).", textureDst->getTextureType(), textureSrc->getTextureType()));
		techname = copyRawPixels ?"copy_3d_int4" : "copy_3d";
		break;
	default:
		break; // unsupported texture type -- handled below
	}
	if(techname == 0)
	{
		SIMUL_BREAK_ONCE(base::QuickFormat("textureDst's dimensionality (0x%02X) is not supported by this function.", textureDst->getTextureType()));
		return;
	}
	copyEffect->Apply(deviceContext,techname);
	//gfxc.setCsShader(shader->m_shader, &shader->m_offsetsTable);

	textureDstCopy.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC); // The destination texture is GPU-coherent, because we will write to it.
	textureSrcCopy.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeRO); // The source texture is read-only, because we'll only ever read from it.

	const uint32_t oldDstMipBase   = textureDstCopy.getBaseMipLevel();
	const uint32_t oldDstMipLast   = textureDstCopy.getLastMipLevel();
	const uint32_t oldDstSliceBase = textureDstCopy.getBaseArraySliceIndex();
	const uint32_t oldDstSliceLast = textureDstCopy.getLastArraySliceIndex();
	for(uint32_t iMip=oldDstMipBase; iMip <= oldDstMipLast; ++iMip)
	{
		textureSrcCopy.setMipLevelRange(iMip, iMip);
		textureDstCopy.setMipLevelRange(iMip, iMip);
		const uint32_t mipWidth  = std::max(textureDstCopy.getWidth() >> iMip, 1U);
		const uint32_t mipHeight = std::max(textureDstCopy.getHeight() >> iMip, 1U);
		const uint32_t mipDepth  = std::max(textureDstCopy.getDepth() >> iMip, 1U);
		for(uint32_t iSlice=oldDstSliceBase; iSlice <= oldDstSliceLast; ++iSlice)
		{
			textureSrcCopy.setArrayView(iSlice, iSlice);
			textureDstCopy.setArrayView(iSlice, iSlice);
			ApplyContextState(deviceContext,false);
			deviceContext.asGfxContext()->setTextures( sce::Gnm::kShaderStageCs, 0, 1, &textureSrcCopy );
			deviceContext.asGfxContext()->setRwTextures( sce::Gnm::kShaderStageCs, 0, 1, &textureDstCopy );

			switch(textureDstCopy.getTextureType())
			{
			case sce::Gnm::kTextureType1d:
			case sce::Gnm::kTextureType1dArray:
				DispatchCompute(deviceContext, (mipWidth+63)/64, 1, 1);
				break;
			case sce::Gnm::kTextureTypeCubemap:
			case sce::Gnm::kTextureType2d:
			case sce::Gnm::kTextureType2dArray:
				DispatchCompute(deviceContext, (mipWidth+7)/8, (mipHeight+7)/8, 1);
				break;
			case sce::Gnm::kTextureType3d:
				DispatchCompute(deviceContext, (mipWidth+3)/4, (mipHeight+3)/4, (mipDepth+3)/4 );
				break;
			default:
				SIMUL_BREAK_ONCE(""); // This path should have been caught in the previous switch statement
				return;
			}
		}
	}
	copyEffect->Unapply(deviceContext);
	//Toolkit::synchronizeComputeToGraphics(&gfxc.m_dcb);
}
void RenderPlatform::CopyTexture(crossplatform::DeviceContext &deviceContext,crossplatform::Texture *dst,crossplatform::Texture *src)
{
	// compute copy
/*	if(dst->IsComputable())
	{
		SIMUL_BREAK_ONCE("Not implemented yet");
	}
	else if(dst->HasRenderTargets())
	{
		debugEffect->Apply(deviceContext,debugEffect->GetTechniqueByName("copy_2d"),"replace");
		for(int i=0;i<dst->NumFaces()&&i<src->NumFaces();i++)
		{
			for(int j=0;j<dst->mips&&j<src->mips;j++)
			{
				debugEffect->SetTexture(deviceContext,"imageTexture",src,i,j);
				debugEffect->Reapply(deviceContext);
				dst->activateRenderTarget(deviceContext,i,j);
				DrawQuad(deviceContext);
				//if(i==0)
				//	DrawTexture(deviceContext,0,0,64,64,nullptr,vec4(1,0,1,.5));
				dst->deactivateRenderTarget(deviceContext);
			}
		}
		debugEffect->Unapply(deviceContext);
	}*/
	//else	
	{
		if(dst->arraySize==src->arraySize&&dst->width==src->width&&dst->length==src->length&&dst->depth==src->depth)
		{
			CopyGnmTexture(deviceContext,dst->AsGnmTexture(),src->AsGnmTexture());
		}
		else
		{
			SIMUL_BREAK_ONCE("Not implemented yet");
		}
	}
}

void RenderPlatform::DispatchCompute(crossplatform::DeviceContext &deviceContext,int w,int l,int d)
{
	if(!w||!l||!d)
	{
		SIMUL_CERR<<"Empty dispatch command!"<<std::endl;
		return;
	}
	ApplyContextState(deviceContext);
	auto gfxc=deviceContext.asGfxContext();

	{
		// Does this draw call use any fenced resources?
		WaitForFencedResources(deviceContext);
	}
	SIMUL_PS4_VALIDATE
	gfxc->dispatch(w, l, d);
	{
		// assign this fence to all active UAV textures.
		crossplatform::ContextState *cs=GetContextState(deviceContext);
		if(cs->currentTechnique&&cs->currentTechnique->shouldFenceOutputs())
		{
			volatile uint64_t* label = (volatile uint64_t*)gfxc->allocateFromCommandBuffer( sizeof(uint64_t), sce::Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
			*label = 0x0; // set the memory to have the val 0
			gfxc->writeAtEndOfShader( sce::Gnm::kEosCsDone, const_cast<uint64_t*>(label), 0x1 ); // tell the CP to write a 1 into the memory only when all compute shaders have finished

			long long fence=(long long)label;
			for(auto i=cs->textureAssignmentMap.begin();i!=cs->textureAssignmentMap.end();i++)
			{
				int slot=i->first;
				if (slot<1000)
					continue;
				const crossplatform::TextureAssignment &ta=i->second;
				if(ta.texture&&!ta.texture->IsUnfenceable())
				{
					ta.texture->SetFence(fence);
					fencedTextures.insert(ta.texture);
				}
			}
		}
	}

}

void RenderPlatform::WaitForGpu(crossplatform::DeviceContext &deviceContext)
{
	if((&deviceContext)==(&immediateContext)&&gfxc_i==immediateContext.platform_context)
	{
		auto gfxc=deviceContext.asGfxContext();
		volatile uint64_t* label = (volatile uint64_t*)gfxc->allocateFromCommandBuffer( sizeof(uint64_t), sce::Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
		*label = 0x0; // set the memory to have the val 0
		gfxc->writeImmediateAtEndOfPipe(sce::Gnm::kEopFlushCbDbCaches,(void *)label,0x1,sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2);
		gfxc->submit();
		volatile uint32_t wait = 0;
		//gfxc->waitOnAddress((void*)label, 0xffffffff,sce::Gnm::kWaitCompareFuncEqual, 0x1);
		while(*label != 0x1)
		{
			wait++;
		} 
		if(wait>0)
		{
			SIMUL_CERR<<"Waited "<<wait<<" times in WaitForGpu"<<std::endl;
		}
		gfxc->reset();
		gfxc->initializeToDefaultContextState();
	}
	else
		graphicsWaitForCompute(deviceContext.asGfxContext());
}

void RenderPlatform::ApplyShaderPass(crossplatform::DeviceContext &deviceContext,crossplatform::Effect *effect,crossplatform::EffectTechnique *tech,int index)
{
}

void RenderPlatform::Draw(crossplatform::DeviceContext &deviceContext,int num_verts,int start_vert)
{
	SIMUL_PS4_VALIDATE3(deviceContext)
	ApplyContextState(deviceContext);
	deviceContext.asGfxContext()->drawIndexAuto(num_verts,start_vert,0);
	SIMUL_PS4_VALIDATE3(deviceContext)
}

void RenderPlatform::DrawIndexed(crossplatform::DeviceContext &deviceContext,int num_indices,int start_index,int base_vertex)
{
	SIMUL_PS4_VALIDATE2(deviceContext)
	ApplyContextState(deviceContext);
	deviceContext.asGfxContext()->drawIndexOffset(base_vertex,num_indices);
}
void RenderPlatform::DrawMarker(crossplatform::DeviceContext &deviceContext,const double *matrix)
{
 /*   glColor3f(0.0, 1.0, 1.0);
    glLineWidth(1.0);
	
	glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixd((const double*) matrix);

    glBegin(GL_LINE_LOOP);
        glVertex3f(+1.0f, -1.0f, +1.0f);
        glVertex3f(+1.0f, -1.0f, -1.0f);
        glVertex3f(+1.0f, +1.0f, -1.0f);
        glVertex3f(+1.0f, +1.0f, +1.0f);

        glVertex3f(+1.0f, +1.0f, +1.0f);
        glVertex3f(+1.0f, +1.0f, -1.0f);
        glVertex3f(-1.0f, +1.0f, -1.0f);
        glVertex3f(-1.0f, +1.0f, +1.0f);

        glVertex3f(+1.0f, +1.0f, +1.0f);
        glVertex3f(-1.0f, +1.0f, +1.0f);
        glVertex3f(-1.0f, -1.0f, +1.0f);
        glVertex3f(+1.0f, -1.0f, +1.0f);

        glVertex3f(-1.0f, -1.0f, +1.0f);
        glVertex3f(-1.0f, +1.0f, +1.0f);
        glVertex3f(-1.0f, +1.0f, -1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);

        glVertex3f(-1.0f, -1.0f, +1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glVertex3f(+1.0f, -1.0f, -1.0f);
        glVertex3f(+1.0f, -1.0f, +1.0f);

        glVertex3f(-1.0f, -1.0f, -1.0f);
        glVertex3f(-1.0f, +1.0f, -1.0f);
        glVertex3f(+1.0f, +1.0f, -1.0f);
        glVertex3f(+1.0f, -1.0f, -1.0f);
    glEnd();
    glPopMatrix();*/
}

void RenderPlatform::DrawLine(crossplatform::DeviceContext &deviceContext, const float *pGlobalBasePosition, const float *pGlobalEndPosition, const float *colour, float width)
{
/*    glColor3f(colour[0],colour[1],colour[2]);
    glLineWidth(width);

    glBegin(GL_LINES);

    glVertex3dv((const GLdouble *)pGlobalBasePosition);
    glVertex3dv((const GLdouble *)pGlobalEndPosition);

    glEnd();*/
}

void RenderPlatform::DrawCrossHair(crossplatform::DeviceContext &deviceContext,const double *pGlobalPosition)
{
/*    glColor3f(1.0, 1.0, 1.0);
    glLineWidth(1.0);
	
	glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixd((double*) pGlobalPosition);

    double lCrossHair[6][3] = { { -3, 0, 0 }, { 3, 0, 0 },
    { 0, -3, 0 }, { 0, 3, 0 },
    { 0, 0, -3 }, { 0, 0, 3 } };

    glBegin(GL_LINES);

    glVertex3dv(lCrossHair[0]);
    glVertex3dv(lCrossHair[1]);

    glEnd();

    glBegin(GL_LINES);

    glVertex3dv(lCrossHair[2]);
    glVertex3dv(lCrossHair[3]);

    glEnd();

    glBegin(GL_LINES);

    glVertex3dv(lCrossHair[4]);
    glVertex3dv(lCrossHair[5]);

    glEnd();
	
	glMatrixMode(GL_MODELVIEW);
    glPopMatrix();*/
}

void RenderPlatform::DrawCamera(crossplatform::DeviceContext &deviceContext,const double *pGlobalPosition, double pRoll)
{
 /*   glColor3d(1.0, 1.0, 1.0);
    glLineWidth(1.0);
	
	glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixd((const double*) pGlobalPosition);
    glRotated(pRoll, 1.0, 0.0, 0.0);

    int i;
    float lCamera[10][2] = {{ 0, 5.5 }, { -3, 4.5 },
    { -3, 7.5 }, { -6, 10.5 }, { -23, 10.5 },
    { -23, -4.5 }, { -20, -7.5 }, { -3, -7.5 },
    { -3, -4.5 }, { 0, -5.5 }   };

    glBegin( GL_LINE_LOOP );
    {
        for (i = 0; i < 10; i++)
        {
            glVertex3f(lCamera[i][0], lCamera[i][1], 4.5);
        }
    }
    glEnd();

    glBegin( GL_LINE_LOOP );
    {
        for (i = 0; i < 10; i++)
        {
            glVertex3f(lCamera[i][0], lCamera[i][1], -4.5);
        }
    }
    glEnd();

    for (i = 0; i < 10; i++)
    {
        glBegin( GL_LINES );
        {
            glVertex3f(lCamera[i][0], lCamera[i][1], -4.5);
            glVertex3f(lCamera[i][0], lCamera[i][1], 4.5);
        }
        glEnd();
    }
    glPopMatrix();*/
}

void RenderPlatform::DrawLineLoop(crossplatform::DeviceContext &deviceContext,const double *mat,int lVerticeCount,const double *vertexArray,const float colr[4])
{
/*    glPushMatrix();
    glMultMatrixd((const double*)mat);
	glColor3f(colr[0],colr[1],colr[2]);
	glBegin(GL_LINE_LOOP);
	for (int lVerticeIndex = 0; lVerticeIndex < lVerticeCount; lVerticeIndex++)
	{
		glVertex3dv((GLdouble *)&vertexArray[lVerticeIndex*3]);
	}
	glEnd();
    glPopMatrix();*/
}

void RenderPlatform::ApplyDefaultMaterial()
{
 //   const float BLACK_COLOR[] = {0.0f, 0.0f, 0.0f, 1.0f};
    //const float GREEN_COLOR[] = {0.0f, 1.0f, 0.0f, 1.0f};
//    const GLfloat WHITE_COLOR[] = {1.0f, 1.0f, 1.0f, 1.0f};
/*    glMaterialfv(GL_FRONT, GL_EMISSION, BLACK_COLOR);
    glMaterialfv(GL_FRONT, GL_AMBIENT, BLACK_COLOR);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, GREEN_COLOR);
    glMaterialfv(GL_FRONT, GL_SPECULAR, BLACK_COLOR);
    glMaterialf(GL_FRONT, GL_SHININESS, 0);

    glBindTexture(GL_TEXTURE_2D, 0);*/
}

void MakeWorldViewProjMatrix(float *wvp,const double *w,const float *v,const float *p)
{
	simul::math::Matrix4x4 tmp1,view(v),proj(p),model(w);
	simul::math::Multiply4x4(tmp1,model,view);
	simul::math::Multiply4x4(*(simul::math::Matrix4x4*)wvp,tmp1,proj);
}

void RenderPlatform::SetModelMatrix(crossplatform::DeviceContext &deviceContext,const double *m,const crossplatform::PhysicalLightRenderData &)
{
//	glGetFloatv(GL_PROJECTION_MATRIX,proj.RowPointer(0));
//	glGetFloatv(GL_MODELVIEW_MATRIX,view.RowPointer(0));
	simul::math::Matrix4x4 wvp;
	simul::math::Matrix4x4 viewproj;
	simul::math::Matrix4x4 modelviewproj;
	simul::math::Multiply4x4(viewproj,deviceContext.viewStruct.view,deviceContext.viewStruct.proj);
	simul::math::Matrix4x4 model(m);
	simul::math::Multiply4x4(modelviewproj,model,viewproj);
	solidConstants.worldViewProj=modelviewproj;
	ID3D11DeviceContext *pContext=(ID3D11DeviceContext*)deviceContext.asD3D11DeviceContext();
	debugEffect->SetConstantBuffer(deviceContext,&solidConstants);

	//effect->GetTechniqueByName("solid")->GetPassByIndex(0)->Apply(0,pContext);
}

crossplatform::Material *RenderPlatform::CreateMaterial()
{
	orbis::Material *mat=new orbis::Material;
	mat->effect=solidEffect;
	materials.insert(mat);
	return mat;
}

crossplatform::Mesh *RenderPlatform::CreateMesh()
{
	return new orbis::Mesh();
}

crossplatform::Light *RenderPlatform::CreateLight()
{
	return new orbis::Light();
}

crossplatform::Texture *RenderPlatform::CreateTexture(const char *fileNameUtf8)
{
	crossplatform::Texture * tex=new orbis::Texture(fileNameUtf8);
	std::string f=fileNameUtf8?fileNameUtf8:"";
	
	if(f.find(".")<f.length())
		tex->LoadFromFile(this,fileNameUtf8);
	return tex;
}

crossplatform::BaseFramebuffer *RenderPlatform::CreateFramebuffer(const char *name)
{
	return new orbis::Framebuffer(name);
}

crossplatform::SamplerState *RenderPlatform::CreateSamplerState(crossplatform::SamplerStateDesc *d)
{
	orbis::SamplerState *ss=new orbis::SamplerState(d);
	
	return ss;
}

crossplatform::Effect *RenderPlatform::CreateEffect() 
{
	orbis::Effect*e=new orbis::Effect();
	return e;
}

crossplatform::Effect *RenderPlatform::CreateEffect(const char *filename_utf8,const std::map<std::string,std::string> &defines)
{
	std::string fn(filename_utf8);
	if(fn.find(".")>=fn.length())
		fn+=".sfxo";
	orbis::Effect *e=(orbis::Effect*)CreateEffect();
	e->Load(this, fn.c_str(), defines);
	if (!e->techniques.size())
	{
		delete e;
		return NULL;
	}
	return e;
}

crossplatform::PlatformConstantBuffer *RenderPlatform::CreatePlatformConstantBuffer()
{
	crossplatform::PlatformConstantBuffer *b=new orbis::PlatformConstantBuffer();
	return b;
}

crossplatform::PlatformStructuredBuffer	*RenderPlatform::CreatePlatformStructuredBuffer()
{
	crossplatform::PlatformStructuredBuffer *b=new orbis::PlatformStructuredBuffer();
	return b;
}

crossplatform::Buffer *RenderPlatform::CreateBuffer()
{
	crossplatform::Buffer *b=new orbis::Buffer();
	return b;
}

crossplatform::Layout *RenderPlatform::CreateLayout(int num_elements,const crossplatform::LayoutDesc *desc)
{
	orbis::Layout *l=new orbis::Layout();
	l->SetDesc(desc,num_elements);
	return l;
}

sce::Gnm::CompareFunc RenderPlatform::ToGnmDepthComparison(crossplatform::DepthComparison d)
{
	switch(d)
	{											
	case crossplatform::DEPTH_ALWAYS:			return sce::Gnm::kCompareFuncAlways     ; 
	case crossplatform::DEPTH_NEVER:			return sce::Gnm::kCompareFuncNever    ;       
	case crossplatform::DEPTH_LESS:				return sce::Gnm::kCompareFuncLess    ;        
	case crossplatform::DEPTH_EQUAL:			return sce::Gnm::kCompareFuncEqual;    
	case crossplatform::DEPTH_LESS_EQUAL:		return sce::Gnm::kCompareFuncLessEqual ;   
	case crossplatform::DEPTH_GREATER:			return sce::Gnm::kCompareFuncGreater;   
	case crossplatform::DEPTH_NOT_EQUAL:		return sce::Gnm::kCompareFuncNotEqual ;    
	case crossplatform::DEPTH_GREATER_EQUAL:	return sce::Gnm::kCompareFuncGreaterEqual ;       
	default:
		return sce::Gnm::kCompareFuncAlways;
	}
}

sce::Gnm::BlendFunc RenderPlatform::ToGnmBlendFunction(crossplatform::BlendOperation b)
{
	switch(b)
	{
	case crossplatform::BLEND_OP_NONE:
		return sce::Gnm::kBlendFuncAdd;
	case crossplatform::BLEND_OP_ADD:
		return sce::Gnm::kBlendFuncAdd;
	case crossplatform::BLEND_OP_SUBTRACT:
		return sce::Gnm::kBlendFuncSubtract;
	case crossplatform::BLEND_OP_MAX:
		return sce::Gnm::kBlendFuncMax;
	case crossplatform::BLEND_OP_MIN:
		return sce::Gnm::kBlendFuncMin;
	default:
		return sce::Gnm::BlendFunc::kBlendFuncAdd;
	};
}

sce::Gnm::BlendMultiplier RenderPlatform::ToGnmBlendMultiplier(crossplatform::BlendOption o)
{
	switch(o)
	{
	case crossplatform::BLEND_ZERO:
		return sce::Gnm::kBlendMultiplierZero;
	case crossplatform::BLEND_ONE:
		return sce::Gnm::kBlendMultiplierOne;
	case crossplatform::BLEND_SRC_COLOR:
		return sce::Gnm::kBlendMultiplierSrcColor;
	case crossplatform::BLEND_INV_SRC_COLOR:
		return sce::Gnm::kBlendMultiplierOneMinusSrcColor;
	case crossplatform::BLEND_SRC_ALPHA:
		return sce::Gnm::kBlendMultiplierSrcAlpha;
	case crossplatform::BLEND_INV_SRC_ALPHA:
		return sce::Gnm::kBlendMultiplierOneMinusSrcAlpha;
	case crossplatform::BLEND_DEST_ALPHA:
		return sce::Gnm::kBlendMultiplierDestAlpha;
	case crossplatform::BLEND_INV_DEST_ALPHA:
		return sce::Gnm::kBlendMultiplierOneMinusDestAlpha; 
	case crossplatform::BLEND_DEST_COLOR:
		return sce::Gnm::kBlendMultiplierDestColor;
	case crossplatform::BLEND_INV_DEST_COLOR:
		return sce::Gnm::kBlendMultiplierOneMinusDestColor;
	case crossplatform::BLEND_SRC_ALPHA_SAT:
		return sce::Gnm::kBlendMultiplierSrcAlphaSaturate;
	case crossplatform::BLEND_BLEND_FACTOR:
		return sce::Gnm::kBlendMultiplierConstantColor;
	case crossplatform::BLEND_INV_BLEND_FACTOR:
		return sce::Gnm::kBlendMultiplierOneMinusConstantColor;
	case crossplatform::BLEND_SRC1_COLOR:
		return sce::Gnm::kBlendMultiplierSrc1Color;
	case crossplatform::BLEND_INV_SRC1_COLOR:
		return sce::Gnm::kBlendMultiplierInverseSrc1Color;
	case crossplatform::BLEND_SRC1_ALPHA:
		return sce::Gnm::kBlendMultiplierSrc1Alpha;
	case crossplatform::BLEND_INV_SRC1_ALPHA:
		return sce::Gnm::kBlendMultiplierInverseSrc1Alpha;
	default:
		return sce::Gnm::kBlendMultiplierOne;
	//	return sce::Gnm::kBlendMultiplierConstantAlpha;
	//	return sce::Gnm::kBlendMultiplierOneMinusConstantAlpha; 
	};
}

sce::Gnm::ScanModeControlViewportScissor RenderPlatform::ToGnm(crossplatform::ViewportScissor o)
{
	switch(o)
	{
	case crossplatform::ViewportScissor::VIEWPORT_SCISSOR_DISABLE:
		return sce::Gnm::kScanModeControlViewportScissorDisable;
	case crossplatform::ViewportScissor::VIEWPORT_SCISSOR_ENABLE:
		return sce::Gnm::kScanModeControlViewportScissorEnable;
	};
	SIMUL_CERR_ONCE<<"Unknown enum "<<int(o)<<std::endl;
}

sce::Gnm::PrimitiveSetupCullFaceMode RenderPlatform::ToGnm(crossplatform::CullFaceMode c)
{
	switch(c)
	{
	case crossplatform::CULL_FACE_BACK:
		return sce::Gnm::kPrimitiveSetupCullFaceBack;
	case crossplatform::CULL_FACE_FRONT:
		return sce::Gnm::kPrimitiveSetupCullFaceFront;
	case crossplatform::CULL_FACE_FRONTANDBACK:
		return sce::Gnm::kPrimitiveSetupCullFaceFrontAndBack;
	case crossplatform::CULL_FACE_NONE:
		return sce::Gnm::kPrimitiveSetupCullFaceNone;
	};
	SIMUL_CERR_ONCE<<"Unknown enum "<<int(c)<<std::endl;
}

sce::Gnm::PrimitiveSetupFrontFace RenderPlatform::ToGnm(crossplatform::FrontFace f)
{
	switch(f)
	{
	case crossplatform::FRONTFACE_CLOCKWISE:
		return sce::Gnm::kPrimitiveSetupFrontFaceCw;
	case crossplatform::FRONTFACE_COUNTERCLOCKWISE:
		return sce::Gnm::kPrimitiveSetupFrontFaceCcw;
	};
	SIMUL_CERR_ONCE<<"Unknown enum "<<int(f)<<std::endl;
}

sce::Gnm::PrimitiveSetupPolygonMode RenderPlatform::ToGnm(crossplatform::PolygonMode p)
{
	switch(p)
	{
	case crossplatform::POLYGON_MODE_FILL:
		return sce::Gnm::kPrimitiveSetupPolygonModeFill;
	case crossplatform::POLYGON_MODE_LINE:
		return sce::Gnm::kPrimitiveSetupPolygonModeLine;
	case crossplatform::POLYGON_MODE_POINT:
		return sce::Gnm::kPrimitiveSetupPolygonModePoint;
	};
	SIMUL_CERR_ONCE<<"Unknown enum "<<int(p)<<std::endl;
}

sce::Gnm::PrimitiveSetupPolygonOffsetMode RenderPlatform::ToGnm(crossplatform::PolygonOffsetMode p)
{
	switch(p)
	{
	case crossplatform::POLYGON_OFFSET_ENABLE:
		return sce::Gnm::kPrimitiveSetupPolygonOffsetEnable;
	case crossplatform::POLYGON_OFFSET_DISABLE:
		return sce::Gnm::kPrimitiveSetupPolygonOffsetDisable;
	};
	SIMUL_CERR_ONCE<<"Unknown enum "<<int(p)<<std::endl;
}

crossplatform::RenderState *RenderPlatform::CreateRenderState(const crossplatform::RenderStateDesc &desc)
{
	orbis::RenderState *r=new orbis::RenderState();
	r->type=desc.type;
	if(desc.type==crossplatform::BLEND)
	{
		r->num=desc.blend.numRTs;
		for(int i=0;i<desc.blend.numRTs;i++)
		{
			sce::Gnm::BlendControl &bc=r->blendControls[i];
			const crossplatform::RTBlendDesc &t=desc.blend.RenderTarget[i];
			r->renderTargetWriteMask=desc.blend.RenderTarget[i].RenderTargetWriteMask;
			bc.init();
			bool blendEnable=(t.blendOperation!=crossplatform::BLEND_OP_NONE)||(t.blendOperationAlpha!=crossplatform::BLEND_OP_NONE);
			bc.setBlendEnable(blendEnable);
			//if(desc.blend.RenderTarget[i].BlendEnable)
			{
				bc.setSeparateAlphaEnable(blendEnable);
				//bc.setBlendEnable(true);
				bc.setColorEquation(ToGnmBlendMultiplier(t.SrcBlend)
					,ToGnmBlendFunction(t.blendOperation)
					,ToGnmBlendMultiplier(t.DestBlend));
				bc.setAlphaEquation(ToGnmBlendMultiplier(t.SrcBlendAlpha)
					,ToGnmBlendFunction(t.blendOperationAlpha)
					,ToGnmBlendMultiplier(t.DestBlendAlpha));
				
				//bc.RenderTargetWriteMask	=desc.blend.RenderTarget[i].RenderTargetWriteMask;
			}
		}
		//omDesc.IndependentBlendEnable			=desc.blend.IndependentBlendEnable;
		//omDesc.AlphaToCoverageEnable			=desc.blend.AlphaToCoverageEnable;
	}
	else if(desc.type==crossplatform::DEPTH)
	{
		r->num=1;
		sce::Gnm::DepthStencilControl &ds=r->depthStencilControls[0];
		ds.init();
		ds.setDepthBoundsEnable(false);
		ds.setDepthControl(desc.depth.write?sce::Gnm::DepthControlZWrite::kDepthControlZWriteEnable
			:sce::Gnm::DepthControlZWrite::kDepthControlZWriteDisable,ToGnmDepthComparison(desc.depth.comparison));
		ds.setDepthEnable(desc.depth.test);
		ds.setSeparateStencilEnable(false);
		ds.setStencilEnable(false);
	}
	else if(desc.type==crossplatform::RASTERIZER)
	{
		sce::Gnm::PrimitiveSetup &prim=r->primitiveSetup;
		prim.init();
		prim.setCullFace(ToGnm(desc.rasterizer.cullFaceMode));
		prim.setFrontFace(ToGnm(desc.rasterizer.frontFace));
		prim.setPolygonMode(ToGnm(desc.rasterizer.polygonMode),ToGnm(desc.rasterizer.polygonMode));
	}
	return r;
}

crossplatform::Query *RenderPlatform::CreateQuery(crossplatform::QueryType q)
{
	orbis::Query *Q=new orbis::Query(q);
	return Q;
}

crossplatform::Shader *RenderPlatform::CreateShader()
{
	orbis::Shader *S = new orbis::Shader();
	return S;
}

void *RenderPlatform::GetDevice()
{
	return device;
}

void RenderPlatform::SetVertexBuffers(crossplatform::DeviceContext &deviceContext, int slot, int num_buffers, crossplatform::Buffer **buffers, const crossplatform::Layout *layout, const int *vertexSteps)
{
	if (!buffers)
		return;
	orbis::RenderPlatform *r=(orbis::RenderPlatform *)deviceContext.renderPlatform;
	crossplatform::ContextState *cs=r->GetContextState(deviceContext);
	for(int i=0;i<num_buffers;i++)
	{
		cs->applyVertexBuffers[slot+i]=(buffers[i]);
	}
}

void RenderPlatform::SetStreamOutTarget(crossplatform::DeviceContext &deviceContext, crossplatform::Buffer *buffer, int start_index)
{
	orbis::RenderPlatform *r=(orbis::RenderPlatform *)deviceContext.renderPlatform;
	crossplatform::ContextState *cs=r->GetContextState(deviceContext);
	if(buffer)
	{
		cs->streamoutTargets[start_index]=buffer;
		cs->streamoutTargetsValid=false;
	}
	else
	{
		cs->streamoutTargets.erase(start_index);
	}
}

void RenderPlatform::ActivateRenderTargets(crossplatform::DeviceContext &deviceContext,int num,crossplatform::Texture **targs,crossplatform::Texture *depth)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	crossplatform::TargetsAndViewport *targetAndViewport=new crossplatform::TargetsAndViewport;
	targetAndViewport->temp=true;
	targetAndViewport->num=num;
	unsigned msk=0;
	for(int i=0;i<num;i++)
	{
		if(targs[i]&&targs[i]->IsValid())
		{
			auto rt=((orbis::Texture*)targs[i])->AsGnmRenderTarget(i,0);
			gfxc->setRenderTarget(i,rt);
			targetAndViewport->m_rt[i]=rt;
		}
		unsigned m=(0xF)<<(i*4);
		msk|=m;
	}
	targetAndViewport->m_dt=(depth&&depth->IsValid())?((orbis::Texture*)depth)->AsGnmDepthRenderTarget():NULL;
	targetAndViewport->viewport.x=targetAndViewport->viewport.y=0;
	targetAndViewport->viewport.w=targs[0]->width;
	targetAndViewport->viewport.h=targs[0]->length;
	targetAndViewport->viewport.zfar=1.0f;
	targetAndViewport->viewport.znear=0.0f;

	gfxc->setRenderTargetMask(msk);
	if(targetAndViewport->m_dt)
	{
		const sce::Gnm::DepthRenderTarget *dt=static_cast<const sce::Gnm::DepthRenderTarget*>(targetAndViewport->m_dt);
		gfxc->setDepthRenderTarget(dt);
	}
	else
		Utilities::DisableDepth(gfxc);
	if(num)
		gfxc->setupScreenViewport(0,0
								,targs[0]->width
								,targs[0]->length
								,1.0f,0.0f);
	orbis::Framebuffer::GetFrameBufferStack().push(targetAndViewport);
}

void RenderPlatform::DeactivateRenderTargets(crossplatform::DeviceContext &deviceContext)
{
	std::stack<crossplatform::TargetsAndViewport*> &fbs=Framebuffer::GetFrameBufferStack();
	crossplatform::TargetsAndViewport *oldtv=fbs.top();
	fbs.pop();
	if(oldtv->temp)
		delete oldtv;
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	crossplatform::TargetsAndViewport *tv=NULL;
	if(fbs.size())
	{
		tv=(fbs.top());
	}
	else
	{
		tv=&Framebuffer::defaultTargetsAndViewport;
	}
	const sce::Gnm::DepthRenderTarget *dt=static_cast<const sce::Gnm::DepthRenderTarget*>(tv->m_dt);
	const sce::Gnm::RenderTarget *rt0=static_cast<const sce::Gnm::RenderTarget*>(tv->m_rt[0]);
	if(dt&&!dt->getHeight())
	{
		SIMUL_BREAK_ONCE("Bad depth target");
	}
	if(rt0&&!rt0->getHeight())
	{
		SIMUL_BREAK_ONCE("Bad render target");
	}
	unsigned msk=0;
	for(int i=0;i<tv->num;i++)
	{
		const sce::Gnm::RenderTarget *rt=static_cast<const sce::Gnm::RenderTarget*>(tv->m_rt[i]);
		gfxc->setRenderTarget(i,rt);
		
		gfxc->setViewportScissor(i
				,tv->viewport.x
				,tv->viewport.y
				,tv->viewport.x+tv->viewport.w
				,tv->viewport.y+tv->viewport.h
				,sce::Gnm::kWindowOffsetDisable);

		unsigned m=(0xF)<<(i*4);
		msk|=m;
	}
	gfxc->setRenderTargetMask(msk);
	gfxc->setDepthRenderTarget(dt);
	if(!tv->m_dt)
		Utilities::DisableDepth(gfxc);

	for(int i=tv->num;i<8;i++)
		gfxc->setRenderTarget(i,NULL);
	if(tv->viewport.w>0)
	{
		gfxc->setupScreenViewport(tv->viewport.x,
							tv->viewport.y,
							tv->viewport.x+tv->viewport.w,
							tv->viewport.y+tv->viewport.h
							,1.0f,0.0f);
		gfxc->setWindowOffset(
					0,
				   0
				);
		gfxc->setGenericScissor(0, 0, tv->viewport.x+tv->viewport.w, tv->viewport.y+tv->viewport.h,sce::Gnm::kWindowOffsetDisable);
		gfxc->setScreenScissor(0, 0, tv->viewport.x+tv->viewport.w, tv->viewport.y+tv->viewport.h);
		gfxc->setWindowScissor(0, 0, tv->viewport.x+tv->viewport.w, tv->viewport.y+tv->viewport.h, sce::Gnm::kWindowOffsetDisable);
		gfxc->setViewportScissor(
							0,
							tv->viewport.x,
							tv->viewport.y,
							tv->viewport.x+tv->viewport.w,
							tv->viewport.y+tv->viewport.h, sce::Gnm::kWindowOffsetDisable
		);
	}
}

void RenderPlatform::SetViewports(crossplatform::DeviceContext &deviceContext,int num,const crossplatform::Viewport *vps)
{
	if(!vps)
		return;
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	const vec3 scale( (vps->w)*0.5f,-(vps->h)*0.5f, 0.5f );
	const vec3 offset( vps->x + (vps->w)*0.5f, vps->y + (vps->h)*0.5f, 0.5f );
	gfxc->setViewport(0,0.0f,1.0f,scale,offset);
	if(Framebuffer::GetFrameBufferStack().size())
	{
		crossplatform::TargetsAndViewport *f=Framebuffer::GetFrameBufferStack().top();
		if(f)
			f->viewport=*vps;
	}
	else
	{
		Framebuffer::defaultTargetsAndViewport.viewport=*vps;
	}
}

void RenderPlatform::SetIndexBuffer(crossplatform::DeviceContext &deviceContext,crossplatform::Buffer *buffer)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	orbis::Buffer *orbisBuffer=(orbis::Buffer*)(buffer);
	gfxc->setIndexBuffer(orbisBuffer->GetVideoMemory());
	gfxc->setIndexCount(orbisBuffer->count);
	gfxc->setIndexSize(orbisBuffer->stride==2?sce::Gnm::kIndexSize16:sce::Gnm::kIndexSize32);
}

void RenderPlatform::SetTopology(crossplatform::DeviceContext &deviceContext,crossplatform::Topology t)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	gfxc->setPrimitiveType(orbis::RenderPlatform::ToGnmTopology(t));
}

void RenderPlatform::StoreRenderState(crossplatform::DeviceContext &deviceContext)
{
}

void RenderPlatform::RestoreRenderState(crossplatform::DeviceContext &deviceContext)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	if (gfxc)
	{
		//gfxc->setGsMode(kGsModeEnable);
		//gfxc->setActiveShaderStages(sce::Gnm::kActiveShaderStagesVsPs);
	}
}

void RenderPlatform::PopRenderTargets(crossplatform::DeviceContext &deviceContext)
{
}

void RenderPlatform::SetRenderState(crossplatform::DeviceContext &deviceContext,const crossplatform::RenderState *s)
{
	if(s->type==crossplatform::BLEND)
	{
		orbis::RenderState *rs=(orbis::RenderState*)s;
		for(int i=0;i<rs->num;i++)
		{
			deviceContext.asGfxContext()->setBlendControl(i, rs->blendControls[i]);
			deviceContext.asGfxContext()->setRenderTargetMask(rs->renderTargetWriteMask);
		}
	}
	if(s->type==crossplatform::DEPTH)
	{
		orbis::RenderState *ds=(orbis::RenderState*)s;
		deviceContext.asGfxContext()->setDepthStencilControl(ds->depthStencilControls[0]);
	}
	if(s->type==crossplatform::RASTERIZER)
	{
		orbis::RenderState *rs=(orbis::RenderState*)s;
		deviceContext.asGfxContext()->setPrimitiveSetup(rs->primitiveSetup);
	}
}

void RenderPlatform::Resolve(crossplatform::DeviceContext &deviceContext,crossplatform::Texture *destination,crossplatform::Texture *source)
{
}

void RenderPlatform::SaveTexture(crossplatform::Texture *texture,const char *lFileNameUtf8)
{
}
/*
void RenderPlatform::DrawQuad(crossplatform::DeviceContext &deviceContext,int x1,int y1,int dx,int dy,crossplatform::Effect *effect
	,crossplatform::EffectTechnique *technique,const char *pass)
{
	if(!effect||!technique)
		return;
	
	crossplatform::Viewport viewport=GetViewport(deviceContext,0);
	
	effect->Apply(deviceContext,technique,pass);
	
	vec4 r(2.f*(float)x1/(float)viewport.w-1.f
		,1.f-2.f*(float)(y1+dy)/(float)viewport.h
		,2.f*(float)dx/(float)viewport.w
		,2.f*(float)dy/(float)viewport.h);
	
	debugConstants.LinkToEffect(effect,"DebugConstants");
	debugConstants.rect=r;
	effect->SetConstantBuffer(deviceContext,&debugConstants);
	DrawQuad(deviceContext);
	//effect->UnbindTextures(deviceContext);
	effect->Unapply(deviceContext);
}
*/
void RenderPlatform::DrawDepth(crossplatform::DeviceContext &deviceContext,int x1,int y1,int dx,int dy,crossplatform::Texture *tex,const crossplatform::Viewport *v)
{
}
sce::Gnm::ResourceHandle RenderPlatform:: GetResourceHandle()
{
	return resourceHandle;
}
void RenderPlatform::ClearFencedTextures()
{
	for(auto i:fencedTextures)
	{
		i->SetFence(0);
	}
	fencedTextures.clear();
}
void RenderPlatform::DrawQuad(crossplatform::DeviceContext &deviceContext)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	ApplyContextState(deviceContext);
	gfxc->setPrimitiveType(sce::Gnm::kPrimitiveTypeTriStrip);// was kPrimitiveTypeRectList but that only does SCREEN-ALIGNED things.
	SIMUL_PS4_VALIDATE3(deviceContext)
	gfxc->drawIndexAuto(4);
	//Exception in gfx draw call? This can come from failing to set a resource input.
 	SIMUL_PS4_VALIDATE3(deviceContext)
}

void RenderPlatform::DrawLines(crossplatform::DeviceContext &deviceContext, crossplatform::PosColourVertex *lines, int vertex_count, bool strip, bool test_depth, bool centred)
{
}

void RenderPlatform::Draw2dLines(crossplatform::DeviceContext &deviceContext, crossplatform::PosColourVertex *lines, int vertex_count, bool strip)
{
}

void RenderPlatform::DrawCircle(crossplatform::DeviceContext &deviceContext,const float *dir,float rads,const float *colr,bool fill)
{
}

void RenderPlatform::DrawCube		(crossplatform::DeviceContext &deviceContext)
{
}

	//renderPlatform->PrintAt3dPos((ID3D11DeviceContext *)context,p,text,colr,offsetx,offsety);
/*
void *RenderPlatform::AllocateVideoMemory(int size,int align);
void RenderPlatform::DeallocateVideoMemory(void *ptr);*/
void RenderPlatform::EnsureEffectIsBuilt(const char *filename_utf8,const std::vector<crossplatform::EffectDefineOptions> &options)
{
}


sce::Gnm::Texture *RenderPlatform::GetDummyTexture(int dims)
{
	if(dims==2)
		return &dummytexture2d;
	if(dims==3)
		return &dummytexture3d;
	return NULL;
};

crossplatform::PixelOutputFormat RenderPlatform::GetCurrentPixelOutputFormat(crossplatform::DeviceContext &deviceContext)
{
	const crossplatform::TargetsAndViewport* pTargetAndVP;
	if(orbis::Framebuffer::GetFrameBufferStack().size())
	{
		pTargetAndVP = orbis::Framebuffer::GetFrameBufferStack().top();
		const sce::Gnm::RenderTarget *rt0=static_cast<const sce::Gnm::RenderTarget*>(pTargetAndVP->m_rt[0]);
		if(rt0)
		{
			sce::Gnm::DataFormat gnmFormat=rt0->getDataFormat();
			crossplatform::PixelOutputFormat pixelOutputFormat=PixelOutputFormatFromGnmFormat(gnmFormat);
			return pixelOutputFormat;
		}
	}
	return crossplatform::PixelOutputFormat::FMT_FP16_ABGR;
}

void RenderPlatform::ClearTexture(crossplatform::DeviceContext &deviceContext,crossplatform::Texture *texture,const vec4& colour)
{
	if(texture)
		ClearGnmTexture(deviceContext,texture->AsGnmTexture(),(const float*)&colour);
}

#include "Simul/Platform/PS4/Render/FrameBuffer.h"
//Uses the currently set up viewport.

void RenderPlatform::ClearGnmTexture(crossplatform::DeviceContext &deviceContext, const sce::Gnm::Texture* texture, const float colour[4])
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	
	gfxc->pushMarker("RenderPlatform::ClearGnmTexture");
	crossplatform::ContextState *cs=GetContextState(deviceContext);
	bool comp=cs->last_action_was_compute;
	cs->last_action_was_compute=true;

	sce::Gnm::Texture textureCopy = *texture;
	sce::Gnm::TextureType t=texture->getTextureType();
	crossplatform::EffectTechnique *tech=NULL;
	switch(t)
	{
	case sce::Gnm::kTextureType1d:
	case sce::Gnm::kTextureType1dArray:
		//gfxc->setCsShader(s_clearTexture1d.m_shader);
		break;
	case sce::Gnm::kTextureTypeCubemap:
		// Spoof the cubemap textures as 2D texture arrays.
		//textureCopy.initAs2dArray(textureCopy.getWidth(), textureCopy.getHeight(), textureCopy.getLastArraySliceIndex()+1, textureCopy.getLastMipLevel()+1, textureCopy.getDataFormat(), textureCopy.getTileMode(), textureCopy.getNumSamples(), false);
		//textureCopy.setBaseAddress(texture->getBaseAddress());
		// Intentional fall-through
	case sce::Gnm::kTextureType2d:
	case sce::Gnm::kTextureType2dArray:
		tech=deviceContext.renderPlatform->GetDebugEffect()->GetTechniqueByName("compute_clear");
		//gfxc->setCsShader(Utilities::clearShader);
		break;
	case sce::Gnm::kTextureType3d:
		tech=deviceContext.renderPlatform->GetDebugEffect()->GetTechniqueByName("compute_clear_3d");
		//gfxc->setCsShader(clearTexture3DShader);
		break;
	default:
		//SCE_GNM_ERROR("textureDst's dimensionality (0x%02X) is not supported by this function.", texture->getTextureDimension());
		return;
	}
	
	debugConstants.debugColour=colour;
	debugEffect->SetConstantBuffer(deviceContext,&debugConstants);
	debugEffect->Apply(deviceContext,tech,0);

	const uint32_t oldMipBase   = textureCopy.getBaseMipLevel();
	const uint32_t oldMipLast   = textureCopy.getLastMipLevel();
	const uint32_t oldSliceBase = textureCopy.getBaseArraySliceIndex();
	const uint32_t oldSliceLast = textureCopy.getLastArraySliceIndex();
	
	((orbis::RenderPlatform*)(deviceContext.renderPlatform))->ApplyContextState(deviceContext,false);
	for(uint32_t iMip=oldMipBase; iMip <= oldMipLast; ++iMip)
	{
			
		textureCopy.setMipLevelRange(iMip, iMip);
		const uint32_t mipWidth  = std::max(textureCopy.getWidth() >> iMip, 1U);
		const uint32_t mipHeight = std::max(textureCopy.getHeight() >> iMip, 1U);
		const uint32_t mipDepth  = std::max(textureCopy.getDepth() >> iMip, 1U);
		for(uint32_t iSlice=oldSliceBase; iSlice <= oldSliceLast; ++iSlice)
		{
			textureCopy.setArrayView(iSlice, iSlice);

			textureCopy.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
			gfxc->setRwTextures( sce::Gnm::kShaderStageCs, 0, 1, &textureCopy );
			gfxc->setRwTextures( sce::Gnm::kShaderStageCs,1, 1, &textureCopy );
			
			debugConstants.texSize=uint4(mipWidth,mipHeight);
			debugEffect->SetConstantBuffer(deviceContext,&debugConstants);
			ApplyContextState(deviceContext);
			SIMUL_PS4_VALIDATE2(deviceContext);
			switch(textureCopy.getTextureType())
			{
			case sce::Gnm::kTextureType1d:
			case sce::Gnm::kTextureType1dArray:
				gfxc->dispatch( (mipWidth+63)/64, 1, 1);
				break;
			case sce::Gnm::kTextureTypeCubemap:
			case sce::Gnm::kTextureType2d:
			case sce::Gnm::kTextureType2dArray:
				gfxc->dispatch( (mipWidth+7)/8, (mipHeight+7)/8, 1);
				break;
			case sce::Gnm::kTextureType3d:
				gfxc->dispatch( (mipWidth+3)/4, (mipHeight+3)/4, (mipDepth+3)/4 );
				break;
			default:
				SCE_GNM_ASSERT(0); // This path should have been caught in the previous switch statement
				return;
			}
		}
	}
	graphicsWaitForCompute(gfxc);
	debugEffect->Unapply(deviceContext);
	cs->last_action_was_compute=comp;
	gfxc->popMarker();
}

void RenderPlatform::WaitForFencedResources(crossplatform::DeviceContext &deviceContext)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	crossplatform::ContextState *contextState=GetContextState(deviceContext);
	for(auto i=contextState->textureAssignmentMap.begin();i!=contextState->textureAssignmentMap.end();i++)
	{
		int slot=i->first;
		const crossplatform::TextureAssignment &ta=i->second;
		if(!ta.texture)
			continue;
		// don't need to wait for a writeable texture. PROBABLY
		if(slot<0||slot>=1000)
			continue;
		unsigned long long fence=ta.texture->GetFence();
		if(fence)
		{
			volatile uint64_t* label = (uint64_t*)fence;
			gfxc->waitOnAddress( const_cast<uint64_t*>(label), 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1
			gfxc->flushShaderCachesAndWait(sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 0
				//,sce::Gnm::kStallCommandBufferParserDisable); // This is what the Toolkit uses. What the hell does it mean?
				, sce::Gnm::kStallCommandBufferParserEnable ); // tell the CP to flush the L1$ and L2$

			ta.texture->ClearFence();
		}
	}
}


void RenderPlatform::ClearRenderTarget(crossplatform::DeviceContext &deviceContext,const sce::Gnm::RenderTarget* renderTarget,float r,float g,float b,float a)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	synchronizeRenderTargetGraphicsToCompute(gfxc,renderTarget);
	sce::Gnm::Texture texture;
	texture.initFromRenderTarget(renderTarget,false); // TODO: we sure this isn't a cube map?
	texture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
	float rgba[]={r,g,b,a};
	ClearGnmTexture(deviceContext,&texture,rgba);
	
	if(&deviceContext==&immediateContext)
		gfxc->submit();
	SIMUL_PS4_VALIDATE
}
static void clearDepthStencil(crossplatform::DeviceContext &deviceContext, const sce::Gnm::DepthRenderTarget *depthTarget)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	gfxc->setRenderTargetMask(0x0);
	gfxc->setDepthRenderTarget(depthTarget);
	gfxc->setupScreenViewport(0,
							0,
							depthTarget->getWidth(),
							depthTarget->getHeight(),
							1.0f,
							0.0f
							);
	float *constantBuffer = static_cast<float*>(gfxc->allocateFromCommandBuffer(4*sizeof(float),sce::Gnm::kEmbeddedDataAlignment4));
	float src[]={0.f, 0.f, 0.f, 0.f};
	memcpy(constantBuffer,src,4*sizeof(float));
	crossplatform::Effect *e=deviceContext.renderPlatform->GetDebugEffect();
	//sce::Gnm::Buffer buffer;
	//buffer.initAsConstantBuffer(constantBuffer, 4*sizeof(float));
	//gfxc->setConstantBuffers(sce::Gnm::kShaderStagePs, 0, 1, &buffer);
	auto &debugConstants=deviceContext.renderPlatform->GetDebugConstantBuffer();
	debugConstants.debugColour=vec4(0,0,0,0);
	e->SetConstantBuffer(deviceContext,&debugConstants);
	e->Apply(deviceContext,e->GetTechniqueByName("clear_depth"),0);
	
	deviceContext.renderPlatform->DrawQuad(deviceContext);
	
	e->Unapply(deviceContext);
	
	gfxc->setRenderTargetMask(0xF);

	sce::Gnm::DbRenderControl dbRenderControl;
	dbRenderControl.init();
	gfxc->setDbRenderControl(dbRenderControl);

	if(Framebuffer::GetFrameBufferStack().size())
	{
		crossplatform::TargetsAndViewport *tv=Framebuffer::GetFrameBufferStack().top();
		crossplatform::Viewport viewport=tv->viewport;
		const sce::Gnm::DepthRenderTarget *dt=static_cast<const sce::Gnm::DepthRenderTarget*>(tv->m_dt);
		const sce::Gnm::RenderTarget *rt0=static_cast<const sce::Gnm::RenderTarget*>(tv->m_rt[0]);
		gfxc->setDepthRenderTarget(dt);
		gfxc->setupScreenViewport(viewport.x,
							viewport.y,
							viewport.w,
							viewport.h,
							1.0f,
							0.0f
							);
		if(!dt)
			Utilities::DisableDepth(gfxc);
	}
	else
	{
		gfxc->setDepthRenderTarget(NULL);
		Utilities::DisableDepth(gfxc);
	}
	
	
}

void RenderPlatform::ClearDepthStencil(crossplatform::DeviceContext &deviceContext, const sce::Gnm::DepthRenderTarget *depthTarget,float d)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	
	sce::Gnm::DbRenderControl dbRenderControl;
	dbRenderControl.init();
	dbRenderControl.setDepthClearEnable(true);
	gfxc->setDbRenderControl(dbRenderControl);
	

	sce::Gnm::DepthStencilControl depthControl;
	depthControl.init();
	depthControl.setDepthControl(sce::Gnm::kDepthControlZWriteEnable,sce::Gnm::kCompareFuncAlways);
	depthControl.setStencilFunction(sce::Gnm::kCompareFuncNever);
	depthControl.setDepthEnable(true);
	gfxc->setDepthStencilControl(depthControl);

	gfxc->setDepthClearValue(d);
	
	clearDepthStencil(deviceContext, depthTarget);
	
	if(&deviceContext==&immediateContext)
		gfxc->submit();
}