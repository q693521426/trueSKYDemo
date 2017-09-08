#define NOMINMAX
#include "Texture.h"
#include "Utilities.h"
#include "Simul/Platform/PS4/Render/RenderPlatform.h"
#include "Simul/Platform/PS4/Render/Utilities.h"
#include "Simul/Platform/PS4/Render/Framebuffer.h"
#include "Simul/Platform/CrossPlatform/DeviceContext.h"
#include "Simul/Base/RuntimeError.h"

#include <string>
#include <gnf.h>

using namespace simul;
using namespace orbis;
sce::Gnm::MipFilterMode toGnmMipFilterMode(crossplatform::SamplerStateDesc::Filtering f)
{
	switch(f)
	{
	case crossplatform::SamplerStateDesc::POINT:
		return sce::Gnm::kMipFilterModePoint;
	case crossplatform::SamplerStateDesc::LINEAR:
		return sce::Gnm::kMipFilterModeLinear;
	default:
		return sce::Gnm::kMipFilterModeNone;
	};
}

sce::Gnm::FilterMode toGnmMinFilterMode(crossplatform::SamplerStateDesc::Filtering f)
{
	switch(f)
	{
	case crossplatform::SamplerStateDesc::POINT:
		return sce::Gnm::kFilterModePoint;
	case crossplatform::SamplerStateDesc::LINEAR:
		return sce::Gnm::kFilterModeAnisoBilinear;
	default:
		return sce::Gnm::kFilterModePoint;
	};
}

sce::Gnm::ZFilterMode toGnmZFilterMode(crossplatform::SamplerStateDesc::Filtering f)
{
	switch(f)
	{
	case crossplatform::SamplerStateDesc::POINT:
		return sce::Gnm::kZFilterModePoint;
	case crossplatform::SamplerStateDesc::LINEAR:
		return sce::Gnm::kZFilterModeLinear;
	default:
		return sce::Gnm::kZFilterModeNone;
	};
}
sce::Gnm::WrapMode toGnmWrapMode(crossplatform::SamplerStateDesc::Wrapping w)
{
	switch(w)
	{
	case crossplatform::SamplerStateDesc::WRAP:
		return sce::Gnm::kWrapModeWrap;
	case crossplatform::SamplerStateDesc::CLAMP:
		return sce::Gnm::kWrapModeClampLastTexel;
	case crossplatform::SamplerStateDesc::MIRROR:
		return sce::Gnm::kWrapModeMirror;
	default:
		return sce::Gnm::kWrapModeWrap;
	};
}

SamplerState::SamplerState(crossplatform::SamplerStateDesc *desc)
{
	sampler.init();
	sampler.setMipFilterMode(toGnmMipFilterMode(desc->filtering));
	sampler.setXyFilterMode(toGnmMinFilterMode(desc->filtering),toGnmMinFilterMode(desc->filtering));
	sampler.setZFilterMode(toGnmZFilterMode(desc->filtering));
	sampler.setWrapMode(toGnmWrapMode(desc->x),toGnmWrapMode(desc->y),toGnmWrapMode(desc->z));
	default_slot=desc->slot;
}

SamplerState::~SamplerState()
{
	InvalidateDeviceObjects();
}

void SamplerState::InvalidateDeviceObjects()
{
}


	/** Reproduced from gnf_loader.cpp in Framework
	  * @brief Loads a GNF file header and verifies that header contains valid information
	  * @param outHeader Pointer to GNF header structure to be filled with this call
	  * @param gnfFile File pointer to read this data from
	  * @return Zero if successful; otherwise, a non-zero error code.
	  */
    bool loadGnfHeader(sce::Gnf::Header *outHeader, void *data)
	{
		if (outHeader == NULL || data == NULL)
		{
			return false;
		}
		outHeader->m_magicNumber     = 0;
		outHeader->m_contentsSize    = 0;
		memcpy(outHeader,data,sizeof(sce::Gnf::Header));
		if (outHeader->m_magicNumber != sce::Gnf::kMagic)
		{
			return false;
		}
		return true;
	}
    // content size is sizeof(sce::Gnf::Contents)+gnfContents->m_numTextures*sizeof(sce::Gnm::Texture)+ paddings which is a variable of: gnfContents->alignment
    uint32_t computeContentSize(const sce::Gnf::Contents *gnfContents)
    {
        // compute the size of used bytes
        uint32_t headerSize = sizeof(sce::Gnf::Header) + sizeof(sce::Gnf::Contents) + gnfContents->m_numTextures*sizeof(sce::Gnm::Texture);
        // add the paddings
        uint32_t align = 1<<gnfContents->m_alignment; // actual alignment
	    size_t mask = align-1;
        uint32_t missAligned = ( headerSize & mask ); // number of bytes after the alignemnet point
	    if(missAligned) // if none zero we have to add paddings
	    {
            headerSize += align- missAligned;
        }
        return headerSize-sizeof(sce::Gnf::Header);
    }
	bool readGnfContents(sce::Gnf::Contents *outContents, uint32_t contentsSizeInBytes)
	{
		if(outContents == NULL )
		{
			return false;
		}
		if(outContents->m_alignment>31)
		{
			// Framework::kGnfErrorAlignmentOutOfRange;
			return false;
		}
        if( outContents->m_version == 1 )
	    {
	        if( (outContents->m_numTextures*sizeof(sce::Gnm::Texture) + sizeof(sce::Gnf::Contents)) != contentsSizeInBytes )
	        {
		        //return Framework::kGnfErrorContentsSizeMismatch;
			return false;
	        }
	    }
		else
        {
            if( outContents->m_version != sce::Gnf::kVersion )
	        {
		       // return Framework::kGnfErrorVersionMismatch;
			return false;
	        }
			else
            {
                if( computeContentSize(outContents) != contentsSizeInBytes)
    		    {
					//return Framework::kGnfErrorContentsSizeMismatch;
					return false;
				}
            }
        }
        return true;
	}


