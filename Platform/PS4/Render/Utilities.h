#ifndef UTILITIES_H
#define UTILITIES_H
#include "gnmx/lwgfxcontext.h"
#include "Simul/Base/MemoryInterface.h"
#include "Simul/Base/FileLoader.h"
#include "gnm/texture.h"
#include "gnm/buffer.h"
#include "gnm/sampler.h"
#include <algorithm>
#include <string>
#define SIMUL_PS4_ALWAYSVALIDATE(deviceContext)  if(deviceContext.asGfxContext()&&deviceContext.asGfxContext()->validate()) {BREAK_IF_DEBUGGING}
#ifdef _DEBUG
#define SIMUL_PS4_VALIDATE if(gfxc&&gfxc->validate()) {BREAK_ONCE_IF_DEBUGGING}
#define SIMUL_PS4_VALIDATE2(deviceContext)  if(deviceContext.asGfxContext()&&deviceContext.asGfxContext()->validate()) {BREAK_ONCE_IF_DEBUGGING}
#else
#define SIMUL_PS4_VALIDATE 
#define SIMUL_PS4_VALIDATE2(deviceContext) 
#endif
#define SIMUL_PS4_VALIDATE3(deviceContext) 
namespace simul
{
	namespace crossplatform
	{
		struct DeviceContext;
	}
	namespace orbis
	{
		class Texture;
		extern void SetFileLoader(simul::base::FileLoader *l);
		template<class T> struct ConstantBuffer
		{
			ConstantBuffer()
				:memoryInterface(NULL)
				,videoMemoryPtr(NULL)
			{
			}
			~ConstantBuffer()
			{
				release();
			}
			void init(simul::base::MemoryInterface *mem)
			{
				release();
				memoryInterface=mem;
				videoMemoryPtr=static_cast<T*>(memoryInterface->AllocateVideoMemory(sizeof(T),4));
				b.initAsConstantBuffer(videoMemoryPtr,sizeof(T));
			}
			void release()
			{
				if(memoryInterface)
					memoryInterface->DeallocateVideoMemory(videoMemoryPtr);
				videoMemoryPtr=NULL;
			}
			void apply(sce::Gnmx::LightweightGfxContext *gfxc,sce::Gnm::ShaderStage kShaderStage,int index)
			{
				gfxc->setConstantBuffers(kShaderStage,index,1,&b);
			}
			T &getStruct()
			{
				return *videoMemoryPtr;
			}
		protected:
			simul::base::MemoryInterface *memoryInterface;
			sce::Gnm::Buffer b;
			T *videoMemoryPtr;
		};
		//! A constant buffer wrapper that reallocates memory from the command buffer each time it is used.
		//! This is for buffers that need to be used multiple times per frame with different values.
		template<class T> struct VolatileConstantBuffer
		{
			VolatileConstantBuffer()
				:videoMemoryPtr(NULL)
			{
			}
			void apply(sce::Gnmx::LightweightGfxContext *gfxc,sce::Gnm::ShaderStage kShaderStage,int index)
			{
				gfxc->setConstantBuffers(kShaderStage,index,1,&b);
				videoMemoryPtr=NULL;
			}
			T &getStruct(sce::Gnmx::LightweightGfxContext *gfxc)
			{
				if(!videoMemoryPtr)
				{
					videoMemoryPtr=static_cast<T*>(gfxc->allocateFromCommandBuffer(sizeof(T),sce::Gnm::kEmbeddedDataAlignment4));
					b.initAsConstantBuffer(videoMemoryPtr,sizeof(T));
				}
				return *videoMemoryPtr;
			}
		protected:
			sce::Gnm::Buffer b;
			T *videoMemoryPtr;
		};
		//! A constant buffer wrapper that reallocates memory from the command buffer on demand
		//! This is for buffers that need to be used multiple times per frame with the same values, but may change between frames.
		template<class T> struct PerFrameConstantBuffer
		{
			PerFrameConstantBuffer()
				:videoMemoryPtr(NULL)
			{
			}
			void apply(sce::Gnmx::LightweightGfxContext *gfxc,sce::Gnm::ShaderStage kShaderStage,int index)
			{
				gfxc->setConstantBuffers(kShaderStage,index,1,&b);
			}
			T &getStruct(sce::Gnmx::LightweightGfxContext *gfxc)
			{
				videoMemoryPtr=static_cast<T*>(gfxc->allocateFromCommandBuffer(sizeof(T),sce::Gnm::kEmbeddedDataAlignment16));
				b.initAsConstantBuffer(videoMemoryPtr,sizeof(T));
				return *videoMemoryPtr;
			}
		protected:
			sce::Gnm::Buffer b;
			T *videoMemoryPtr;
		};
		
