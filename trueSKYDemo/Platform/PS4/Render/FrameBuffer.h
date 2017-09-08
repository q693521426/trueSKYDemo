#pragma once
#include "Simul/Platform/CrossPlatform/BaseFramebuffer.h"
#include "gnm/texture.h"
#include "gnm/rendertarget.h"
#include "gnm/depthrendertarget.h"
#include "gnm/commandbuffer.h"
#include "gnmx/lwgfxcontext.h"
#include "Simul/Platform/PS4/Render/Utilities.h"
#include "Simul/Platform/PS4/Render/Texture.h"
#include "Simul/Base/MemoryInterface.h"
namespace simul
{
	namespace orbis
	{
		class Framebuffer:public crossplatform::BaseFramebuffer
		{
		public:
			Framebuffer(const char *name=NULL);
			virtual ~Framebuffer();
			void RestoreDeviceObjects(crossplatform::RenderPlatform *renderPlatform) override;
			void ActivateDepth(crossplatform::DeviceContext &) override;
			void SetAntialiasing(int s) override;

			void InvalidateDeviceObjects() override;
			bool CreateBuffers() override;
			void Activate(crossplatform::DeviceContext &deviceContext) override;
			void SetExternalTextures(crossplatform::Texture *colour,crossplatform::Texture *depth) override;
			//! Activate, replacing the current fb instead of putting it on the stack. quicker?
			//void ActivateReplace(crossplatform::DeviceContext &deviceContext);
			void ActivateViewport(crossplatform::DeviceContext &deviceContext,float x,float y,float w,float h) override;
			void ActivateColour(crossplatform::DeviceContext &deviceContext,const float viewportXYWH[4]) override;
			void ActivateColour(crossplatform::DeviceContext &deviceContext);
			void Deactivate(crossplatform::DeviceContext &deviceContext) override;
			void DeactivateDepth(crossplatform::DeviceContext &deviceContext) override;
			void Clear(crossplatform::DeviceContext &deviceContext,float,float,float,float,float,int mask=0) override;
			void ClearColour(crossplatform::DeviceContext &deviceContext,float,float,float,float) override;
			void SetLinear(bool);
			void SetWidthAndHeight(int w,int h,int mips=-1) override;
			void SetAsCubemap(int face_size,int num_mips=1,crossplatform::PixelFormat f=crossplatform::RGBA_32_FLOAT) override;
			void SetFormat(crossplatform::PixelFormat) override;
			void SetDepthFormat(crossplatform::PixelFormat) override;
			void Sync(crossplatform::DeviceContext &deviceContext,bool stall_cb=false) const;
			void SyncToCompute(crossplatform::DeviceContext &deviceContext);
			bool IsValid() const override;
			
			void GetTextureDimensions(const void* tex, unsigned int& widthOut, unsigned int& heightOut) const;
			
		protected:
			simul::base::MemoryInterface *memoryInterface;
			bool makeLinear;
			orbis::Texture scratch_texture;
			void *depth2CpuBaseAddr;
			void *baseAddr;
			bool depth_active;	// Whether we are using the framebuffer's depth target just now
		};
	}
}
