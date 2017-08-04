#include "Framebuffer.h"
#include "Simul/Base/RuntimeError.h"
#include "Pssl/CppPssl.h"
#include "PSSL/copy_constant_buffer.sl"
#include "Utilities.h"
#include "RenderPlatform.h"
#include "gnm/commandbuffer.h"
#include "Simul/Platform/CrossPlatform/Macros.h"

#define  SCRATCH_SIZE 4096

using namespace simul;
using namespace orbis;

static const sce::Gnm::DataFormat kInvalidDataFormat = sce::Gnm::DataFormat::build(sce::Gnm::kZFormatInvalid);


Framebuffer::Framebuffer(const char *n)
	:BaseFramebuffer(n)
	,makeLinear(false)
	,baseAddr(nullptr)
	,depth_active(false)
{
}

Framebuffer::~Framebuffer()
{
	InvalidateDeviceObjects();
}

void Framebuffer::SetLinear(bool l)
{
	makeLinear=l;
}

void Framebuffer::SetWidthAndHeight(int w,int h,int m)
{
	if(Width!=w||Height!=h||mips!=m)
	{
		Width=w;
		Height=h;
		mips=m;
		InvalidateDeviceObjects();
	}
}

void Framebuffer::SetAsCubemap(int w,int num_mips,crossplatform::PixelFormat f)
{
	SetWidthAndHeight(w,w,num_mips);
	SetFormat(f);
	is_cubemap=true;
}


void Framebuffer::SetFormat(crossplatform::PixelFormat f)
{
	if(target_format==f)
		return;
	target_format=f;
	InvalidateDeviceObjects();
}

void Framebuffer::SetDepthFormat(crossplatform::PixelFormat f)
{
	if((int)depth_format==f)
		return;
	depth_format=f;
	InvalidateDeviceObjects();
}

void Framebuffer::Sync(crossplatform::DeviceContext &deviceContext,bool stall_cb) const
{
	if(buffer_texture)
		((orbis::Texture*)buffer_texture)->SyncRenderTarget(deviceContext,stall_cb);
}

void Framebuffer::SyncToCompute(crossplatform::DeviceContext &deviceContext)
{
	if(buffer_texture)
		((orbis::Texture*)buffer_texture)->SyncRenderTargetToCompute(deviceContext);
}

bool Framebuffer::IsValid() const
{
	return ((buffer_texture!=NULL&&buffer_texture->IsValid())||(buffer_depth_texture!=NULL&&buffer_depth_texture->IsValid()));
}

void Framebuffer::RestoreDeviceObjects(crossplatform::RenderPlatform *r)
{
	renderPlatform=r;
	if(!external_texture)
		SAFE_DELETE(buffer_texture);
	if(!external_depth_texture)
		SAFE_DELETE(buffer_depth_texture);
	static int seed = 0;
	seed = seed % 1001;
	shSeed=seed++;
	if(renderPlatform)
	{
		if(!external_texture)
			buffer_texture=renderPlatform->CreateTexture("BaseFramebuffer");
		if(!external_depth_texture)
			buffer_depth_texture=renderPlatform->CreateTexture("BaseFramebuffer");
	}

	int s=(bands+1);
	if(s<4)
		s=4;
	sphericalHarmonics.InvalidateDeviceObjects();
	sphericalSamples.InvalidateDeviceObjects();
	sphericalHarmonicsConstants.RestoreDeviceObjects(renderPlatform);
	sphericalHarmonicsConstants.LinkToEffect(sphericalHarmonicsEffect,"SphericalHarmonicsConstants");
	CreateBuffers();
}

void Framebuffer::ActivateDepth(crossplatform::DeviceContext &deviceContext)
{
}

void Framebuffer::SetAntialiasing(int s)
{
}

