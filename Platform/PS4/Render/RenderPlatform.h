#pragma once
#include "Export.h"
#include "Simul/Platform/CrossPlatform/RenderPlatform.h"
#include "Simul/Platform/CrossPlatform/BaseRenderer.h"
#include "Simul/Platform/PS4/Render/Effect.h"
#include "Simul/Platform/PS4/Render/Utilities.h"
#include "Simul/Platform/CrossPlatform/Effect.h"

#include <unordered_map>
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif

namespace simul
{
	namespace crossplatform
	{
		class Material;
		class ConstantBufferBase;
		struct DeviceContext;
	}
	namespace orbis
	{
		class ConstantBufferCache;
		class Material;
		class Shader;
		class Buffer;
		//! A class to implement common rendering functionality for DirectX 11.
		class RenderPlatform:public crossplatform::RenderPlatform
		{
			sce::Gnmx::LightweightGfxContext*				device;
		public:
			RenderPlatform(simul::base::MemoryInterface *m=NULL,sce::Gnmx::LightweightGfxContext *immediatecontext=NULL);
			virtual ~RenderPlatform();
			virtual const char *GetName() const override
			{
				return "PS4";
			}
			void RestoreDeviceObjects(void*) override;
			void InvalidateDeviceObjects() override;
			void RecompileShaders() override;
			
			void SynchronizeCacheAndState(crossplatform::DeviceContext &deviceContext) override;
			//! Because contexts are not properly independent in PS4, we must be handed a context to use by the external engine.
			void SetImmediateContext(sce::Gnmx::LightweightGfxContext *c)
			{
				immediateContext.platform_context=c;
				SynchronizeCacheAndState(immediateContext);
			}
			sce::Gnmx::LightweightGfxContext *asGfxContext()
			{
				return device;
			}
			void StartRender(crossplatform::DeviceContext &deviceContext) override;
			void EndRender(crossplatform::DeviceContext &deviceContext) override;
			void IntializeLightingEnvironment(const float pAmbientLight[3]) override;
			void BeginEvent	(crossplatform::DeviceContext &deviceContext,const char *name) override;
			void EndEvent	(crossplatform::DeviceContext &deviceContext) override;

			void CopyTexture(crossplatform::DeviceContext &,crossplatform::Texture *,crossplatform::Texture *) override;

			void DispatchCompute	(crossplatform::DeviceContext &deviceContext,int w,int l,int d) override;
			void WaitForGpu			(crossplatform::DeviceContext &deviceContext);
			void WaitForFencedResources(crossplatform::DeviceContext &deviceContext);
			
			void ApplyShaderPass(crossplatform::DeviceContext &deviceContext,crossplatform::Effect *,crossplatform::EffectTechnique *,int index);
			
			void Draw			(crossplatform::DeviceContext &deviceContext,int num_verts,int start_vert) override;
			void DrawIndexed	(crossplatform::DeviceContext &deviceContext,int num_indices,int start_index=0,int base_vertex=0) override;
			void DrawMarker		(crossplatform::DeviceContext &deviceContext,const double *matrix) override;
			void DrawLine(crossplatform::DeviceContext &deviceContext, const float *pGlobalBasePosition, const float *pGlobalEndPosition, const float *colour, float width) override;
			void DrawCrossHair	(crossplatform::DeviceContext &deviceContext,const double *pGlobalPosition) override;
			void DrawCamera		(crossplatform::DeviceContext &deviceContext,const double *pGlobalPosition, double pRoll) override;
			void DrawLineLoop	(crossplatform::DeviceContext &deviceContext,const double *mat,int num,const double *vertexArray,const float colr[4]) override;
			void DrawDepth		(crossplatform::DeviceContext &deviceContext,int x1,int y1,int dx,int dy,crossplatform::Texture *tex,const crossplatform::Viewport *v=NULL);
			//void DrawQuad		(crossplatform::DeviceContext &deviceContext, int x1, int y1, int dx, int dy, crossplatform::Effect *effect, crossplatform::EffectTechnique *technique, const char *pass) override;
			void DrawQuad		(crossplatform::DeviceContext &deviceContext) override;

			void DrawLines(crossplatform::DeviceContext &deviceContext, crossplatform::PosColourVertex *lines, int count, bool strip = false, bool test_depth = false, bool view_centred = false) override;
			void Draw2dLines(crossplatform::DeviceContext &deviceContext, crossplatform::PosColourVertex *lines, int vertex_count, bool strip) override;
			void DrawCircle		(crossplatform::DeviceContext &deviceContext,const float *dir,float rads,const float *colr,bool fill=false) override;
			void DrawCube		(crossplatform::DeviceContext &deviceContext);
		
			void ApplyDefaultMaterial() override;
			void SetModelMatrix(crossplatform::DeviceContext &deviceContext,const double *mat,const crossplatform::PhysicalLightRenderData &physicalLightRenderData) override;
			crossplatform::Material					*CreateMaterial() override;
			crossplatform::Mesh						*CreateMesh() override;
			crossplatform::Light					*CreateLight() override;
			crossplatform::Texture					*CreateTexture(const char *lFileNameUtf8=NULL) override;
			crossplatform::BaseFramebuffer			*CreateFramebuffer(const char *name) override;
			crossplatform::SamplerState				*CreateSamplerState(crossplatform::SamplerStateDesc *d) override;
			crossplatform::Effect					*CreateEffect() override;
			crossplatform::Effect					*CreateEffect(const char *filename_utf8,const std::map<std::string,std::string> &defines) override;
			crossplatform::PlatformConstantBuffer	*CreatePlatformConstantBuffer() override;
			crossplatform::PlatformStructuredBuffer	*CreatePlatformStructuredBuffer() override;
			crossplatform::Buffer					*CreateBuffer() override;
			crossplatform::Layout					*CreateLayout(int num_elements,const crossplatform::LayoutDesc *) override;
			crossplatform::RenderState				*CreateRenderState(const crossplatform::RenderStateDesc &desc) override;
			crossplatform::Query					*CreateQuery(crossplatform::QueryType q) override;