void Texture::copyToMemory(crossplatform::DeviceContext &deviceContext,void *target,int start_texel,int texels)
{
}
#include "Simul/Base/StringFunctions.h"
void Texture::reallocateMemory(sce::Gnm::SizeAlign sa,const char *name)
{
	if(!allocator)
	{
		SIMUL_BREAK_ONCE("No allocator, can't allocate memory!")
		return;
	}
	if(sa.m_size>0&&(sizeAlign.m_align!=sa.m_align||sa.m_size>sizeAlign.m_size||!texture.getBaseAddress()))
	{
		if(texture.getBaseAddress())
			allocator->DeallocateVideoMemory(texture.getBaseAddress());
		std::string str=name;
		if(str.length()==0)
			str=base::QuickFormat("0x%16x",this);
		texture.setBaseAddress((uint8_t*)allocator->AllocateVideoMemoryTracked(sa.m_size,sa.m_align,name));
		sizeAlign=sa;
		memset(texture.getBaseAddress(),0x00,sizeAlign.m_size);
	}
}

void Texture::reallocateStagingMemory(uint32_t ls,sce::Gnm::SizeAlign sa,const char *name)
{
	if(sa.m_size>ls)
		ls=sa.m_size;
	if(ls>0&&(ls>stagingLinearSize||!linearStagingTexels))
	{
		allocator->Deallocate(linearStagingTexels);
		stagingLinearSize=ls;
		linearStagingTexels=(uint8_t*)allocator->AllocateTracked(stagingLinearSize,0,name);
	}
	if(sa.m_size>0&&(stagingSizeAlign.m_align!=sa.m_align||sa.m_size>stagingSizeAlign.m_size||!finalStagingTexels))
	{
		if(finalStagingTexels)
			allocator->DeallocateVideoMemory(finalStagingTexels);
		stagingSizeAlign				=sa;
		finalStagingTexels		=(uint8_t*)allocator->AllocateVideoMemoryTracked(stagingSizeAlign.m_size,stagingSizeAlign.m_align,name);
		memset(finalStagingTexels, 0x00,stagingSizeAlign.m_size);
	}
}

void Texture::AllocateRenderTargets(int a,int m)
{
	ClearRenderTargets();
	renderTargetViews=new sce::Gnm::RenderTarget*[a];
	for(int i=0;i<a;i++)
	{
		renderTargetViews[i]=new sce::Gnm::RenderTarget[m];
	}
}

void Texture::ClearRenderTargets()
{
	if(renderTargetViews)
	for(int i=0;i<arraySize;i++)
	{
		delete [] renderTargetViews[i];
	}
	delete [] renderTargetViews;
	renderTargetViews=nullptr;
}

void Texture::InvalidateDeviceObjects()
{
	if(allocator)
	{
		if(texture.getBaseAddress())
			allocator->DeallocateVideoMemory(texture.getBaseAddress());
		if(linearStagingTexels)
			allocator->Deallocate(linearStagingTexels);
		if(finalStagingTexels)
			allocator->DeallocateVideoMemory(finalStagingTexels);
			// Don't free UAV's because these are just pointers into the main texture memory.
		//	allocator->DeallocateVideoMemory(unorderedAccessViews[i].getBaseAddress());
	}
	texture.setBaseAddress(nullptr);
	linearStagingTexels	=nullptr;
	finalStagingTexels	=nullptr;
	allocator			=nullptr;
	pixelFormat			=simul::crossplatform::UNKNOWN;
	renderPlatform		=nullptr;
	
	ClearViews();
	ClearRenderTargets();
	computable=false;
	mips=arraySize=0;
}

