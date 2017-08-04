#pragma once
#include "Simul/Platform/PS4/Render/Export.h"
#include "Simul/Platform/CrossPlatform/Texture.h"
#include "Simul/Platform/CrossPlatform/RenderPlatform.h"
#include "gnm/texture.h"
#include "gnmx/lwgfxcontext.h"
#include <string>
#include <map>
#ifdef _MSC_VER
#pragma warning(disable:4251)
#endif
namespace simul
{
	namespace orbis
	{
		class RenderPlatform;
		class SamplerState:public crossplatform::SamplerState
		{
			sce::Gnm::Sampler sampler;
		public:
			SamplerState(crossplatform::SamplerStateDesc *desc);
			virtual ~SamplerState() override;
			void InvalidateDeviceObjects() override;
			virtual sce::Gnm::Sampler *AsGnmSampler() override
			{
				return &sampler;
			}
		};
		class Texture:public simul::crossplatform::Texture
		{
		public:
			Texture(const char *name=NULL);
			~Texture();
			void InvalidateDeviceObjects() override;
			void LoadFromFile(crossplatform::RenderPlatform *r,const char *pFilePathUtf8) override;
			void LoadTextureArray(crossplatform::RenderPlatform *r,const std::vector<std::string> &texture_files) override;
			bool IsValid() const override;
			sce::Gnm::Texture *AsGnmTexture(crossplatform::ShaderResourceType resourceType=crossplatform::ShaderResourceType::UNKNOWN,int index=-1,int mip=-1) override;
			sce::Gnm::RenderTarget	*AsGnmRenderTarget(int index,int mip)
			{
				if(index<0)
					index=0;
				if(mip<0)
					mip=0;
#ifdef _DEBUG
				if(mip>=mips)
				{
					SIMUL_BREAK_ONCE("AsGnmRenderTarget: mip out of range");
					return NULL;
				}
				if(index>=NumFaces())
				{
					SIMUL_BREAK_ONCE("AsGnmRenderTarget: layer index out of range");
					return NULL;
				}
#endif
				if(!renderTargetViews)
					return nullptr;
				return &renderTargetViews[index][mip];				// Correspond to layerMipViews.			
			}
			sce::Gnm::DepthRenderTarget *AsGnmDepthRenderTarget()
			{
				return &depthTarget;
			}
			bool HasRenderTargets() const override
			{
				return (renderTargetViews!=nullptr);
			}
			/// Use this orbis::Texture as a wrapper for a nativ texture. Both pointers are needed.
			void InitFromExternalGnmTexture(sce::Gnm::Texture *t,bool make_rt);
			void InitFromExternalTexture2D(crossplatform::RenderPlatform *renderPlatform,void *t,void *srv,bool make_rt=false) override;

			void copyToMemory(crossplatform::DeviceContext &deviceContext,void *target,int start_texel=0,int texels=0) override;

			void setTexels(crossplatform::DeviceContext &deviceContext,const void *src,int texel_index,int num_texels) override;
			bool ensureTexture3DSizeAndFormat(crossplatform::RenderPlatform *renderPlatform,int w,int l,int d,crossplatform::PixelFormat f,bool computable,int mips=1,bool rendertargets=false) override;
			bool ensureTexture2DSizeAndFormat(crossplatform::RenderPlatform *renderPlatform,int w,int l
				,crossplatform::PixelFormat f,bool computable=false,bool rendertarget=false,bool depthstencil=false,int num_samples=1,int aa_quality=0,bool wrap=false) override;
			void ensureTexture1DSizeAndFormat(sce::Gnmx::LightweightGfxContext *pd3dDevice,int w,crossplatform::PixelFormat f,bool computable=false);
			bool ensureTextureArraySizeAndFormat(crossplatform::RenderPlatform *renderPlatform,int w,int l,int num,int mips,crossplatform::PixelFormat f,bool computable=false,bool rendertarget=false,bool cubemap=false) override;
			void GenerateMips(crossplatform::DeviceContext &deviceContext) override;
		
			void activateRenderTarget(crossplatform::DeviceContext &deviceContext,int array_index=-1,int mip_index=0) override;
			void deactivateRenderTarget(crossplatform::DeviceContext &deviceContext) override;
			virtual int GetLength() const override
			{
				return length;
			}
			virtual int GetWidth() const override
			{
				return width;
			}
			virtual int GetDimension() const override
			{
				return dim;
			}
			int GetSampleCount() const override;
			bool IsComputable() const override;

			void SyncRenderTarget(crossplatform::DeviceContext &deviceContext,bool stall_cb) const;
			void SyncRenderTargetToCompute(crossplatform::DeviceContext &deviceContext) const;
		protected:
			bool computable;
			sce::Gnm::Texture staging_texture;
			void reallocateMemory(sce::Gnm::SizeAlign sa,const char *name);
			void reallocateStagingMemory(uint32_t ls,sce::Gnm::SizeAlign sa,const char *name);
			simul::base::MemoryInterface *allocator;
			sce::Gnm::Texture			texture;
			sce::Gnm::Texture			cubemapArrayTexture;
			
			//ID3D11ShaderResourceView*		mainShaderResourceView;		// Same as main texture.	
			sce::Gnm::Texture				*layerViews;				// views for each layer, including all mips
			sce::Gnm::Texture				*mainMipViews;				// views for the whole texture at different mips.
			sce::Gnm::Texture				**layerMipViews;			// views for each layer at different mips.
			
			sce::Gnm::RenderTarget			**renderTargetViews;				// Correspond to layerMipViews.			

			sce::Gnm::DepthRenderTarget depthTarget;
			sce::GpuAddress::TilingParameters tp;
			crossplatform::DeviceContext *lastDeviceContext;
			uint8_t						*linearStagingTexels;
			uint8_t						*finalStagingTexels;
			sce::Gnm::SizeAlign			sizeAlign;
			
	sce::Gnm::ResourceHandle m_owner;
			sce::Gnm::SizeAlign			stagingSizeAlign;
			sce::Gnmx::ResourceBarrier resourceBarrier;				// Not always needed.
			uint32_t					stagingLinearSize;
			void AllocateViews(int a,int m);
			void ClearViews();
			void AllocateRenderTargets(int a,int m);
			void ClearRenderTargets();
			int GetNumUav() const;
			void CreateViews(int num,int m,bool rendertarget);
		public:
			void Swap(Texture &t)
			{
				std::swap(linearStagingTexels	,t.linearStagingTexels);
				std::swap(finalStagingTexels	,t.finalStagingTexels);
				std::swap(sizeAlign				,t.sizeAlign);
				std::swap(stagingSizeAlign		,t.stagingSizeAlign);
				std::swap(width					,t.width);
				std::swap(length				,t.length);
				std::swap(depth					,t.depth);
				std::swap(texture				,t.texture);
				std::swap(tp					,t.tp);
			}
		};
	}
}

namespace std
{
	template<> inline void swap(simul::orbis::Texture& _Left, simul::orbis::Texture& _Right)
	{
		_Left.Swap(_Right);
	}
}