			crossplatform::Shader					*CreateShader() override;

			void									*GetDevice();
			void									SetVertexBuffers(crossplatform::DeviceContext &deviceContext, int slot, int num_buffers, crossplatform::Buffer *const*buffers, const crossplatform::Layout *layout, const int *vertexSteps) override;
			void									SetStreamOutTarget(crossplatform::DeviceContext &deviceContext, crossplatform::Buffer *buffer, int start_index) override;
			void									ActivateRenderTargets(crossplatform::DeviceContext &deviceContext, int num, crossplatform::Texture **targs, crossplatform::Texture *depth) override;
			void									DeactivateRenderTargets(crossplatform::DeviceContext &deviceContext) override;
		
			void									SetViewports(crossplatform::DeviceContext &deviceContext,int num,const crossplatform::Viewport *vps) override;
			void									SetIndexBuffer(crossplatform::DeviceContext &deviceContext,crossplatform::Buffer *buffer) override;
			
			void									SetTopology(crossplatform::DeviceContext &deviceContext,crossplatform::Topology t) override;

			void									StoreRenderState(crossplatform::DeviceContext &deviceContext) override;
			void									RestoreRenderState(crossplatform::DeviceContext &deviceContext) override;
			void									PopRenderTargets(crossplatform::DeviceContext &deviceContext) override;
			void									SetRenderState(crossplatform::DeviceContext &deviceContext,const crossplatform::RenderState *s) override;
			void									Resolve(crossplatform::DeviceContext &deviceContext,crossplatform::Texture *destination,crossplatform::Texture *source) override;
			void									SaveTexture(crossplatform::Texture *texture,const char *lFileNameUtf8) override;
			bool									ApplyContextState(crossplatform::DeviceContext &deviceContext,bool error_checking=true) override;


			// Gnm-specific stuff:
			static sce::Gnm::PrimitiveType ToGnmTopology(crossplatform::Topology t);
			static sce::Gnm::DataFormat ToGnmFormat(crossplatform::PixelFormat p);
			static crossplatform::PixelFormat FromGnmFormat(sce::Gnm::DataFormat f);
			static crossplatform::PixelFormat FromGnmDepthFormat(sce::Gnm::ZFormat d);
			static sce::Gnm::ZFormat ToGnmDepthFormat(crossplatform::PixelFormat f);
			static sce::Gnm::DataFormat DepthToEquivalentGnmFormat(crossplatform::PixelFormat f);
			static sce::Gnm::BlendFunc ToGnmBlendFunction(crossplatform::BlendOperation b);
			static sce::Gnm::BlendMultiplier ToGnmBlendMultiplier(crossplatform::BlendOption o);
			static sce::Gnm::CompareFunc ToGnmDepthComparison(crossplatform::DepthComparison d);

			
			static sce::Gnm::ScanModeControlViewportScissor ToGnm(crossplatform::ViewportScissor o);
			static sce::Gnm::PrimitiveSetupCullFaceMode ToGnm(crossplatform::CullFaceMode c);
			static sce::Gnm::PrimitiveSetupFrontFace ToGnm(crossplatform::FrontFace f);
			static sce::Gnm::PrimitiveSetupPolygonMode ToGnm(crossplatform::PolygonMode p);
			static sce::Gnm::PrimitiveSetupPolygonOffsetMode ToGnm(crossplatform::PolygonOffsetMode p);

			sce::Gnm::Texture *GetDummyTexture(int dims);
			crossplatform::PixelOutputFormat GetCurrentPixelOutputFormat(crossplatform::DeviceContext &deviceContext);
			virtual void ClearTexture(crossplatform::DeviceContext &deviceContext,crossplatform::Texture *texture,const vec4& colour) override;
			void ClearGnmTexture(crossplatform::DeviceContext &deviceContext, const sce::Gnm::Texture* texture, const float colour[4]);
			void ClearRenderTarget(crossplatform::DeviceContext &deviceContext,const sce::Gnm::RenderTarget* renderTarget,float r,float g,float b,float a);
			void ClearDepthStencil(crossplatform::DeviceContext &deviceContext, const sce::Gnm::DepthRenderTarget *depthTarget,float d);
			sce::Gnm::ResourceHandle GetResourceHandle();
			void ClearFencedTextures();
		protected:
			sce::Gnm::ResourceHandle resourceHandle;
			void CopyGnmTexture(crossplatform::DeviceContext &deviceContext, const sce::Gnm::Texture* textureDst, const sce::Gnm::Texture* textureSrc);
			sce::Gnmx::LightweightGfxContext *gfxc_i;
			sce::Gnm::Texture dummytexture2d;
			sce::Gnm::Texture dummytexture3d;
			void EnsureEffectIsBuilt(const char *filename_utf8,const std::vector<crossplatform::EffectDefineOptions> &options) override;
			void *esgsRingBuffer;
			void *gsvsRingBuffer;
		uint32_t kEsGsRingSizeInBytes;
		uint32_t kGsVsRingSizeInBytes;

		void *globalResourceTable;
			std::set<crossplatform::Texture*> fencedTextures;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif