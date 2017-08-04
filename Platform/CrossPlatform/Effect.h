#pragma once
#include "Simul/Platform/CrossPlatform/Export.h"
#include "Simul/Platform/CrossPlatform/SL/CppSl.hs"
#include "Simul/Platform/CrossPlatform/Topology.h"
#include "Simul/Platform/CrossPlatform/PixelFormat.h"
#include "Simul/Base/RuntimeError.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <set>
#include <stdint.h>
struct ID3DX11Effect;
struct ID3DX11EffectTechnique;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
typedef unsigned int GLuint;
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable:4251)
#endif

namespace simul
{
	namespace crossplatform
	{
		/*!
				OpenGL					|	Direct3D
				-------------------------------------------
				Vertex Shader			|	Vertex Shader
				Tessellation Control	|	Hull Shader
				Tessellation Evaluation	|	Domain Shader
				Geometry Shader			|	Geometry Shader
				Fragment Shader			|	Pixel Shader
				Compute Shader			|	Compute Shader
		*/
		enum ShaderType
		{
			SHADERTYPE_VERTEX,
			SHADERTYPE_HULL,		// tesselation control.
			SHADERTYPE_DOMAIN,		// tesselation evaluation.
			SHADERTYPE_GEOMETRY,
			SHADERTYPE_PIXEL,
			SHADERTYPE_COMPUTE,
			SHADERTYPE_COUNT
		};
		/// Tells the renderer what to do with shader source to get binaries. values can be combined, e.g. ALWAYS_BUILD|TRY_AGAIN_ON_FAIL
		enum ShaderBuildMode
		{
			NEVER_BUILD=0
			,ALWAYS_BUILD=1
			,BUILD_IF_CHANGED=2
			, BREAK_ON_FAIL = 8			// 0x1000
			, TRY_AGAIN_ON_FAIL = 12	// 0x11000 - includes break.
		};
		inline ShaderBuildMode operator|(ShaderBuildMode a, ShaderBuildMode b)
		{
			return static_cast<ShaderBuildMode>(static_cast<int>(a) | static_cast<int>(b));
		}
		inline ShaderBuildMode operator&(ShaderBuildMode a, ShaderBuildMode b)
		{
			return static_cast<ShaderBuildMode>(static_cast<int>(a) & static_cast<int>(b));
		}
		inline ShaderBuildMode operator~(ShaderBuildMode a)
		{
			return static_cast<ShaderBuildMode>(~static_cast<int>(a));
		}
		struct DeviceContext;
		class RenderPlatform;
		struct Query;
		class Effect;
		/// A disjoint query structure, like those in DirectX 11.
		/// Its main use is actually to get the clock frequency that will
		/// be used for timestamp queries, but the Disjoint value is
		/// used on some platforms to indicate whether the timestamp values are invalid.
		struct DisjointQueryStruct
		{
			uint64_t Frequency;
			int		Disjoint;
		};
		/// Crossplatform GPU query class.
		struct SIMUL_CROSSPLATFORM_EXPORT Query
		{
			static const int QueryLatency = 6;
			bool QueryStarted;
			bool QueryFinished;
			int currFrame;
			QueryType type;
			bool gotResults[QueryLatency];
			bool doneQuery[QueryLatency];
			Query(QueryType t)
				:QueryStarted(false)
				,QueryFinished(false)
				,currFrame(0)
				,type(t)
			{
				for(int i=0;i<QueryLatency;i++)
				{
					gotResults[i]=true;
					doneQuery[i]=false;
				}
			}
			virtual ~Query()
			{
			}
			virtual void RestoreDeviceObjects(crossplatform::RenderPlatform *r)=0;
			virtual void InvalidateDeviceObjects()=0;
			virtual void Begin(DeviceContext &deviceContext) =0;
			virtual void End(DeviceContext &deviceContext) =0;
			/// Get query data. Returns true if successful, or false otherwise.
			/// Blocking queries will return false until they succeed.
			virtual bool GetData(DeviceContext &deviceContext,void *data,size_t sz) =0;
			virtual void SetName(const char *){}
		};
		
		enum BlendOption
		{
			BLEND_ZERO
			,BLEND_ONE
			,BLEND_SRC_COLOR
			,BLEND_INV_SRC_COLOR
			,BLEND_SRC_ALPHA
			,BLEND_INV_SRC_ALPHA
			,BLEND_DEST_ALPHA
			,BLEND_INV_DEST_ALPHA
			,BLEND_DEST_COLOR
			,BLEND_INV_DEST_COLOR
			,BLEND_SRC_ALPHA_SAT
			,BLEND_BLEND_FACTOR
			,BLEND_INV_BLEND_FACTOR
			,BLEND_SRC1_COLOR
			,BLEND_INV_SRC1_COLOR
			,BLEND_SRC1_ALPHA
			,BLEND_INV_SRC1_ALPHA
		};
		enum BlendOperation
		{
			BLEND_OP_NONE
			,BLEND_OP_ADD
			,BLEND_OP_SUBTRACT
			,BLEND_OP_MAX
			,BLEND_OP_MIN
		};
		struct RTBlendDesc
		{
			BlendOperation blendOperation;
			BlendOperation blendOperationAlpha;
			BlendOption SrcBlend;
			BlendOption DestBlend;
			BlendOption SrcBlendAlpha;
			BlendOption DestBlendAlpha;
			unsigned char RenderTargetWriteMask;
		};
		struct BlendDesc
		{
			bool AlphaToCoverageEnable;
			bool IndependentBlendEnable;
			int numRTs;
			RTBlendDesc RenderTarget[8];
		};
		enum DepthComparison
		{
			DEPTH_ALWAYS,
			DEPTH_NEVER,
			DEPTH_LESS,
			DEPTH_EQUAL,
			DEPTH_LESS_EQUAL,
			DEPTH_GREATER,
			DEPTH_NOT_EQUAL,
			DEPTH_GREATER_EQUAL
		};
		struct DepthStencilDesc
		{
			bool test;
			bool write;
			DepthComparison comparison;
		};
		enum ViewportScissor
		{
			VIEWPORT_SCISSOR_DISABLE      = 0, ///< Disable the scissor rectangle for a viewport.
			VIEWPORT_SCISSOR_ENABLE       = 1, ///< Enable the scissor rectangle for a viewport.
		};
		enum CullFaceMode
		{
			CULL_FACE_NONE              = 0, ///< Disable face culling.
			CULL_FACE_FRONT             = 1, ///< Cull front-facing primitives only.
			CULL_FACE_BACK              = 2, ///< Cull back-facing primitives only.
			CULL_FACE_FRONTANDBACK      = 3, ///< Cull front and back faces.
		};
		enum FrontFace
		{
			FRONTFACE_CLOCKWISE                     = 1, ///< Clockwise is front-facing.
			FRONTFACE_COUNTERCLOCKWISE              = 0, ///< Counter-clockwise is front-facing.
		};
		enum PolygonMode
		{
			POLYGON_MODE_POINT              = 0, ///< Render polygons as points.
			POLYGON_MODE_LINE               = 1, ///< Render polygons in wireframe.
			POLYGON_MODE_FILL               = 2, ///< Render polygons as solid/filled.
		};
		enum PolygonOffsetMode
		{
			POLYGON_OFFSET_ENABLE           = 1, ///< Enable polygon offset.
			POLYGON_OFFSET_DISABLE          = 0, ///< Disable polygon offset.
		} ;
		struct RasterizerDesc
		{
			ViewportScissor		viewportScissor;
			CullFaceMode		cullFaceMode;
			FrontFace			frontFace;
			PolygonMode			polygonMode;
			PolygonOffsetMode	polygonOffsetMode;
		};
		enum RenderStateType
		{
			NONE
			,BLEND
			,DEPTH
			,RASTERIZER
			,NUM_RENDERSTATE_TYPES
		};
		/// An initialization structure for a RenderState. Create a RenderStateDesc and pass it to RenderPlatform::CreateRenderState,
		/// then store the returned pointer. Delete the pointer when done.
		struct RenderStateDesc
		{
			RenderStateType type;
			union
			{
				DepthStencilDesc depth;
				BlendDesc blend;
				RasterizerDesc rasterizer;
			};
		};
		struct SIMUL_CROSSPLATFORM_EXPORT RenderState
		{
			RenderStateType type;
			RenderState():type(NONE){}
			virtual ~RenderState(){}
		};
		class SIMUL_CROSSPLATFORM_EXPORT EffectPass
		{
		public:
			EffectPass()
				:samplerSlots(0)
				,bufferSlots(0)
				,textureSlots(0)
				,textureSlotsForSB(0)
				,rwTextureSlots(0)
				,rwTextureSlotsForSB(0)
				,should_fence_outputs(true)
				,platform_pass(nullptr)
			{}
			virtual ~EffectPass(){}
			bool usesTextureSlot(int s) const;
			bool usesTextureSlotForSB(int s) const;
			bool usesRwTextureSlotForSB(int s) const;
			bool usesBufferSlot(int s) const;
			bool usesSamplerSlot(int s) const;
			bool usesRwTextureSlot(int s) const;