	enum MeshVertexBufferElement
	{
		kPosition,
		kNormal,
		kTangent,
		kTexture
	};
		class SimpleMesh
		{
		public:
			enum { kMaximumVertexBufferElements = 16 };

			void *m_vertexBuffer;
			uint32_t m_vertexBufferSize;
			uint32_t m_vertexCount;
			uint32_t m_vertexStride; // in bytes

			uint32_t m_vertexAttributeCount;

			void *m_indexBuffer;
			uint32_t m_indexBufferSize;
			uint32_t m_indexCount;
			sce::Gnm::IndexSize m_indexType;
			sce::Gnm::PrimitiveType m_primitiveType;

			void SetVertexBuffer(sce::Gnmx::LightweightGfxContext *gfxc, sce::Gnm::ShaderStage stage);

			sce::Gnm::Buffer m_buffer[kMaximumVertexBufferElements];

		};


		class Utilities
		{
			static simul::base::MemoryInterface	*allocator;
			static int screenWidth,screenHeight;
		public:
			
			static void Init(simul::base::MemoryInterface	*a);
			static void RestoreDeviceObjects(void *);
			static void RecompileShaders();
			static void ClearShaders();
			static void InvalidateDeviceObjects();
			
			static void DisableBlend(void *context);
			static void DisableDepth(void *context);
			static void EnableDepthWriteOnly(void *context);
			static void EnableDepthTestOnly(void *context,sce::Gnm::CompareFunc compare);
			static void EnableDepth(void *context,sce::Gnm::CompareFunc compare=sce::Gnm::kCompareFuncLessEqual);
			static void EnableOverrideDepth(void *context);
			static void EnableBlend(void *context);
			
			static void SetClampMirrorClampSamplerState	(void *context,int slot=5,sce::Gnm::ShaderStage s=sce::Gnm::kShaderStagePs);
			static void SetWrapSamplerState				(void *context,int slot=6,sce::Gnm::ShaderStage s=sce::Gnm::kShaderStagePs);
			static void SetWrapWrapClampSamplerState	(void *context,int slot=7,sce::Gnm::ShaderStage s=sce::Gnm::kShaderStagePs);
			static void SetClampWrapSamplerState		(void *context,int slot=8,sce::Gnm::ShaderStage s=sce::Gnm::kShaderStagePs);
			static void SetClampSamplerState			(void *context,int slot=9,sce::Gnm::ShaderStage s=sce::Gnm::kShaderStagePs);
			static void SetWrapClampSamplerState		(void *context,int slot=10,sce::Gnm::ShaderStage s=sce::Gnm::kShaderStagePs);
			static void SetWrapMirrorSamplerState		(void *context,int slot=3,sce::Gnm::ShaderStage s=sce::Gnm::kShaderStagePs);
			// We make everything clockwise wound.
			static void SetFaceCullMode(void *context, sce::Gnm::PrimitiveSetupCullFaceMode mode);

			static void DrawQuad(void *context,float x,float y,float w,float h);

			static void DrawQuad(void *context,int X,int Y,int W,int H);
			static void SetScreenSize(int W,int H);

			static void BuildCubeMesh(simul::base::MemoryInterface* allocator,SimpleMesh *destMesh, float side);
			static void RenderAngledQuad(void *context,const float *dir,const float *view,const float *proj,float half_angle_radians);
		};
	}
	namespace math
	{
		class Vector3;
		class Matrix4x4;
	}
	namespace sky
	{
		struct float4;
	}
}
extern void syncOnRt(sce::Gnmx::LightweightGfxContext *gfxc);
extern void synchronizeRenderTargetGraphicsToCompute(sce::Gnmx::LightweightGfxContext *gfxc, const sce::Gnm::RenderTarget* renderTarget);
/// Get graphics to wait for compute shaders.
extern void graphicsWaitForCompute( sce::Gnmx::LightweightGfxContext *gfxc );
extern void computeWaitForGraphics( sce::Gnmx::LightweightGfxContext *gfxc );

#define SET_CONSTANT_BUFFER(gfxc,kShaderStage,index,instance){\
	sce::Gnm::Buffer b;\
	b.initAsConstantBuffer(&instance,sizeof(instance));\
	gfxc->setConstantBuffers(kShaderStage,index,1,&b);}

#define APPLY_CONSTANT_BUFFER(gfxc,kShaderStage,index,InstanceType,instance){\
	InstanceType* x = static_cast<InstanceType*>(gfxc->allocateFromCommandBuffer(sizeof(InstanceType),sce::Gnm::kEmbeddedDataAlignment4));\
	*x=instance;\
	sce::Gnm::Buffer b;\
	b.initAsConstantBuffer(x,sizeof(InstanceType));\
	gfxc->setConstantBuffers(kShaderStage,index,1,&b);}

#endif

