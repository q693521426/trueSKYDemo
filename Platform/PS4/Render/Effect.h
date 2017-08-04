#pragma once
#include "Simul/Platform/PS4/Render/Export.h"
#include "Simul/Platform/CrossPlatform/Effect.h"
#include "Simul/Platform/CrossPlatform/Buffer.h"
#include <string>
#include <unordered_map>
#include "gnm/buffer.h"
#include <gnmx/shader_parser.h>
#include <shader/pssl_types.h>
#include <shader/binary.h>
#include <gnmx/shader_parser.h>
#include <gnmx/lwcue_base.h>
#ifdef _MSC_VER
#pragma warning(disable:4251)
#endif

namespace simul
{
	namespace orbis
	{
		struct Query:public crossplatform::Query
		{
			Query(crossplatform::QueryType t);
			~Query() override
			{
				InvalidateDeviceObjects();
			}
			void RestoreDeviceObjects(crossplatform::RenderPlatform *r) override;
			void InvalidateDeviceObjects() override;
			void Begin(crossplatform::DeviceContext &deviceContext) override;
			void End(crossplatform::DeviceContext &deviceContext) override;
			bool GetData(crossplatform::DeviceContext &deviceContext,void *data,size_t sz) override;
		protected:
			unsigned char *videomem[QueryLatency];
			crossplatform::RenderPlatform *renderPlatform;
		};
		struct RenderState:public crossplatform::RenderState
		{
			int num;
			sce::Gnm::BlendControl blendControls[8];
			sce::Gnm::DepthStencilControl depthStencilControls[8];
			sce::Gnm::PrimitiveSetup primitiveSetup;
			unsigned renderTargetWriteMask;
			RenderState();
			virtual ~RenderState();
		};
		class EffectPass;
		// Platform-specific data for constant buffer, managed by RenderPlatform.
		class PlatformConstantBuffer : public crossplatform::PlatformConstantBuffer
		{
			void *videoMemoryPtr;
			crossplatform::RenderPlatform *renderPlatform;
		public:
			sce::Gnm::Buffer b;
			void *addr;
			size_t size;
			PlatformConstantBuffer();
			~PlatformConstantBuffer()
			{
				InvalidateDeviceObjects();
			}
			void RestoreDeviceObjects(crossplatform::RenderPlatform *r,size_t sz,void *addr) override;
			void InvalidateDeviceObjects() override;
			void LinkToEffect(crossplatform::Effect *effect,const char *name,int bindingIndex) override;
			void Unbind(simul::crossplatform::DeviceContext &deviceContext) override;
			
			virtual void Apply(crossplatform::DeviceContext &deviceContext,size_t size,void *addr){}
			/// For RenderPlatform's use only: do not call.
			virtual void ActualApply(simul::crossplatform::DeviceContext &deviceContext,crossplatform::EffectPass *currentEffectPass,int index) override;
		};
		class PlatformStructuredBuffer:public crossplatform::PlatformStructuredBuffer
		{
			static const int latency = 3;
			sce::Gnm::Buffer				buffer;
			crossplatform::DeviceContext	*lastContext;
			int num_elements;
			int currFrame;
			int element_bytesize;
			unsigned char *staging_data[latency];
			unsigned char *read_data;
			crossplatform::RenderPlatform *renderPlatform;
		public:
			PlatformStructuredBuffer()
				:lastContext(NULL)
				,num_elements(0)
				,currFrame(0)
				,element_bytesize(0)
				,read_data(NULL)
				,renderPlatform(NULL)
			{
				for(int i=0;i<latency;i++)
					staging_data[i]=0;
			}
			virtual ~PlatformStructuredBuffer()
			{
				InvalidateDeviceObjects();
			}
			void RestoreDeviceObjects(crossplatform::RenderPlatform *renderPlatform,int ct,int unit_size,bool computable,void *init_data);
			void *GetBuffer(crossplatform::DeviceContext &deviceContext) override;
			const void *OpenReadBuffer(crossplatform::DeviceContext &deviceContext) override;
			void CloseReadBuffer(crossplatform::DeviceContext &deviceContext) override;
			void CopyToReadBuffer(crossplatform::DeviceContext &deviceContext) override;
			void SetData(crossplatform::DeviceContext &deviceContext,void *data) override;
			void InvalidateDeviceObjects() override;
			void LinkToEffect(crossplatform::Effect *effect,const char *name,int bindingIndex) ;
			void Apply(crossplatform::DeviceContext &deviceContext,crossplatform::Effect *effect,const char *name) override;
			void ApplyAsUnorderedAccessView(crossplatform::DeviceContext &deviceContext,crossplatform::Effect *effect,const char *name) override;
			void Unbind(crossplatform::DeviceContext &deviceContext) override;
			
			/// For RenderPlatform's use only: do not call.
			virtual void ActualApply(crossplatform::DeviceContext &deviceContext,crossplatform::EffectPass *currentEffectPass,int slot) override;
		};
		//! This is the class for individual shaders, which we will hold in a single list per-RenderPlatform,
		//! and select from for each technique pass.
		class Shader:public crossplatform::Shader
		{
			//! As far as I can tell, the shader binary Program just holds semantic information, it doesn't do actual shading.
			sce::Shader::Binary::Program sbProgram;// Generate shader resource table
		public:
			sce::Gnmx::InputResourceOffsets resourceOffsets;
			void release();
			void load(crossplatform::RenderPlatform *renderPlatform,const char *filename,crossplatform::ShaderType t) override;
			union
			{
				sce::Gnmx::EsShader			*exportShader;
				sce::Gnmx::GsShader			*geometryShader;
				sce::Gnmx::VsShader			*vertexShader;
				sce::Gnmx::PsShader			*pixelShader;
				sce::Gnmx::CsShader			*computeShader;
			};
			void						*fetchShader;
			uint32_t					shaderModifier;
			sce::Gnm::ResourceHandle resourceHandle;
		};
		class EffectPass:public crossplatform::EffectPass
		{
		public:
			void Apply(crossplatform::DeviceContext &deviceContext,bool test) override;
		};
		class EffectTechnique:public simul::crossplatform::EffectTechnique
		{
		public:
			crossplatform::EffectPass *AddPass(const char *name,int i) override;
		};
		class Effect : public simul::crossplatform::Effect
		{
		protected:
			crossplatform::EffectTechnique *CreateTechnique() override;
			void ParseBuffer(const sce::Shader::Binary::Program &program);
		public:
			Effect();
			virtual ~Effect();
			void InvalidateDeviceObjects() ;
			crossplatform::EffectTechnique *GetTechniqueByName(const char *name) override;
			crossplatform::EffectTechnique *GetTechniqueByIndex(int index) override;
			crossplatform::ShaderResource GetShaderResource(const char *name) override;
			//! Set a constant buffer for this effect.
			void SetConstantBuffer(crossplatform::DeviceContext &deviceContext,const char *name	,crossplatform::ConstantBufferBase *s)	override;
			void SetSamplerState(crossplatform::DeviceContext &deviceContext,const char *name,crossplatform::SamplerState *s) override;

			void Apply(crossplatform::DeviceContext &deviceContext,crossplatform::EffectTechnique *effectTechnique,int pass) override;
			void Apply(crossplatform::DeviceContext &deviceContext,crossplatform::EffectTechnique *effectTechnique,const char *pass) override;
			void Reapply(crossplatform::DeviceContext &deviceContext) override;
			void Unapply(crossplatform::DeviceContext &deviceContext) override;
		};
	}
}