			bool usesTextures() const;
			bool usesSBs() const;
			bool usesRwSBs() const;
			bool usesBuffers() const;
			bool usesSamplers() const;
			bool usesRwTextures() const;
			bool shouldFenceOutputs() const
			{
				return should_fence_outputs;
			}
			
			void setShouldFenceOutputs(bool f) 
			{
				should_fence_outputs=f;
			}
			void SetUsesBufferSlots(unsigned);
			void SetUsesTextureSlots(unsigned);
			void SetUsesTextureSlotsForSB(unsigned);
			void SetUsesRwTextureSlots(unsigned);
			void SetUsesRwTextureSlotsForSB(unsigned);
			void SetUsesSamplerSlots(unsigned);
			unsigned GetRwTextureSlots() const
			{
				return rwTextureSlots;
			}
			unsigned GetTextureSlots() const
			{
				return textureSlots;
			}
			unsigned GetRwStructuredBufferSlots() const
			{
				return rwTextureSlotsForSB;
			}
			unsigned GetStructuredBufferSlots() const
			{
				return textureSlotsForSB;
			}
			unsigned GetConstantBufferSlots() const
			{
				return bufferSlots;
			}
			void *GetPlatformPass()
			{
				return platform_pass;
			}
			void SetPlatformPass(void *p)
			{
				platform_pass=p;
			}
		protected:
			unsigned samplerSlots;
			unsigned bufferSlots;
			unsigned textureSlots;
			unsigned textureSlotsForSB;
			unsigned rwTextureSlots;
			unsigned rwTextureSlotsForSB;
			bool should_fence_outputs;
			void *platform_pass;
		};
		class SIMUL_CROSSPLATFORM_EXPORT PlatformConstantBuffer
		{
		protected:
			bool changed;
		public:
			PlatformConstantBuffer():changed(true){}
			virtual ~PlatformConstantBuffer(){}
			virtual void RestoreDeviceObjects(RenderPlatform *dev,size_t sz,void *addr)=0;
			virtual void InvalidateDeviceObjects()=0;
			virtual void LinkToEffect(crossplatform::Effect *effect,const char *name,int bindingIndex)=0;
			virtual void Apply(DeviceContext &deviceContext,size_t size,void *addr)=0;
			virtual void Reapply(DeviceContext &deviceContext,size_t size,void *addr)
			{
				Apply(deviceContext,size,addr);
			}
			virtual void Unbind(DeviceContext &deviceContext)=0;
			void SetChanged()
			{
				changed=true;
			}
			/// For RenderPlatform's use only: do not call.
			virtual void ActualApply(simul::crossplatform::DeviceContext &,EffectPass *,int){}
		};
		class SIMUL_CROSSPLATFORM_EXPORT ConstantBufferBase
		{
		protected:
			PlatformConstantBuffer *platformConstantBuffer;
			std::string defaultName;
		public:
			ConstantBufferBase():platformConstantBuffer(NULL)
			{
			}
			~ConstantBufferBase()
			{
				delete platformConstantBuffer;
			}
			const char *GetDefaultName() const
			{
				return defaultName.c_str();
			}
		
			PlatformConstantBuffer *GetPlatformConstantBuffer()
			{
				return platformConstantBuffer;
			}
			virtual int GetIndex() const=0;
			virtual size_t GetSize() const=0;
			virtual void * GetAddr() const=0;
		};
		template<class T> class ConstantBuffer:public ConstantBufferBase,public T
		{
			std::set<Effect*> linkedEffects;
		public:
			ConstantBuffer():ConstantBufferBase()
			{
				// Clear out the part of memory that corresponds to the base class.
				// We should ONLY inherit from simple structs.
				memset(((T*)this),0,sizeof(T));
			}
			~ConstantBuffer()
			{
				InvalidateDeviceObjects();
			}
			void copyTo(void *pData)
			{
				*(T*)pData = *this;
			}
			/// For Effect's use only, do not call.
			size_t GetSize() const override
			{
				return sizeof(T);
			}
			/// For Effect's use only, do not call.
			void * GetAddr() const override
			{
				return (void*)((T*)this);
			}
			/// Get the binding index in shaders.
			int GetIndex() const override
			{
				return T::bindingIndex;
			}
			//! Create the buffer object.
#ifdef _MSC_VER
			void RestoreDeviceObjects(RenderPlatform *p)
			{
				InvalidateDeviceObjects();
				if (p)
				{
					platformConstantBuffer = p->CreatePlatformConstantBuffer();
					platformConstantBuffer->RestoreDeviceObjects(p, sizeof(T), (T*)this);
				}
			}
			void LinkToEffect(Effect *effect, const char *name)
			{
				if (!effect)
					return;
				if (IsLinkedToEffect(effect))
					return;
				defaultName=name;
				SIMUL_ASSERT(platformConstantBuffer!=nullptr);
				SIMUL_ASSERT(effect!=nullptr);
				if (effect&&platformConstantBuffer)
				{
					platformConstantBuffer->LinkToEffect(effect, name, T::bindingIndex);
					linkedEffects.insert(effect);
					effect->StoreConstantBufferLink(this);
				}
			}
			bool IsLinkedToEffect(crossplatform::Effect *effect)
			{
				if (!effect)
					return false;
				if (linkedEffects.find(effect) != linkedEffects.end())
				{
					if (effect->IsLinkedToConstantBuffer(this))
						return true;
				}
				return false;
			}
#else
			void RestoreDeviceObjects(RenderPlatform *p);
			//! Find the constant buffer in the given effect, and link to it.
			void LinkToEffect(Effect *effect, const char *name);
			bool IsLinkedToEffect(crossplatform::Effect *effect);
#endif
			//! Free the allocated buffer.
			void InvalidateDeviceObjects()
			{
				linkedEffects.clear();
				if(platformConstantBuffer)
					platformConstantBuffer->InvalidateDeviceObjects();
				delete platformConstantBuffer;
				platformConstantBuffer=NULL;
			}
			//! Unbind from the effect.
			void Unbind(DeviceContext &deviceContext)
			{
				if(platformConstantBuffer)
					platformConstantBuffer->Unbind(deviceContext);
			}
		};
		/// A base class for structured buffers, used by StructuredBuffer internally.
		class SIMUL_CROSSPLATFORM_EXPORT PlatformStructuredBuffer
		{
		protected:
			int numCopies;	// for tracking when the data should be valid, i.e. when numCopies==Latency.
		public:
			PlatformStructuredBuffer():numCopies(0){}
			virtual ~PlatformStructuredBuffer(){}
			virtual void RestoreDeviceObjects(RenderPlatform *r,int count,int unit_size,bool computable,void *init_data)=0;
			virtual void InvalidateDeviceObjects()=0;
			virtual void Apply(DeviceContext &deviceContext,Effect *effect,const char *name)=0;
			virtual void ApplyAsUnorderedAccessView(DeviceContext &deviceContext,Effect *effect,const char *name)=0;
			virtual void Unbind(DeviceContext &deviceContext)=0;
			virtual void *GetBuffer(crossplatform::DeviceContext &deviceContext)=0;
			virtual const void *OpenReadBuffer(crossplatform::DeviceContext &deviceContext)=0;
			virtual void CloseReadBuffer(crossplatform::DeviceContext &deviceContext)=0;
			virtual void CopyToReadBuffer(crossplatform::DeviceContext &deviceContext)=0;
			virtual void SetData(crossplatform::DeviceContext &deviceContext,void *data)=0;
			virtual ID3D11ShaderResourceView *AsD3D11ShaderResourceView(){return NULL;}
			virtual ID3D11UnorderedAccessView *AsD3D11UnorderedAccessView(int =0){return NULL;}
			void ResetCopies()
			{
				numCopies=0;
			}
			/// For RenderPlatform's use only: do not call.
			virtual void ActualApply(simul::crossplatform::DeviceContext & /*deviceContext*/,EffectPass * /*currentEffectPass*/,int /*slot*/){}
		};
		class SIMUL_CROSSPLATFORM_EXPORT BaseStructuredBuffer
		{
		};

		/// Templated structured buffer, which uses platform-specific implementations of PlatformStructuredBuffer.
		///
		/// Declare like so:
		/// \code
		/// 	StructuredBuffer<Example> example;
		/// \endcode
		template<class T> class StructuredBuffer : public BaseStructuredBuffer
		{
			PlatformStructuredBuffer *platformStructuredBuffer;
		public:
			StructuredBuffer()
				:platformStructuredBuffer(NULL)
				,count(0)
			{
			}
			~StructuredBuffer()
			{
				InvalidateDeviceObjects();
			}
#ifdef _MSC_VER
			void RestoreDeviceObjects(RenderPlatform *p, int ct, bool computable = false, T *data = NULL)
			{
				if(!p)
					return;
				count = ct;
				delete platformStructuredBuffer;
				platformStructuredBuffer = NULL;
				platformStructuredBuffer = p->CreatePlatformStructuredBuffer();
				platformStructuredBuffer->RestoreDeviceObjects(p, count, sizeof(T), computable, data);
			}
#else
			void RestoreDeviceObjects(RenderPlatform *p, int ct, bool computable = false, T *data = NULL);
#endif
			T *GetBuffer(crossplatform::DeviceContext &deviceContext)
			{
				if (!platformStructuredBuffer)
				{
					SIMUL_BREAK_ONCE("Null Platform structured buffer pointer.");
					return NULL;
				}
				return (T*)platformStructuredBuffer->GetBuffer(deviceContext);
			}
			const T *OpenReadBuffer(crossplatform::DeviceContext &deviceContext)
			{
				if (!platformStructuredBuffer)
				{
					SIMUL_BREAK_ONCE("Null Platform structured buffer pointer.");
					return NULL;
				}
				return (const T*)platformStructuredBuffer->OpenReadBuffer(deviceContext);
			}
			void CloseReadBuffer(crossplatform::DeviceContext &deviceContext)
			{
				if (!platformStructuredBuffer)
				{
					SIMUL_BREAK_ONCE("Null Platform structured buffer pointer.");
					return;
				}
				platformStructuredBuffer->CloseReadBuffer(deviceContext);
			}
			void CopyToReadBuffer(crossplatform::DeviceContext &deviceContext)
			{
				if (!platformStructuredBuffer)
				{
					SIMUL_BREAK_ONCE("Null Platform structured buffer pointer.");
					return;
				}
				platformStructuredBuffer->CopyToReadBuffer(deviceContext);
			}
			void SetData(crossplatform::DeviceContext &deviceContext,T *data)
			{
				if (!platformStructuredBuffer)
				{
					SIMUL_BREAK_ONCE("Null Platform structured buffer pointer.");
					return;
				}
					platformStructuredBuffer->SetData(deviceContext,(void*)data);
			}
			ID3D11ShaderResourceView *AsD3D11ShaderResourceView()
			{
				if (!platformStructuredBuffer)
				{
					SIMUL_BREAK_ONCE("Null Platform structured buffer pointer.");
					return NULL;
				}
				return platformStructuredBuffer->AsD3D11ShaderResourceView();
			}
			ID3D11UnorderedAccessView *AsD3D11UnorderedAccessView(int mip=0)
			{
				if (!platformStructuredBuffer)
				{
					SIMUL_BREAK_ONCE("Null Platform structured buffer pointer.");
					return NULL;
				}
				return platformStructuredBuffer->AsD3D11UnorderedAccessView();
			}
			void Apply(crossplatform::DeviceContext &pContext,crossplatform::Effect *effect,const char *name)
			{
				if (!platformStructuredBuffer)
				{
					SIMUL_BREAK_ONCE("Null Platform structured buffer pointer.");
					return ;
				}
				platformStructuredBuffer->Apply(pContext,effect,name);
			}
			void ApplyAsUnorderedAccessView(crossplatform::DeviceContext &pContext,crossplatform::Effect *effect,const char *name)
			{
				if (!platformStructuredBuffer)
				{
					SIMUL_BREAK_ONCE("Null Platform structured buffer pointer.");
					return;
				}
				platformStructuredBuffer->ApplyAsUnorderedAccessView(pContext,effect,name);
			}
			void InvalidateDeviceObjects()
			{
				if(platformStructuredBuffer)
					platformStructuredBuffer->InvalidateDeviceObjects();
				delete platformStructuredBuffer;
				platformStructuredBuffer=NULL;
				count=0;
			}
			void ResetCopies()
			{
				if(platformStructuredBuffer)
					platformStructuredBuffer->ResetCopies();
			}

			int count;
		};
		class Texture;
		class SamplerState;
		struct SIMUL_CROSSPLATFORM_EXPORT ShaderResource
		{
			ShaderResource():slot(-1),dimensions(-1),valid(false){}
			ShaderResourceType shaderResourceType;
			void *platform_shader_resource;
			int slot;
			int dimensions;
			bool valid;
		};
		class SIMUL_CROSSPLATFORM_EXPORT EffectTechnique
		{
		public:
			typedef std::map<std::string,EffectPass *> PassMap;
			typedef std::map<int,EffectPass *> PassIndexMap;
			typedef std::map<std::string,int> IndexMap;
			std::string name;
			PassMap passes_by_name;
			PassIndexMap passes_by_index;
			IndexMap pass_indices;
			EffectTechnique();
			virtual ~EffectTechnique();
			void *platform_technique;
			inline ID3DX11EffectTechnique *asD3DX11EffectTechnique()
			{
				return (ID3DX11EffectTechnique*)platform_technique;
			}
			virtual GLuint passAsGLuint(int )
			{
				return (GLuint)0;
			}
			virtual GLuint passAsGLuint(const char *)
			{
				return (GLuint)0;
			}
			inline int GetPassIndex(const char *n)
			{
				std::string str(n);
				if(pass_indices.find(str)==pass_indices.end())
					return -1;
				return pass_indices[str];
			}
			bool shouldFenceOutputs() const
			{
				return should_fence_outputs;
			}
			
			void setShouldFenceOutputs(bool f) 
			{
				should_fence_outputs=f;
			}
			bool should_fence_outputs;
			int NumPasses() const;
			virtual EffectPass *AddPass(const char *name,int i);
			EffectPass *GetPass(int i);
			EffectPass *GetPass(const char *name);
		};
		typedef std::map<std::string,EffectTechnique *> TechniqueMap;
		typedef std::unordered_map<const char *,EffectTechnique *> TechniqueCharMap;
		typedef std::map<int,EffectTechnique *> IndexMap;
		/// Crossplatform equivalent of D3DXEffectGroup - a named group of techniques.
		class SIMUL_CROSSPLATFORM_EXPORT EffectTechniqueGroup
		{
			TechniqueCharMap charMap;
		public:
			~EffectTechniqueGroup();
			TechniqueMap techniques;
			IndexMap techniques_by_index;
			EffectTechnique *GetTechniqueByName(const char *name);
			EffectTechnique *GetTechniqueByIndex(int index);
		};
		/// A crossplatform structure for a \#define and its possible values.
		/// This allows all of the macro combinations to be built to binary.
		struct SIMUL_CROSSPLATFORM_EXPORT EffectDefineOptions
		{
			std::string name;
			std::vector<std::string> options;
		};
		extern SIMUL_CROSSPLATFORM_EXPORT EffectDefineOptions CreateDefineOptions(const char *name,const char *option1);
		extern SIMUL_CROSSPLATFORM_EXPORT EffectDefineOptions CreateDefineOptions(const char *name,const char *option1,const char *option2);
		extern SIMUL_CROSSPLATFORM_EXPORT EffectDefineOptions CreateDefineOptions(const char *name,const char *option1,const char *option2,const char *option3);
		typedef std::map<std::string,EffectTechniqueGroup *> GroupMap;
		typedef std::unordered_map<const char *,EffectTechniqueGroup *> GroupCharMap;
		/// The cross-platform base class for shader effects.
		class SIMUL_CROSSPLATFORM_EXPORT Effect
		{
		protected:
			crossplatform::RenderPlatform *renderPlatform;
			virtual EffectTechnique *CreateTechnique()=0;
			EffectTechnique *EnsureTechniqueExists(const std::string &groupname,const std::string &techname,const std::string &passname);
			const char *GetTechniqueName(const EffectTechnique *t) const;
			std::set<ConstantBufferBase*> linkedConstantBuffers;
			std::map<const char *,crossplatform::ShaderResource> shaderResources;
			GroupCharMap groupCharMap;
			typedef std::unordered_map<std::string,ShaderResource*> TextureDetailsMap;
			typedef std::unordered_map<const char *,ShaderResource*> TextureCharMap;
			TextureDetailsMap textureDetailsMap;
			mutable TextureCharMap textureCharMap;
			std::unordered_map<std::string,crossplatform::SamplerState *> samplerStates;
			std::unordered_map<std::string,crossplatform::RenderState *> depthStencilStates;
			std::unordered_map<std::string,crossplatform::RenderState *> blendStates;
			std::unordered_map<std::string,crossplatform::RenderState *> rasterizerStates;
			const ShaderResource *GetTextureDetails(const char *name);
		public:
			GroupMap groups;
			TechniqueMap techniques;
			IndexMap techniques_by_index;
			std::string filename;
			std::string filenameInUseUtf8;
			int apply_count;
			int currentPass;
			crossplatform::EffectTechnique *currentTechnique;
			void *platform_effect;
			Effect();
			virtual ~Effect();
			inline ID3DX11Effect *asD3DX11Effect()
			{
				return (ID3DX11Effect*)platform_effect;
			}
			void SetName(const char *n)
			{
				filename=n;
			}
			const char *GetName()const
			{
				return filename.c_str();
			}
			void InvalidateDeviceObjects();
			virtual void Load(RenderPlatform *renderPlatform,const char *filename_utf8,const std::map<std::string,std::string> &defines)=0;
			void Load(RenderPlatform *r,const char *filename_utf8)
			{
				std::map<std::string,std::string> defines;
				Load(r,filename_utf8,defines);
			}
			// Which texture is at this slot. Warning: slow.
			std::string GetTextureForSlot(int s) const
			{
				for(auto i:textureDetailsMap)
				{
					if(i.second->slot==s)
						return i.first;
				}
				return std::string("Unknown");

			}
			EffectTechniqueGroup *GetTechniqueGroupByName(const char *name);
			virtual EffectTechnique *GetTechniqueByName(const char *name);
			virtual EffectTechnique *GetTechniqueByIndex(int index)				=0;
			//! Set the texture for read-write access by compute shaders in this effect.
			virtual void SetUnorderedAccessView(DeviceContext &deviceContext,const char *name,Texture *tex,int index=-1,int mip=-1)	;
			virtual ShaderResource GetShaderResource(const char *name)=0;
			//! Set the texture for read-write access by compute shaders in this effect.
			virtual void SetUnorderedAccessView(DeviceContext &deviceContext,ShaderResource &name,Texture *tex,int index=-1,int mip=-1)	;
			//! Set the texture for this effect. If mip is specified, the specific mipmap will be used, otherwise it's the full texture with all its mipmaps.
			virtual void SetTexture		(DeviceContext &deviceContext,ShaderResource &name	,Texture *tex,int array_idx=-1,int mip=-1);
			//! Set the texture for this effect. If mip is specified, the specific mipmap will be used, otherwise it's the full texture with all its mipmaps.
			virtual void SetTexture		(DeviceContext &deviceContext,const char *name	,Texture *tex,int array_idx=-1,int mip=-1);
			
			//! Set the texture for this effect.
			virtual void SetSamplerState(DeviceContext &deviceContext,const char *name	,SamplerState *s)=0;
			//! Set a constant buffer for this effect.
			virtual void SetConstantBuffer(DeviceContext &deviceContext,const char *name	,ConstantBufferBase *s)=0;
			//! Set a constant buffer for this effect.
			void SetConstantBuffer(crossplatform::DeviceContext &deviceContext,crossplatform::ConstantBufferBase *s)
			{
				SetConstantBuffer(deviceContext,s->GetDefaultName(),s);
			}
			/// Activate the shader. Unapply must be called after rendering is done.
			virtual void Apply(DeviceContext &deviceContext,const char *tech_name,const char *pass);
			/// Activate the shader. Unapply must be called after rendering is done.
			virtual void Apply(DeviceContext &deviceContext,const char *tech_name,int pass=0);
			/// Activate the shader. Unapply must be called after rendering is done.
			virtual void Apply(DeviceContext &deviceContext,EffectTechnique *effectTechnique,int pass=0);
			/// Activate the shader. Unapply must be called after rendering is done.
			virtual void Apply(DeviceContext &deviceContext,EffectTechnique *effectTechnique,const char *pass);
			/// Call Reapply between Apply and Unapply to apply the effect of modified constant buffers etc.
			virtual void Reapply(DeviceContext &deviceContext)=0;
			/// Deactivate the shader.
			virtual void Unapply(DeviceContext &deviceContext)=0;
			/// Zero-out the textures that are set for this shader. Call before apply.
			virtual void UnbindTextures(crossplatform::DeviceContext &deviceContext);

			void StoreConstantBufferLink(crossplatform::ConstantBufferBase *);
			bool IsLinkedToConstantBuffer(crossplatform::ConstantBufferBase*) const;
			
			int GetSlot(const char *name);
			int GetSamplerStateSlot(const char *name);
			int GetDimensions(const char *name);
			crossplatform::ShaderResourceType GetResourceType(const char *name);
		};
	}
}

#ifdef _MSC_VER
    #pragma warning(pop)
#endif