bool Texture::ensureTexture2DSizeAndFormat(crossplatform::RenderPlatform *rp
										   ,int width_x,int length_y
										   ,crossplatform::PixelFormat f
										   ,bool computable,bool rendertarget,bool depthstencil
										   ,int num_samples,int aa_quality,bool wrap)
{
	int m=1;
	renderPlatform=rp;
	allocator=renderPlatform->GetMemoryInterface();
	if(!allocator)
	{
		SIMUL_BREAK("No memory allocator available");
	}
	sce::Gnm::DataFormat gnmFormat	=RenderPlatform::ToGnmFormat(f);
	sce::Gnm::ZFormat zformat		=RenderPlatform::ToGnmDepthFormat(f);
	bool format_different			=(pixelFormat!=f);
	width							=width_x;
	length							=length_y;
	depth							=1;
	dim								=2;
	targetsAndViewport.num			=1;
	targetsAndViewport.viewport.x	=0;
	targetsAndViewport.viewport.y	=0;
	targetsAndViewport.viewport.w	=width_x;
	targetsAndViewport.viewport.h	=length_y;
	bool result=false;
	if(!format_different&&texture.getBaseAddress()&&texture.getWidth()==width_x
		&&texture.getHeight()==length_y&&this->computable==computable)
		return false;
	sce::Gnm::ResourceHandle ownerHandle=((orbis::RenderPlatform*)renderPlatform)->GetResourceHandle();
	m_owner=sce::Gnm::registerOwner(&ownerHandle,name.c_str());

	this->computable=computable;
	targetsAndViewport.m_dt=NULL;
	targetsAndViewport.m_rt[0]=NULL;
	ClearViews();
	// Don't free views, they point to the same memory.
	ClearRenderTargets();
	AllocateViews(1,m);
	// Create the main view:
	targetsAndViewport.m_dt=&depthTarget;
	if (zformat != sce::Gnm::kZFormatInvalid)
	{
		sce::Gnm::SizeAlign shadowHtileSizeAlign;
		sce::Gnm::TileMode depthTileModeHint;
		sce::Gnm::DataFormat gnmDepthFormat=RenderPlatform::DepthToEquivalentGnmFormat(f);
		sce::GpuAddress::computeSurfaceTileMode(&depthTileModeHint,sce::GpuAddress::kSurfaceTypeDepthOnlyTarget,gnmDepthFormat, 1);
		sce::Gnm::SizeAlign sa = depthTarget.init(width_x, length_y, zformat, sce::Gnm::kStencilInvalid, depthTileModeHint, sce::Gnm::kNumFragments1, NULL, &shadowHtileSizeAlign);

		allocator->DeallocateVideoMemory(texture.getBaseAddress());
		texture.setBaseAddress((uint8_t*)allocator->AllocateVideoMemory(sa.m_size, sa.m_align));
		depthTarget.setZReadAddress(texture.getBaseAddress());
		depthTarget.setZWriteAddress(texture.getBaseAddress());
		depthTarget.setHtileAccelerationEnable(false);
	
		texture.initFromDepthRenderTarget(&depthTarget, true);
		texture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
	}
	else
	{
		/*
		class SCE_GNM_LIBRARY_EXPORT TextureSpec
		{
		public:
			void init(void);					///< Clears all fields to safe defaults. Call this before filling out the rest of the fields.
			Gnm::TextureType m_textureType;		///< The type of texture to create -- 1D, 2D, Cubemap Array, etc.
			uint32_t m_width;					///< The desired width, in pixels. The actual surface width may be padded to accommodate hardware restrictions. Valid range is [1..16384].
			uint32_t m_height;					///< The desired height, in pixels. The actual surface width may be padded to accommodate hardware restrictions. Valid range is [1..16384].
			uint32_t m_depth;					///< The desired depth (for 3D textures). Valid range is [1..8192]. For non-3D textures, set 1.
			uint32_t m_pitch;					///< The desired pitch in pixels. If this value is zero, the library will compute the minimum legal pitch for the surface given the restrictions
												///< imposed by other surface parameters; otherwise the provided pitch will be used, provided it also conforms to hardware restrictions. A non-zero
												///< pitch that does not conform to hardware restrictions will cause initialization to fail. The valid range is [0..16384], subject to hardware restrictions.
			uint32_t m_numMipLevels;			///< The expected number of MIP levels (including the base level). Must be in the range [1..16]. This must be set to 1 if <c><i>numFragments</i></c> is greater than <c>kNumFragments1</c>.
			uint32_t m_numSlices;				///< The desired number of array slices, for texture arrays. The actual number of slices may be padded to accommodate hardware restrictions. Valid range is [1..8192].
												///< If the texture type is <c>kTextureTypeCubemapArray</c>, this is the number of cubemaps in the array (not the total number of cube face surfaces, which would be <c>6*<i>m_numSlices</i></c>).
												///< The array slice count must be 1 if <c><i>m_textureType</i></c> is <c>kTextureType3d</c>.
			Gnm::DataFormat m_format;			///< The desired format for each texel. This format must be texture-compatible; see DataFormat::supportsTexture().
			Gnm::TileMode m_tileModeHint;		///< The desired tiling mode. The actual tiling mode by be different to accommodate hardware restrictions; use Texture::getTileMode() to determine the object's final tiling mode.
			Gnm::GpuMode m_minGpuMode;			///< The minimum GPU mode this surface should be supported on. This setting may affect surface sizes, memory layout, available features, and so on.
			Gnm::NumFragments m_numFragments;	///< The number of fragments per texel. For 3D textures, this must be set to kNumFragments1.
			Gnm::TextureInitFlags m_flags;		///< Used to enable additional Texture features.
		};
		
				int32_t status = init(&spec);
				if (status == SCE_GNM_OK)
					sa=getSizeAlign();
		*/
		sce::Gnm::SizeAlign sa = texture.initAs2d(width_x, length_y, 1, gnmFormat
			, sce::Gnm::kTileModeThin_1dThin/*kTileModeDisplay_LinearAligned*/, sce::Gnm::kNumFragments1);
		texture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
		uint32_t ls		=width_x*length_y*gnmFormat.getBytesPerElement();
		reallocateMemory(sa,name.c_str());
	}
	tp.initFromTexture(&texture,0,0);
	// Update tiling params
	tp.m_mipLevel		=0;
	tp.m_linearWidth	=width_x;
	tp.m_linearHeight	=length_y;
	tp.m_linearDepth	=1;
	tp.m_baseTiledPitch =0;
	memset(texture.getBaseAddress(), 0x00,sizeAlign.m_size);

	if(mainMipViews)
	{
		int w=texture.getWidth();
		int h=texture.getHeight();
		for(int i=0;i<m;i++)
		{
			uint64_t mipOffset = 0;
			uint64_t mipSize = 0;
			sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&mipOffset, &mipSize, &texture,i, 0);

			sce::Gnm::SizeAlign mipSA = mainMipViews[i].initAs2d(w, h, 1, gnmFormat, sce::Gnm::kTileModeThin_1dThin, sce::Gnm::kNumFragments1);
			unsigned char *layerMipAddr=(unsigned char *)texture.getBaseAddress()+mipOffset;
			mainMipViews[i].setBaseAddress(layerMipAddr);
			mainMipViews[i].setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
			w=std::max(1,w/2);
			h=std::max(1,h/2);
		}
	}
	if(layerViews)
	{
		int w=texture.getWidth();
		int h=texture.getHeight();
		for(int i=0;i<1;i++)
		{
			uint64_t mipOffset = 0;
			uint64_t mipSize = 0;
			sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&mipOffset, &mipSize, &texture,0, i);
			sce::Gnm::SizeAlign mipSA = layerViews[i].initAs2d(w, h, 1, gnmFormat, sce::Gnm::kTileModeThin_1dThin, sce::Gnm::kNumFragments1);
			unsigned char *layerMipAddr=(unsigned char *)texture.getBaseAddress()+mipOffset;
			layerViews[i].setBaseAddress(layerMipAddr);
			layerViews[i].setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
		}
	}
	if(layerMipViews)
	{
		for(int i=0;i<1;i++)
		{
			int w=texture.getWidth();
			int h=texture.getHeight();
			for(int j=0;j<m;j++)
			{
				uint64_t mipOffset = 0;
				uint64_t mipSize = 0;
				sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&mipOffset, &mipSize, &texture,j, i);
				sce::Gnm::SizeAlign mipSA = layerMipViews[i][j].initAs2d(w, h, 1, gnmFormat, sce::Gnm::kTileModeThin_1dThin, sce::Gnm::kNumFragments1);
				unsigned char *layerMipAddr=(unsigned char *)texture.getBaseAddress()+mipOffset;
				layerMipViews[i][j].setBaseAddress(layerMipAddr);
				layerMipViews[i][j].setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
				w=std::max(1,w/2);
				h=std::max(1,h/2);
			}
		}
	}
	// Rendertargets correspond to layerMipViews.
	if(rendertarget)
	{
		AllocateRenderTargets(1,m);
		for(int j=0;j<m;j++)
		{
			renderTargetViews[0][j].initFromTexture(&texture,j);
			sce::Gnm::registerResource(nullptr, m_owner, renderTargetViews[0][j].getBaseAddress(), sizeAlign.m_size, name.c_str()
				,sce::Gnm::kResourceTypeRenderTargetBaseAddress, 0);
		}
	}
	sce::Gnm::registerResource(nullptr, m_owner, texture.getBaseAddress(), sizeAlign.m_size, name.c_str()
		,sce::Gnm::kResourceTypeTextureBaseAddress, 0);

	result=true;
	
	mips=m;
	arraySize=1;
	pixelFormat						=f;
	return result;
}