bool Framebuffer::CreateBuffers()
{
	if(!Width||!Height)
		return false;
	if(!renderPlatform)
	{
		SIMUL_BREAK("renderPlatform should not be NULL here");
	}
	if(!renderPlatform)
		return false;
	if((buffer_texture&&buffer_texture->IsValid()))
		return true;
	if(buffer_depth_texture&&buffer_depth_texture->IsValid())
		return true;
	if(buffer_texture)
		buffer_texture->InvalidateDeviceObjects();
	if(buffer_depth_texture)
		buffer_depth_texture->InvalidateDeviceObjects();
	if(!buffer_texture)
		buffer_texture=renderPlatform->CreateTexture("BaseFramebuffer");
	if(!buffer_depth_texture)
		buffer_depth_texture=renderPlatform->CreateTexture("BaseFramebuffer");
	static int quality=0;
	if(!external_texture&&target_format!=crossplatform::UNKNOWN)
	{
		if(!is_cubemap)
			buffer_texture->ensureTexture2DSizeAndFormat(renderPlatform,Width,Height,target_format,false,true,false,numAntialiasingSamples,quality);
		else
		{
			buffer_texture->ensureTextureArraySizeAndFormat(renderPlatform,Width,Height,1,mips,target_format,false,true,true);
		}
	}
	if(!external_depth_texture&&depth_format!=crossplatform::UNKNOWN)
	{
		buffer_depth_texture->ensureTexture2DSizeAndFormat(renderPlatform,Width,Height,depth_format,false,false,true,numAntialiasingSamples,quality);
	}
	if(!Width||!Height)
		return false;
	targetsAndViewport.num=1;
	targetsAndViewport.m_rt[0]=buffer_texture!=nullptr?((orbis::Texture*)buffer_texture)->AsGnmRenderTarget(0,0):nullptr;
	if(buffer_depth_texture!=nullptr&&depth_format!=simul::crossplatform::PixelFormat::UNKNOWN)
	{
		///buffer_depth_texture=renderPlatform->CreateTexture(name.c_str());
		//buffer_depth_texture->ensureTexture2DSizeAndFormat(renderPlatform,Width,Height,depth_format,false,false,true,numAntialiasingSamples,quality);
		targetsAndViewport.m_dt=((orbis::Texture*)buffer_depth_texture)->AsGnmDepthRenderTarget();
	}
	else
		targetsAndViewport.m_dt=NULL;
	targetsAndViewport.viewport.x=0;
	targetsAndViewport.viewport.y=0;
	targetsAndViewport.viewport.w=Width;
	targetsAndViewport.viewport.h=Height;
	return true;
}

void Framebuffer::InvalidateDeviceObjects()
{
	SAFE_DELETE(buffer_texture)
	SAFE_DELETE(buffer_depth_texture)
	baseAddr=nullptr;
}

void Framebuffer::Activate(crossplatform::DeviceContext &deviceContext)
{
	ActivateViewport(deviceContext
							,0.f
							,0.f
							,1.f
							,1.f);
}

static inline void relativeToAbsoluteViewport( unsigned int targetW, unsigned int targetH,
											float relativeViewportX,
											float relativeViewportY,
											float relativeViewportW,
											float relativeViewportH,
											crossplatform::Viewport& viewport
											)
{
	const float fW = (float)targetW;
	const float fH = (float)targetH;
	viewport.x = (int)( relativeViewportX*fW + 0.5f );
	viewport.y = (int)( relativeViewportY*fH + 0.5f );
	viewport.w =  (uint32_t)(relativeViewportW*fW + 0.5f);
	viewport.h =  (uint32_t)(relativeViewportH*fH + 0.5f);
}

void Framebuffer::SetExternalTextures(crossplatform::Texture *colour,crossplatform::Texture *depth)
{
	BaseFramebuffer::SetExternalTextures(colour,depth);
	orbis::Texture *c=(orbis::Texture *)colour;
	orbis::Texture *d=(orbis::Texture *)depth;
	targetsAndViewport.m_rt[0]=c?c->AsGnmRenderTarget(0,0):nullptr;
	targetsAndViewport.num=1;
	targetsAndViewport.m_dt=d?d->AsGnmDepthRenderTarget():nullptr;
}

void Framebuffer::ActivateViewport(crossplatform::DeviceContext &deviceContext,float x,float y,float w,float h)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	if(!(buffer_texture&&buffer_texture->IsValid())&&!(buffer_depth_texture&&buffer_depth_texture->IsValid()))
		RestoreDeviceObjects(renderPlatform);
	if(!(buffer_texture&&buffer_texture->IsValid())&&!(buffer_depth_texture&&buffer_depth_texture->IsValid()))
		return;