bool Texture::ensureTextureArraySizeAndFormat(crossplatform::RenderPlatform *rp,int w,int l,int num,int m,crossplatform::PixelFormat f,bool computable,bool rendertarget,bool cubemap)
{
	if(renderPlatform==rp&&width==w&&length==l&&arraySize==num&&pixelFormat==f
		&&(renderTargetViews!=nullptr)==rendertarget&&this->cubemap==cubemap&&
		this->computable==computable&&this->mips==mips)
		return false;
	int size=std::min(l,w);
	if(m>16)
		m=16;
	if(m<1)
		m=1;
	while(m>1&&(1<<(m-1))>size)
	{
		m--;
	}
	int total_num			=cubemap?6*num:num;
	width=w;
	length=l;
	depth=1;
	pixelFormat=f;
	renderPlatform=rp;
	sce::Gnm::ResourceHandle ownerHandle=((orbis::RenderPlatform*)renderPlatform)->GetResourceHandle();
	m_owner=sce::Gnm::registerOwner(&ownerHandle,name.c_str());
	dim=2;
	this->cubemap=cubemap;
	this->computable=computable;
	ClearRenderTargets();
	ClearViews();
	AllocateViews(total_num,m);
	allocator=renderPlatform->GetMemoryInterface();
	sce::Gnm::ZFormat gnmdepthz_format	=RenderPlatform::ToGnmDepthFormat(f);
	sce::Gnm::DataFormat gnmFormat		=RenderPlatform::ToGnmFormat(pixelFormat);
	sce::Gnm::SizeAlign sa;
	//uint32_t ls							=width*length*depth*gnmFormat.getBytesPerElement();
	
	sce::Gnm::TileMode tileMode;
	using namespace sce::GpuAddress;
	if(gnmdepthz_format!=sce::Gnm::kZFormatInvalid)
	{
		sce::Gnm::SizeAlign shadowHtileSizeAlign;
		sce::GpuAddress::computeSurfaceTileMode(&tileMode,sce::GpuAddress::kSurfaceTypeDepthOnlyTarget,gnmFormat,1);
		sa					=depthTarget.init(width,length,gnmdepthz_format,sce::Gnm::kStencilInvalid,tileMode,sce::Gnm::kNumFragments1,NULL,&shadowHtileSizeAlign);
		texture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
		reallocateMemory(sa,name.c_str());
		depthTarget.setZReadAddress(	texture.getBaseAddress());
		depthTarget.setZWriteAddress(texture.getBaseAddress());

		depthTarget.setHtileAccelerationEnable(false);
	}
	else
	{
		SurfaceType surfaceType=kSurfaceTypeTextureFlat;
		if(cubemap)
		{
			if(computable)
				surfaceType=kSurfaceTypeRwTextureCubemap;
			else
				surfaceType=kSurfaceTypeTextureCubemap;
		}
		else
		{
			if(computable)
				surfaceType=kSurfaceTypeRwTextureFlat;
			else
			{
				if(rendertarget)
					surfaceType=kSurfaceTypeColorTarget;
				else
					surfaceType=kSurfaceTypeTextureFlat;
			}
		}
		sce::GpuAddress::computeSurfaceTileMode(&tileMode
			,surfaceType
			,gnmFormat, 1);

		//Not initAsCubemap... ?
	/*	if(cubemap&&num>1)
		{
			sa		=texture.initAsCubemapArray(width,length,num,m,gnmFormat
					,tileMode);
			texture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
			reallocateMemory(sa, name.c_str());
			cubemapArrayTexture.initAs2dArray(width,length,total_num,m,gnmFormat,tileMode,sce::Gnm::kNumFragments1,false);
			cubemapArrayTexture.setBaseAddress(texture.getBaseAddress());
			cubemapArrayTexture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
		}
		else*/
		if(cubemap)
		{
			if(num>1)
				sa		=texture.initAsCubemapArray(width,length,num,m,gnmFormat,tileMode);
			else
				sa		=texture.initAsCubemap(width,length,m,gnmFormat,tileMode);
			texture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
			reallocateMemory(sa, name.c_str());
			cubemapArrayTexture.initAs2dArray(width,length,total_num,m,gnmFormat,tileMode,sce::Gnm::kNumFragments1,false);
			cubemapArrayTexture.setBaseAddress(texture.getBaseAddress());
			cubemapArrayTexture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
		}
		else
		{
			sa		=texture.initAs2dArray(width,length,total_num,m,gnmFormat
					,tileMode
					,sce::Gnm::kNumFragments1,cubemap);
			texture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
			reallocateMemory(sa, name.c_str());
		}
	}
	CreateViews(total_num,m,rendertarget);
	mips=m;
	arraySize=num;
	return true;
}

void Texture::GenerateMips(crossplatform::DeviceContext &deviceContext)
{
}

bool Texture::ensureTexture3DSizeAndFormat(crossplatform::RenderPlatform *rp,int width_x,int length_y,int depth_z,crossplatform::PixelFormat f,bool computable,int m,bool rendertargets)
{
	sce::Gnm::DataFormat format=RenderPlatform::ToGnmFormat(f);
	if(texture.getDataFormat().m_asInt!=format.m_asInt||!texture.getBaseAddress()||texture.getWidth()!=width_x
		||texture.getHeight()!=length_y||texture.getDepth()!=depth_z||this->computable!=computable||this->mips!=mips)
	{
		renderPlatform=rp;
	sce::Gnm::ResourceHandle ownerHandle=((orbis::RenderPlatform*)renderPlatform)->GetResourceHandle();
	m_owner=sce::Gnm::registerOwner(&ownerHandle,name.c_str());
		allocator=renderPlatform->GetMemoryInterface();
		uint32_t ls		=width_x*length_y*depth_z*format.getBytesPerElement();
		targetsAndViewport.num=depth_z;
		memset(&targetsAndViewport,0,sizeof(targetsAndViewport));
		targetsAndViewport.viewport.w	=width_x;
		targetsAndViewport.viewport.h	=length_y;
		sce::Gnm::TileMode offScreenTileMode;
		sce::GpuAddress::computeSurfaceTileMode(&offScreenTileMode,sce::GpuAddress::kSurfaceTypeRwTextureVolume, format, 1);

		sce::Gnm::SizeAlign sa		=texture.initAs3d(width_x,length_y,depth_z,mips,format
												,rendertargets?sce::Gnm::kTileModeThin_1dThin:offScreenTileMode/*sce::Gnm::kTileModeThick_1dThick*/);//kTileModeDisplay_LinearAligned);
		texture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
		reallocateMemory(sa,name.c_str());
		tp.initFromTexture(&texture,0,0);
		// Update tiling params
		tp.m_mipLevel		=0;
		tp.m_linearWidth	=width_x;
		tp.m_linearHeight	=length_y;
		tp.m_linearDepth	=depth_z;
		tp.m_baseTiledPitch =0;
		memset(texture.getBaseAddress(), 0x00,sizeAlign.m_size);
		uint64_t mipOffset = 0;
		uint64_t mipSize = 0;
		sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&mipOffset, &mipSize, &texture,0, 0);
		
		ClearViews();
		// Don't free views, they point to the same memory.
		ClearRenderTargets();
		AllocateViews(1,m);

		width=width_x;
		length=length_y;
		depth=depth_z;
		pixelFormat=f;
		this->mips=m;
		dim=3;
		this->computable=computable;
		//this->mips=mips;
		CreateViews(1,m,false);
		if(rendertargets)
		{
			SIMUL_BREAK("not supported to create RenderTargets from 3D textures just now.");
		//	renderTarget.initFromTexture(&texture,0);
		//	for(int i=0;i<depth;i++)
		//		targetsAndViewport.m_rt[i]=&renderTarget;
		}
		arraySize=1;
		return true;
	}
	return false;
}


Texture::Texture(const char *n):
			crossplatform::Texture(n)
			,allocator(NULL)
			,linearStagingTexels(NULL)
			,finalStagingTexels(NULL)
			,sizeAlign(0,0)
			,stagingSizeAlign(0,0)
			,stagingLinearSize(0)
			,computable(false)
			,layerViews(NULL)
			,mainMipViews(NULL)
			,layerMipViews(NULL)
			,renderTargetViews(NULL)
{
	memset(&texture,0,sizeof(texture));
	memset(&depthTarget,0,sizeof(depthTarget));
}


Texture::~Texture()
{
	InvalidateDeviceObjects();
}

// Load a texture file
void Texture::LoadFromFile(crossplatform::RenderPlatform *r,const char *fileNameUtf8)
{
	renderPlatform=r;
	sce::Gnm::ResourceHandle ownerHandle=((orbis::RenderPlatform*)renderPlatform)->GetResourceHandle();
	m_owner=sce::Gnm::registerOwner(&ownerHandle,name.c_str());
	InvalidateDeviceObjects();
	allocator=r->GetMemoryInterface();
	int textureIndex=0;
	void *gnfData;
	unsigned bytes=0;
	// We can't load any image type but gnf, so replace the extension with .gnf and hope the file exists.
	std::string f=fileNameUtf8;
	size_t pos=f.find_last_of(".");
	if(pos<f.length())
	{
		f=f.replace(f.begin()+pos,f.end(),".gnf");
	}
	name=f;
	std::string filename=simul::base::FileLoader::GetFileLoader()->FindFileInPathStack(f.c_str(),r->GetTexturePathsUtf8());
	if(!filename.length())
	{
		SIMUL_CERR<<"Failed to find texture file: "<<f.c_str()<<std::endl;
		return;
	}
	base::FileLoader::GetFileLoader()->AcquireFileContents(gnfData,bytes,filename.c_str(),false);
	allocator->DeallocateVideoMemory(texture.getBaseAddress());
	texture.setBaseAddress(nullptr);
	sce::Gnf::Contents *gnfContents = NULL;
	do
	{
		if (gnfData == 0)
		{
			break;
		}
		sce::Gnf::Header header;
		bool ok= loadGnfHeader(&header,gnfData);
		if(!ok)
			break;
		unsigned char *d=(unsigned char *)gnfData;
		d				+=sizeof(sce::Gnf::Header);
		gnfContents		=(sce::Gnf::Contents *)d;
		ok				=readGnfContents(gnfContents,header.m_contentsSize);
		if(!ok)
		{
			break;
		}
		sce::Gnm::SizeAlign pixelsSa = getTexturePixelsSize(gnfContents, textureIndex);
		if(pixelsSa.m_size)
	        texture.setBaseAddress((uint8_t*) allocator->AllocateVideoMemoryTracked(pixelsSa.m_size,pixelsSa.m_align,name.c_str()));
		if( texture.getBaseAddress()==0 ) // memory allocation failed
		{
			//kGnfErrorOutOfMemory;
			break;
		}
		d				+=header.m_contentsSize + getTexturePixelsByteOffset(gnfContents, textureIndex);
		memcpy(texture.getBaseAddress(),d,pixelsSa.m_size);
		void *finalTexels=texture.getBaseAddress();
		texture			=*patchTextures(gnfContents,textureIndex,1,(void**) &finalTexels);
		width=texture.getWidth();
		length=texture.getHeight();
		depth=1;
		arraySize=1;
		mips=1;
		finalTexels		=(uint8_t*)texture.getBaseAddress();
	}
	while(0);
	base::FileLoader::GetFileLoader()->ReleaseFileContents(gnfData);
}

void Texture::LoadTextureArray(crossplatform::RenderPlatform *r,const std::vector<std::string> &texture_files)
{
	renderPlatform=r;
}

bool Texture::IsValid() const
{
	return (texture.getBaseAddress()!=NULL);
}

sce::Gnm::Texture *Texture::AsGnmTexture(crossplatform::ShaderResourceType resourceType,int index,int mip)
{
#ifdef _DEBUG
	if(index>=arraySize||mip>=mips)
	{
		SIMUL_BREAK_ONCE("AsGnmTexture: mip or index out of range");
		return NULL;
	}
#endif
	bool no_array=!cubemap&&(arraySize<=1);
	if((mips<=1&&no_array)||(index<0&&mip<0))
	{
		if(cubemap&&((resourceType&crossplatform::ShaderResourceType::TEXTURE_2D_ARRAY)==crossplatform::ShaderResourceType::TEXTURE_2D_ARRAY))
			return &cubemapArrayTexture;
		return &texture;
	}
	if(layerViews&&(mip<0||mips<=1))
	{
		if(index<0||no_array)
			return &texture;
		return &layerViews[index];
	}
	if(mainMipViews&&index<0)
		return &mainMipViews[mip];
	if(layerMipViews)
		return &layerMipViews[index][mip];
	
	return nullptr;
}

void Texture::setTexels(crossplatform::DeviceContext &deviceContext,const void *src,int texel_index,int num_texels)
{
	sce::Gnm::DataFormat format=texture.getDataFormat();
	texture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);
	uint32_t bytes_per_texel=format.getBytesPerElement();

	if(texel_index==0&&num_texels==texture.getWidth()*texture.getHeight()*texture.getDepth())
	{
		// Tile into final texel buffer
		uint64_t mipOffset = 0;
		uint64_t mipSize = 0;
		sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&mipOffset,&mipSize,&texture,0,0);
		uint8_t *finalTexels=(unsigned char *)texture.getBaseAddress();
		sce::GpuAddress::tileSurface(finalTexels + mipOffset, src, &tp);
		return;
	}
	int pitch=texture.getPitch();
	int W	=texture.getWidth();
	int y	=texel_index/W;
	int y1	=(texel_index+num_texels)/W;
	int x	=texel_index-y*W;
	int x1	=texel_index+num_texels-y1*W;
	unsigned char *dest=(unsigned char *)texture.getBaseAddress();
	dest+=bytes_per_texel*y*pitch;
	dest+=bytes_per_texel*x;
	unsigned char *sourc=(unsigned char *)src;
	int w0	=std::min((W-x),num_texels);

	memcpy(dest,sourc,w0*bytes_per_texel);
	dest	+=(w0+pitch-W)*bytes_per_texel;
	sourc	+=w0*bytes_per_texel;

	for(;y<y1-1;y++)
	{
		memcpy(dest,sourc,W*bytes_per_texel);
		dest+=pitch*bytes_per_texel;
		sourc+=W*bytes_per_texel;
	}
	if(x1>0)
	{
		memcpy(dest,src,x1*bytes_per_texel);
	}
}


void Texture::InitFromExternalGnmTexture(sce::Gnm::Texture *t,bool make_rt)
{
	if(!t)
		return;
	if(length!=0&&texture.getBaseAddress()==t->getBaseAddress())
		return;
	sce::Gnm::ResourceHandle ownerHandle=((orbis::RenderPlatform*)renderPlatform)->GetResourceHandle();
	m_owner=sce::Gnm::registerOwner(&ownerHandle,name.c_str());
	// TODO: Only recreate if the texture or pointer has changed.
	texture=*t;
	dim=2;
	width=texture.getWidth();
	length=texture.getHeight();
	depth=texture.getDepth();
	pixelFormat=((orbis::RenderPlatform *)renderPlatform)->FromGnmFormat(texture.getDataFormat());
	targetsAndViewport.num=1;
	ClearRenderTargets();
	ClearViews();
	// TODO: We assume that this is a zero-based view for array/mip purposes.
	
	int a=t->getTotalArraySliceCount();
	cubemap=(t->getTextureType()==sce::Gnm::kTextureTypeCubemap);// also seems to cover cubemap arrays.
	int total_num=cubemap?6*a:a;
	int m=t->getLastMipLevel()-t->getBaseMipLevel()+1;
	CreateViews(total_num,m,make_rt);
	arraySize=a;
	mips=m;
}

void Texture::InitFromExternalTexture2D(crossplatform::RenderPlatform *rp,void *t,void *srv,bool make_rt)
{
	renderPlatform=rp;
	InitFromExternalGnmTexture((sce::Gnm::Texture*)t,make_rt);
}

void Texture::ensureTexture1DSizeAndFormat(sce::Gnmx::LightweightGfxContext *pd3dDevice,int w,crossplatform::PixelFormat pixelFormat,bool computable)
{
}

void Texture::activateRenderTarget(crossplatform::DeviceContext &deviceContext,int array_index,int mip_index)
{
	lastDeviceContext=&deviceContext;
	sce::Gnmx::LightweightGfxContext *gfxc=deviceContext.asGfxContext();
	sce::Gnm::ZFormat gnmdepthz_format=RenderPlatform::ToGnmDepthFormat(pixelFormat);
	sce::Gnm::DataFormat gnm_format=RenderPlatform::ToGnmFormat(pixelFormat);
	if(array_index<0)
		array_index=0;
	if(mip_index<0)
		mip_index=0;
	gfxc->setRenderTarget(0, &renderTargetViews[array_index][mip_index]);
	targetsAndViewport.m_rt[0]=&(renderTargetViews[array_index][mip_index]);
	targetsAndViewport.m_dt=&depthTarget;
	targetsAndViewport.viewport.x=targetsAndViewport.viewport.y=0;
	const sce::Gnm::RenderTarget *rt0=static_cast<const sce::Gnm::RenderTarget*>(targetsAndViewport.m_rt[0]);
	const sce::Gnm::DepthRenderTarget *dt=static_cast<const sce::Gnm::DepthRenderTarget*>(targetsAndViewport.m_dt);
	targetsAndViewport.viewport.w=rt0->getWidth();
	targetsAndViewport.viewport.h=rt0->getHeight();
	targetsAndViewport.viewport.zfar=1.0f;
	targetsAndViewport.viewport.znear=0.0f;
	if(gnmdepthz_format != sce::Gnm::kZFormatInvalid&&targetsAndViewport.m_dt!=NULL)
	{
		gfxc->setDepthRenderTarget(dt);
		if(!targetsAndViewport.m_dt)
			Utilities::DisableDepth(gfxc);
	}
	else
	{
		gfxc->setDepthRenderTarget(NULL);
		Utilities::DisableDepth(gfxc);
	}

	gfxc->setupScreenViewport(0,0,targetsAndViewport.viewport.w,targetsAndViewport.viewport.h,1.0f,0.0f);
	gfxc->setWindowOffset(0,0);
	gfxc->setWindowScissor(
                0,
                0,
				targetsAndViewport.viewport.w,
				targetsAndViewport.viewport.h,
                sce::Gnm::kWindowOffsetDisable
            );
	// Oh look, yet another Scissor command from Gnm:
	gfxc->setViewportScissor(
		0,
		0,
		0,
		targetsAndViewport.viewport.w,
		targetsAndViewport.viewport.h,sce::Gnm::kWindowOffsetDisable
	);
	/*
	You wouldn't normally have to worry about setViewportScissor() at all,
		it's just that UE4 uses it to only render to half the texture for each eye.
	In general, the Screen Scissor is the most global, is always active and is in
		absolute coordinates, the Window Scissor is an auxiliary rectangle in global
		or window-relative coordinates that can be offset via setWindowOffset(),
		and the Viewport Scissor is a further setting for individual viewports
		0 - 15 which are only enabled if setScanModeControl() enables them.
		The combination will always be the Boolean intersection of the Window and Viewport Scissors
		for that viewport if they're enabled, i.e. clipped to the smallest rect of those.
	I wouldn't worry too much about any overhead incurred. Calling these functions will roll the context but you should be able to batch up render targets of a similar size so you don't encounter any context stall problems.
	*/
	
	crossplatform::BaseFramebuffer::GetFrameBufferStack().push(&targetsAndViewport);
}

void Texture::deactivateRenderTarget(crossplatform::DeviceContext &deviceContext)
{
	orbis::RenderPlatform *rp=(orbis::RenderPlatform*)renderPlatform;
	if(rp&&deviceContext.asGfxContext())
		rp->DeactivateRenderTargets(deviceContext);
}

int Texture::GetSampleCount() const
{
	return 0;
}

bool Texture::IsComputable() const
{
	return (computable);
}

void Texture::SyncRenderTarget(crossplatform::DeviceContext &deviceContext,bool stall_cb) const
{
	sce::Gnmx::LightweightGfxContext *gfxc			=deviceContext.asGfxContext();
	uint32_t baseAddress				= renderTargetViews[0][0].getBaseAddress256ByteBlocks();
	uint32_t sizeIn256ByteUnits			= renderTargetViews[0][0].getSliceSizeInBytes()>>8;
	sce::Gnm::CacheAction cacheAction	= sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2;
	sce::Gnm::ExtendedCacheAction extendedCacheAction = sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache;

	//sce::Gnm::CacheAction cacheAction = sce::Gnm::kCacheActionNone;

	sce::Gnm::StallCommandBufferParserMode commandBufferStallMode 
			=stall_cb?sce::Gnm::kStallCommandBufferParserEnable:sce::Gnm::kStallCommandBufferParserDisable;
	gfxc->waitForGraphicsWrites(baseAddress, sizeIn256ByteUnits
		, sce::Gnm::kWaitTargetSlotCb0/*kWaitTargetSlotAll*/
		, cacheAction, extendedCacheAction, commandBufferStallMode);
	
	gfxc->waitForGraphicsWrites(baseAddress, sizeIn256ByteUnits,
				sce::Gnm::kWaitTargetSlotCb0
				,sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2
				,sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache
				,sce::Gnm::kStallCommandBufferParserDisable);
	//gfxc->waitForGraphicsWrites(baseAddress, sizeIn256ByteUnits, sce::Gnm::kWaitTargetSlotAll, cacheAction, extendedCacheAction, commandBufferStallMode);
}

void Texture::SyncRenderTargetToCompute(crossplatform::DeviceContext &deviceContext) const
{
	sce::Gnmx::LightweightGfxContext *gfxc			=deviceContext.asGfxContext();
	synchronizeRenderTargetGraphicsToCompute(gfxc,&renderTargetViews[0][0]);
}

void Texture::AllocateViews(int l,int m)
{
	if(l>=1&&m>=1)
	{
		layerMipViews=new sce::Gnm::Texture*[std::max(l,8)];		// SRV's for each layer at different mips.
		for(int i=0;i<l;i++)
		{
			layerMipViews[i]=new sce::Gnm::Texture[m];	// SRV's for each layer at different mips.
		}
	}
	else
		layerMipViews=nullptr;
	if(l>=1)
		layerViews=new sce::Gnm::Texture[l];			// SRV's for each layer, including all mips
	else
		layerViews=nullptr;
	if(m>1)
		mainMipViews=new sce::Gnm::Texture[m];			// SRV's for the whole texture at different mips.
	else
		mainMipViews=nullptr;
}

void Texture::ClearViews()
{
	for(int i=0;i<arraySize;i++)
	{
		if(layerMipViews)
		{
			delete [] layerMipViews[i];
		}
	}
	delete [] layerViews;
	layerViews=nullptr;
	delete [] layerMipViews;
	layerMipViews=nullptr;
	delete [] mainMipViews;
	mainMipViews=nullptr;
}
/*
		class SCE_GNM_LIBRARY_EXPORT TextureSpec
		{
		public:
			void init(void);					///< Clears all fields to safe defaults. Call this before filling out the rest of the fields.
			Gnm::TextureType m_textureType;		///< The type of texture to create -- 1D, 2D, Cubemap Array, etc.
			uint32_t m_width;					///< The desired width, in pixels. The actual surface width may be padded to accommodate hardware restrictions. Valid range is [1..16384].
			uint32_t m_height;					///< The desired height, in pixels. The actual surface width may be padded to accommodate hardware restrictions. Valid range is [1..16384].
			uint32_t m_depth;					///< The desired depth (for 3D textures). Valid range is [1..8192]. For non-3D textures, set 1.
			uint32_t m_pitch;					///< The desired pitch in pixels. If this value is zero, the library will compute the minimum legal pitch for the surface given the restrictions
												///< imposed by other surface parameters; otherwise the provided pitch will be used, provided it also conforms to hardware restrictions. A non-zero
												///< pitch that does not conform to hardware restrictions will cause initialization to fail. The valid range is [0..16384], subject to hardware restrictions.
			uint32_t m_numMipLevels;			///< The expected number of MIP levels (including the base level). Must be in the range [1..16]. This must be set to 1 if <c><i>numFragments</i></c> is greater than <c>kNumFragments1</c>.
			uint32_t m_numSlices;				///< The desired number of array slices, for texture arrays. The actual number of slices may be padded to accommodate hardware restrictions. Valid range is [1..8192].
												///< If the texture type is <c>kTextureTypeCubemapArray</c>, this is the number of cubemaps in the array (not the total number of cube face surfaces, which would be <c>6*<i>m_numSlices</i></c>).
												///< The array slice count must be 1 if <c><i>m_textureType</i></c> is <c>kTextureType3d</c>.
			Gnm::DataFormat m_format;			///< The desired format for each texel. This format must be texture-compatible; see DataFormat::supportsTexture().
			Gnm::TileMode m_tileModeHint;		///< The desired tiling mode. The actual tiling mode by be different to accommodate hardware restrictions; use Texture::getTileMode() to determine the object's final tiling mode.
			Gnm::GpuMode m_minGpuMode;			///< The minimum GPU mode this surface should be supported on. This setting may affect surface sizes, memory layout, available features, and so on.
			Gnm::NumFragments m_numFragments;	///< The number of fragments per texel. For 3D textures, this must be set to kNumFragments1.
			Gnm::TextureInitFlags m_flags;		///< Used to enable additional Texture features.
		};
		
				int32_t status = init(&spec);
				if (status == SCE_GNM_OK)
					sa=getSizeAlign();
*/
void Texture::CreateViews(int num,int m,bool rendertarget)
{
	sce::Gnm::TextureSpec textureSpec;
	textureSpec.init();
	textureSpec.m_textureType=texture.getTextureType();
	textureSpec.m_width=width;
	textureSpec.m_height=length;
	textureSpec.m_depth=depth;
	textureSpec.m_pitch=0;
	textureSpec.m_tileModeHint=texture.getTileMode();
	textureSpec.m_numFragments=sce::Gnm::kNumFragments1;
	textureSpec.m_format=texture.getDataFormat();
	if(mainMipViews)
	{
		int w=texture.getWidth();
		int h=texture.getHeight();
		int d=texture.getDepth();
		for(int i=0;i<m;i++)
		{
			uint64_t mipOffset = 0;
			uint64_t mipSize = 0;
			sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&mipOffset, &mipSize, &texture,i, 0);
			
			textureSpec.m_width=w;
			textureSpec.m_height=h;
			textureSpec.m_depth=d;
			textureSpec.m_format=texture.getDataFormat();
			
			int32_t status = mainMipViews[i].init(&textureSpec);
			if(status==SCE_GNM_OK)
			{
				sce::Gnm::SizeAlign mipSA = mainMipViews[i].getSizeAlign();
				//initAs2d(w, h, 1, gnmFormat, sce::Gnm::kTileModeThin_1dThin, sce::Gnm::kNumFragments1);
				unsigned char *layerMipAddr=(unsigned char *)texture.getBaseAddress()+mipOffset;
				mainMipViews[i].setBaseAddress(layerMipAddr);
				mainMipViews[i].setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);	
			}
			w=std::max(1,w/2);
			h=std::max(1,h/2);
			d=std::max(1,d/2);
		}
	}
	if(layerViews)
	{
		int w=texture.getWidth();
		int h=texture.getHeight();
		// Layers of a cubemap means faces.
		sce::Gnm::Texture *t = &texture;
		textureSpec.m_numSlices = 1;
		if (textureSpec.m_textureType != sce::Gnm::TextureType::kTextureType3d)
		{
			textureSpec.m_depth = 1;
		}
		if (textureSpec.m_textureType == sce::Gnm::TextureType::kTextureTypeCubemap&&arraySize==1)
		{
			textureSpec.m_textureType = sce::Gnm::TextureType::kTextureType2dArray;
			t=&cubemapArrayTexture;
		}
		for(int i=0;i<num;i++)
		{
			uint64_t mipOffset = 0;
			uint64_t mipSize = 0;
			sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&mipOffset, &mipSize, t,0, i);
			layerViews[i].init(&textureSpec);
			unsigned char *layerMipAddr=(unsigned char *)t->getBaseAddress()+mipOffset;
			layerViews[i].setBaseAddress(layerMipAddr);
			layerViews[i].setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);	
		}
	}
	if(layerMipViews)
	{
		textureSpec.m_numSlices = 1;
		textureSpec.m_numMipLevels = 1;
		if (textureSpec.m_textureType != sce::Gnm::TextureType::kTextureType3d)
		{
			textureSpec.m_depth = 1;
		}
		sce::Gnm::Texture *t = &texture;
		if (textureSpec.m_textureType == sce::Gnm::TextureType::kTextureTypeCubemap)
		{
			textureSpec.m_textureType = sce::Gnm::TextureType::kTextureType2dArray;
			t=&cubemapArrayTexture;
		}
		for(int i=0;i<num;i++)
		{
			int w=texture.getWidth();
			int h=texture.getHeight();
			int d=texture.getDepth();
			for(int j=0;j<m;j++)
			{
				uint64_t mipOffset = 0;
				uint64_t mipSize = 0;
				sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&mipOffset, &mipSize, t,j, i);
				
				textureSpec.m_width=w;
				textureSpec.m_height=h;
			//	textureSpec.m_depth=d;
				textureSpec.m_format=texture.getDataFormat();
			
				int32_t status = layerMipViews[i][j].init(&textureSpec);
				if(status==SCE_GNM_OK)
				{
					sce::Gnm::SizeAlign mipSA = layerMipViews[i][j].getSizeAlign();

					unsigned char *layerMipAddr=(unsigned char *)texture.getBaseAddress()+mipOffset;
					layerMipViews[i][j].setBaseAddress(layerMipAddr);
					layerMipViews[i][j].setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC);	
				}
				else
				{
					SIMUL_CERR << "Failed to create layer mip view of texture." << std::endl;
				}
				w=std::max(1,w/2);
				h=std::max(1,h/2);
				d=std::max(1,d/2);
			}
		}
	}
	// Rendertargets correspond to layerMipViews, unless there's only one
	if(rendertarget)
	{
		AllocateRenderTargets(num,m);
		for(int i=0;i<num;i++)
		{
			for(int j=0;j<m;j++)
			{
				sce::Gnm::Texture *t=&texture;
				if(layerMipViews)
					t=&layerMipViews[i][j];
				else if(mainMipViews&&num==1)
					t=&mainMipViews[j];
				else if(layerViews&&m==1)
					t=&layerViews[i];
				renderTargetViews[i][j].initFromTexture(t,0);
			}
		}
	}
}