/*	gfxc->waitForGraphicsWrites(renderTarget.getBaseAddress256ByteBlocks(), renderTarget.getSizeInBytes()>>8,
		sce::Gnm::kWaitTargetSlotCb0,sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2
		,sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache
		, sce::Gnm::kStallCommandBufferParserEnable);*/
	sce::Gnm::ZFormat gnmdepthz_format=RenderPlatform::ToGnmDepthFormat(depth_format);
	sce::Gnm::DataFormat gnmdepth_format=RenderPlatform::ToGnmFormat(depth_format);
	if(buffer_texture&&buffer_texture->IsValid())
	{
		auto rt=((orbis::Texture*)buffer_texture)->AsGnmRenderTarget(is_cubemap?current_face:0,0);
		gfxc->setRenderTarget(0,rt);
		targetsAndViewport.m_rt[0]=rt;
		colour_active=true;
	}
	else
	{
		gfxc->setRenderTarget(0,NULL);
		colour_active=false;
	}
	if(buffer_depth_texture&&buffer_depth_texture->IsValid())
	{
		sce::Gnm::DepthRenderTarget *dt=((orbis::Texture*)buffer_depth_texture)->AsGnmDepthRenderTarget();
		gfxc->setDepthRenderTarget(dt);
		if(!dt)
			Utilities::DisableDepth(gfxc);
		depth_active=true;
	}
	else
	{
		gfxc->setDepthRenderTarget(NULL);
		Utilities::DisableDepth(gfxc);
		depth_active=false;
	}
	
	relativeToAbsoluteViewport(Width,Height,x,y,w,h,
								targetsAndViewport.viewport
								);

	gfxc->setupScreenViewport(targetsAndViewport.viewport.x,
							targetsAndViewport.viewport.y,
							targetsAndViewport.viewport.x+targetsAndViewport.viewport.w,
							targetsAndViewport.viewport.y+targetsAndViewport.viewport.h,
							1.0f,
							0.0f
							);
	GetFrameBufferStack().push(&targetsAndViewport);
}

void Framebuffer::ActivateColour(crossplatform::DeviceContext &deviceContext)
{
	float viewportXYWH[]={0.f,
							0.f,
							(float)Width,
							(float)Height};
	ActivateColour(deviceContext,viewportXYWH);
}

void Framebuffer::ActivateColour(crossplatform::DeviceContext &deviceContext,const float viewportXYWH[4])
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	if(!baseAddr)
		RestoreDeviceObjects(renderPlatform);
	if(!baseAddr)
		return;
	/*gfxc->waitForGraphicsWrites(renderTarget.getBaseAddress256ByteBlocks(), renderTarget.getSizeInBytes()>>8,
		sce::Gnm::kWaitTargetSlotCb0,sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2
		,sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache
		, sce::Gnm::kStallCommandBufferParserEnable);*/
	gfxc->setRenderTarget(0, ((orbis::Texture*)buffer_texture)->AsGnmRenderTarget(0,0));
	gfxc->setDepthRenderTarget(NULL);
	Utilities::DisableDepth(gfxc);
	depth_active=false;

	relativeToAbsoluteViewport(Width,Height,viewportXYWH[0],viewportXYWH[1],viewportXYWH[2],viewportXYWH[3],
								targetsAndViewport.viewport
								);
	gfxc->setupScreenViewport(targetsAndViewport.viewport.x,
							targetsAndViewport.viewport.y,
							targetsAndViewport.viewport.x+targetsAndViewport.viewport.w,
							targetsAndViewport.viewport.y+targetsAndViewport.viewport.h,
							1.0f,
							0.0f
							);

	GetFrameBufferStack().push(&targetsAndViewport);
}

void Framebuffer::Clear(crossplatform::DeviceContext &deviceContext,float r,float g,float b,float a,float d,int mask)
{
	orbis::RenderPlatform *rp=(orbis::RenderPlatform*)renderPlatform;
	if(!(buffer_texture&&buffer_texture->IsValid())&&!(buffer_depth_texture&&buffer_depth_texture->IsValid()))
		RestoreDeviceObjects(renderPlatform);
	if(!(buffer_texture&&buffer_texture->IsValid())&&!(buffer_depth_texture&&buffer_depth_texture->IsValid()))
	{
		SIMUL_BREAK_ONCE("Nothing valid in framebuffer to clear.");
		return;
	}
	
	if(buffer_texture)
	{
		if(is_cubemap&&current_face>=0)
		{
			//renderPlatform->ClearTexture(deviceContext,buffer_texture,vec4(r,g,b,a));
		}
		else
//	assert(!GetFrameBufferStack().empty()); //We expect to be within an Activate/Deactivate pair so the required viewport is properly set up for our clearing.
		{
			if(buffer_texture&&buffer_texture->IsValid())
				renderPlatform->ClearTexture(deviceContext,buffer_texture,vec4(r,g,b,a));
		}
	}
	if(buffer_depth_texture&&buffer_depth_texture->IsValid())
		rp->ClearDepthStencil(deviceContext, ((orbis::Texture*)buffer_depth_texture)->AsGnmDepthRenderTarget(), d);


}

void Framebuffer::ClearColour(crossplatform::DeviceContext &deviceContext,float r,float g,float b,float a)
{
	orbis::RenderPlatform *rp=(orbis::RenderPlatform*)renderPlatform;
	rp->ClearRenderTarget(deviceContext, ((orbis::Texture*)buffer_texture)->AsGnmRenderTarget(0,0),r,g,b,a);
}

void Framebuffer::Deactivate(crossplatform::DeviceContext &deviceContext)
{	// Sync on the offscreen target
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	auto rt=((orbis::Texture*)buffer_texture)->AsGnmRenderTarget(0,0);
	gfxc->waitForGraphicsWrites(rt->getBaseAddress256ByteBlocks(),rt->getSliceSizeInBytes()>>8,
		sce::Gnm::kWaitTargetSlotCb0
		,sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2
		,sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache
		,sce::Gnm::kStallCommandBufferParserEnable);
	crossplatform::TargetsAndViewport* current=GetFrameBufferStack().top();
	if(current->temp)
	{
		// NOTE: This should never happen.
		SIMUL_CERR<<"Invalid TV"<<std::endl;
		delete current;
	}
	const crossplatform::TargetsAndViewport* pOldTargetAndVP;
	GetFrameBufferStack().pop();
	if(GetFrameBufferStack().size())
	{
		pOldTargetAndVP = GetFrameBufferStack().top();
	}
	else
	{
		pOldTargetAndVP = &defaultTargetsAndViewport;
	}
	for(int i=0;i<pOldTargetAndVP->num;i++)
	{
		const sce::Gnm::RenderTarget *rt=static_cast<const sce::Gnm::RenderTarget *>(pOldTargetAndVP->m_rt[i]);
		gfxc->setRenderTarget(i,rt);
	}
	const sce::Gnm::DepthRenderTarget *dt=static_cast<const sce::Gnm::DepthRenderTarget *>(pOldTargetAndVP->m_dt);
	gfxc->setDepthRenderTarget(dt);
	if(!pOldTargetAndVP->m_dt)
		Utilities::DisableDepth(gfxc);
	depth_active=false;
		colour_active=false;
	gfxc->setupScreenViewport(pOldTargetAndVP->viewport.x,
							pOldTargetAndVP->viewport.y,
							pOldTargetAndVP->viewport.x+pOldTargetAndVP->viewport.w,
							pOldTargetAndVP->viewport.y+pOldTargetAndVP->viewport.h
							,1.0f,0.0f);
		gfxc->setWindowOffset(
					0,
				   0
				);
		gfxc->setWindowScissor(
					0,
					0,
					pOldTargetAndVP->viewport.x+pOldTargetAndVP->viewport.w,
					pOldTargetAndVP->viewport.y+pOldTargetAndVP->viewport.h,
					sce::Gnm::kWindowOffsetDisable
				);
		gfxc->setViewportScissor(
			0,
			pOldTargetAndVP->viewport.x,
			pOldTargetAndVP->viewport.y,
			pOldTargetAndVP->viewport.x+pOldTargetAndVP->viewport.w,
			pOldTargetAndVP->viewport.y+pOldTargetAndVP->viewport.h,
			sce::Gnm::kWindowOffsetDisable
		);
}

void Framebuffer::DeactivateDepth(crossplatform::DeviceContext &deviceContext)
{
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	if(buffer_depth_texture&&buffer_depth_texture->IsValid())
	{
		gfxc->waitForGraphicsWrites(((orbis::Texture*)buffer_depth_texture)->AsGnmDepthRenderTarget()->getZWriteAddress256ByteBlocks(),((orbis::Texture*)buffer_depth_texture)->AsGnmDepthRenderTarget()->getZSliceSizeInBytes()>>8,
			sce::Gnm::kWaitTargetSlotDb
			,sce::Gnm::kCacheActionNone
			,sce::Gnm::kExtendedCacheActionFlushAndInvalidateDbCache
			,sce::Gnm::kStallCommandBufferParserEnable);
	}
	gfxc->setDepthRenderTarget(NULL);
	Utilities::DisableDepth(gfxc);
	depth_active=false;
}

void Framebuffer::GetTextureDimensions(const void* tex, unsigned int& widthOut, unsigned int& heightOut) const
{
	const sce::Gnm::Texture* pGnmTex = reinterpret_cast<const sce::Gnm::Texture*>(tex);
	widthOut = pGnmTex->getWidth();
	heightOut = pGnmTex->getHeight();
}

