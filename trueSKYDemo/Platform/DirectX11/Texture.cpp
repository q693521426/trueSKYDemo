#define NOMINMAX
#include "Texture.h"
#include "CreateEffectDX1x.h"
#include "Utilities.h"
#include "Simul/Base/RuntimeError.h"
#include "Simul/Platform/DirectX11/RenderPlatform.h"
#include "Simul/Platform/CrossPlatform/DeviceContext.h"
#include "Simul/Platform/CrossPlatform/BaseFramebuffer.h"
#include <string>
#include <algorithm>

using namespace simul;
using namespace dx11;

#pragma optimize("",off)

SamplerState::SamplerState(crossplatform::SamplerStateDesc *d)
	:m_pd3D11SamplerState(NULL)
{
}

SamplerState::~SamplerState()
{
	InvalidateDeviceObjects();
}

void SamplerState::InvalidateDeviceObjects()
{
	SAFE_RELEASE(m_pd3D11SamplerState);
}


Texture::Texture()
	:texture(NULL)
	,mainShaderResourceView(NULL)
	,arrayShaderResourceView(nullptr)
	,layerShaderResourceViews(NULL)
	,mainMipShaderResourceViews(NULL)
	,layerMipShaderResourceViews(NULL)
	,mipUnorderedAccessViews(NULL)
	,layerMipUnorderedAccessViews(NULL)
	,depthStencilView(NULL)
	,renderTargetViews(NULL)
	,stagingBuffer(NULL)
	,last_context(NULL)
{
	memset(&mapped,0,sizeof(mapped));
}


Texture::~Texture()
{
	InvalidateDeviceObjects();
}

void Texture::FreeRTVTables()
{
	if(renderTargetViews)
	{
		int total_num=cubemap?arraySize*6:arraySize;
		for(int i=0;i<total_num;i++)
		{
			for(int j=0;j<mips;j++)
			{
				SAFE_RELEASE(renderTargetViews[i][j]);
			}
			delete [] renderTargetViews[i];
		}
		delete [] renderTargetViews;
		renderTargetViews=NULL;
	}
}

void Texture::InitUAVTables(int l,int m)
{
	mipUnorderedAccessViews			=nullptr;
	if(m)
		mipUnorderedAccessViews		=new ID3D11UnorderedAccessView*[m];		// UAV's for whole texture at different mips.
	layerMipUnorderedAccessViews	=nullptr;
	if(l&&m)
	{
		layerMipUnorderedAccessViews	=new ID3D11UnorderedAccessView**[l];			
		for(int i=0;i<l;i++)
		{
			layerMipUnorderedAccessViews[i]=new ID3D11UnorderedAccessView*[m];	// UAV's for each layer at different mips.
		}
	}
}

void Texture::FreeUAVTables()
{
	if(mipUnorderedAccessViews)
	{
		for(int j=0;j<mips;j++)
		{
			SAFE_RELEASE(mipUnorderedAccessViews[j]);
		}
		delete [] mipUnorderedAccessViews;
	}
	mipUnorderedAccessViews=nullptr;
	if(layerMipUnorderedAccessViews)
	{
		int total_num=cubemap?arraySize*6:arraySize;
		for(int i=0;i<total_num;i++)
		{
			for(int j=0;j<mips;j++)
			{
				SAFE_RELEASE(layerMipUnorderedAccessViews[i][j]);
			}
			delete [] layerMipUnorderedAccessViews[i];
		}
		delete [] layerMipUnorderedAccessViews;
		layerMipUnorderedAccessViews=nullptr;
	}
}

void Texture::InitRTVTables(int l,int m)
{
	renderTargetViews=nullptr;
	
	renderTargetViews=new ID3D11RenderTargetView**[l];			// SRV's for each layer at different mips.
	for(int i=0;i<l;i++)
	{
		renderTargetViews[i]=new ID3D11RenderTargetView*[m];	// SRV's for each layer at different mips.
	}
}

void Texture::InvalidateDeviceObjects()
{
	FreeRTVTables();
	FreeUAVTables();
	FreeSRVTables();
	if(last_context&&mapped.pData)
	{
		last_context->Unmap(texture,0);
		memset(&mapped,0,sizeof(mapped));
	}
	SAFE_RELEASE(texture);
	SAFE_RELEASE(depthStencilView);
	SAFE_RELEASE(stagingBuffer);
	arraySize=0;
	mips=0;
}

// Load a texture file
void Texture::LoadFromFile(crossplatform::RenderPlatform *renderPlatform,const char *pFilePathUtf8)
{
	ERRNO_BREAK
	const std::vector<std::string> &pathsUtf8=renderPlatform->GetTexturePathsUtf8();
	InvalidateDeviceObjects();
	SAFE_RELEASE(mainShaderResourceView);
	SAFE_RELEASE(arrayShaderResourceView);
	mainShaderResourceView	=simul::dx11::LoadTexture(renderPlatform->AsD3D11Device(),pFilePathUtf8,pathsUtf8);
	SetDebugObjectName(texture,pFilePathUtf8);
}

void Texture::LoadTextureArray(crossplatform::RenderPlatform *r,const std::vector<std::string> &texture_files)
{
	const std::vector<std::string> &pathsUtf8=r->GetTexturePathsUtf8();
	InvalidateDeviceObjects();
	std::vector<ID3D11Texture2D *> textures;
	for(unsigned i=0;i<texture_files.size();i++)
	{
		textures.push_back(simul::dx11::LoadStagingTexture(r->AsD3D11Device(),texture_files[i].c_str(),pathsUtf8));
	}
	D3D11_TEXTURE2D_DESC desc;
	ID3D11DeviceContext *pContext=NULL;
	r->AsD3D11Device()->GetImmediateContext(&pContext);
	for(int i=0;i<(int)textures.size();i++)
	{
		if(!textures[i])
			return;
		textures[i]->GetDesc(&desc);
	}
	static int m=5;
	desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
	desc.Usage=D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags=0;
	desc.ArraySize=(UINT)textures.size();
	desc.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;
	desc.MipLevels=m;
	ID3D11Texture2D *tex;
	r->AsD3D11Device()->CreateTexture2D(&desc,NULL,&tex);
	texture=tex;
	if(tex)
	for(unsigned i=0;i<textures.size();i++)
	{
		// Copy the resource directly, no CPU mapping
		pContext->CopySubresourceRegion(
						tex
						,i*m
						,0
						,0
						,0
						,textures[i]
						,0
						,NULL
						);
	}
	void FreeSRVTables();
	void FreeRTVTables();
//	InitSRVTables(texture_files.size(),m);
	V_CHECK(r->AsD3D11Device()->CreateShaderResourceView(tex,NULL,&mainShaderResourceView));
	for(unsigned i=0;i<textures.size();i++)
	{
		SAFE_RELEASE(textures[i])
	}
	pContext->GenerateMips(mainShaderResourceView);
	SAFE_RELEASE(pContext)
	mips=m;
	arraySize=(int)texture_files.size();
}

bool Texture::IsValid() const
{
	return (mainShaderResourceView!=NULL);
}

ID3D11ShaderResourceView *Texture::AsD3D11ShaderResourceView(crossplatform::ShaderResourceType t,int index,int mip)
{
	if(mip>=mips)
		mip=mips-1;
#ifdef _DEBUG
	if(index>=arraySize)
	{
		SIMUL_BREAK_ONCE("AsD3D11UnorderedAccessView: mip or index out of range");
		return NULL;
	}
#endif
	bool no_array=!cubemap&&(arraySize<=1);
	if(mips<=1&&no_array||(index<0&&mip<0))
	{
		if(IsCubemap()&&t==crossplatform::ShaderResourceType::TEXTURE_2D_ARRAY)
			return arrayShaderResourceView;
		return mainShaderResourceView;
	}
	if(layerShaderResourceViews&&(mip<0||mips<=1))
	{
		if(index<0||no_array)
			return mainShaderResourceView;
		return layerShaderResourceViews[index];
	}
	if(mainMipShaderResourceViews&&(no_array||index<0))
		return mainMipShaderResourceViews[mip];
	if(layerMipShaderResourceViews)
		return layerMipShaderResourceViews[index][mip];
	
	return nullptr;
}

ID3D11UnorderedAccessView *Texture::AsD3D11UnorderedAccessView(int index,int mip)
{
	if(mip<0)
	{
		mip=0;
	}
	if(index<0)
	{
		if(mipUnorderedAccessViews)
			return mipUnorderedAccessViews[mip];		// UAV for the whole texture at various mips
		else index=0;
	}
	if(!layerMipUnorderedAccessViews)
		return NULL;
	return layerMipUnorderedAccessViews[index][mip];
}

void Texture::copyToMemory(crossplatform::DeviceContext &deviceContext,void *target,int start_texel,int num_texels)
{
	int byteSize=simul::dx11::ByteSizeOfFormatElement(dxgi_format);
	if(!stagingBuffer)
	{
		//Create a "Staging" Resource to actually copy data to-from the GPU buffer. 
		D3D11_TEXTURE3D_DESC stagingBufferDesc;

		stagingBufferDesc.Width			=width;
		stagingBufferDesc.Height		=length;
		stagingBufferDesc.Depth			=depth;
		stagingBufferDesc.Format		=dxgi_format;
		stagingBufferDesc.MipLevels		=1;
		stagingBufferDesc.Usage			=D3D11_USAGE_STAGING;
		stagingBufferDesc.BindFlags		=0;
		stagingBufferDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
		stagingBufferDesc.MiscFlags		=0;

		deviceContext.renderPlatform->AsD3D11Device()->CreateTexture3D(&stagingBufferDesc,NULL,(ID3D11Texture3D**)(&stagingBuffer));
	}
	deviceContext.asD3D11DeviceContext()->CopyResource(stagingBuffer,texture);
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	V_CHECK(deviceContext.asD3D11DeviceContext()->Map( stagingBuffer, 0, D3D11_MAP_READ, SIMUL_D3D11_MAP_FLAGS, &mappedResource));
	unsigned char *source = (unsigned char *)(mappedResource.pData);
	
	int expected_pitch=byteSize*width;
	int expected_depth_pitch=expected_pitch*length;
	char *dest=(char*)target;
	if(mappedResource.RowPitch==expected_pitch&&mappedResource.DepthPitch==expected_depth_pitch)
	{
		source+=start_texel*byteSize;
		dest+=start_texel*byteSize;
		memcpy(dest,source,num_texels*byteSize);
	}
	else
	{
		for(int z=0;z<depth;z++)
		{
			unsigned char *s=source;
			for(int y=0;y<length;y++)
			{
				memcpy(dest,source,width*byteSize);
				source		+=mappedResource.RowPitch;
				dest		+=width*byteSize;
			}
			source=s;
			source		+=mappedResource.DepthPitch;
		}
	}
	deviceContext.asD3D11DeviceContext()->Unmap( stagingBuffer, 0);
}

void Texture::setTexels(crossplatform::DeviceContext &deviceContext,const void *src,int texel_index,int num_texels)
{
	last_context=deviceContext.asD3D11DeviceContext();
#ifdef _XBOX_ONE
	
// block ME until all shader stages are done at EOP, i.e. GPU idling
((ID3D11DeviceContextX*)last_context)->InsertWaitUntilIdle(0);

// flush and invalidate all caches at PFP
((ID3D11DeviceContextX*)last_context)->FlushGpuCacheRange(  D3D11_FLUSH_TEXTURE_L1_INVALIDATE  |
		     D3D11_FLUSH_TEXTURE_L2_INVALIDATE |
		     D3D11_FLUSH_COLOR_BLOCK_INVALIDATE |
		     D3D11_FLUSH_DEPTH_BLOCK_INVALIDATE |
		     D3D11_FLUSH_KCACHE_INVALIDATE |
		     D3D11_FLUSH_ICACHE_INVALIDATE |
		     D3D11_FLUSH_ENGINE_PFP
				,nullptr,0);
#endif
	D3D11_MAP map_type=D3D11_MAP_WRITE_DISCARD;
/*	if(((dx11::RenderPlatform*)deviceContext.renderPlatform)->UsesFastSemantics())
		map_type=D3D11_MAP_WRITE;*/
	if(!mapped.pData)
		last_context->Map(texture,0,map_type,SIMUL_D3D11_MAP_FLAGS,&mapped);
	if(!mapped.pData)
	{
		SIMUL_CERR<<"Failed to set texels on texture "<<name.c_str()<<std::endl;
		return;
	}
	int byteSize=simul::dx11::ByteSizeOfFormatElement(dxgi_format);
	const unsigned char *source=(const unsigned char*)src;
	unsigned char *target=(unsigned char*)mapped.pData;
	int expected_pitch=byteSize*width;
	if(mapped.RowPitch==expected_pitch)
	{
		target+=texel_index*byteSize;
		memcpy(target,source,num_texels*byteSize);
	}
	else if(depth>1)
	{
		for(int z=0;z<depth;z++)
		{
			unsigned char *t=target;
			for(int r=0;r<length;r++)
			{
				memcpy(t,source,width*byteSize);
				t		+=mapped.RowPitch;
				source	+=width*byteSize;
			}
			target+=mapped.DepthPitch;
		}
	}
	else
	{
		int block	=mapped.RowPitch/byteSize;
		int row		=texel_index/width;
		int last_row=(texel_index+num_texels)/width;
		int col		=texel_index-row*width;
		target		+=row*block*byteSize;
		source		+=col*byteSize;
		int columns=std::min(num_texels,width-col);
		memcpy(target,source,columns*byteSize);
		source		+=columns*byteSize;
		target		+=block*byteSize;
		for(int r=row+1;r<last_row;r++)
		{
			memcpy(target,source,width*byteSize);
			target		+=block*byteSize;
			source		+=width*byteSize;
		}
		int end_columns=texel_index+num_texels-last_row*width;
		if(end_columns>0)
			memcpy(target,source,end_columns*byteSize);
	}
	if(texel_index+num_texels>=width*length)
	{
		last_context->Unmap(texture,0);
		memset(&mapped,0,sizeof(mapped));
	}
#ifdef _XBOX_ONE
	
// block ME until all shader stages are done at EOP, i.e. GPU idling
((ID3D11DeviceContextX*)last_context)->InsertWaitUntilIdle(0);

// flush and invalidate all caches at PFP
((ID3D11DeviceContextX*)last_context)->FlushGpuCacheRange(  D3D11_FLUSH_TEXTURE_L1_INVALIDATE  |
		     D3D11_FLUSH_TEXTURE_L2_INVALIDATE |
		     D3D11_FLUSH_COLOR_BLOCK_INVALIDATE |
		     D3D11_FLUSH_DEPTH_BLOCK_INVALIDATE |
		     D3D11_FLUSH_KCACHE_INVALIDATE |
		     D3D11_FLUSH_ICACHE_INVALIDATE |
		     D3D11_FLUSH_ENGINE_PFP
				,nullptr,0);
#endif
}

bool Texture::IsComputable() const
{
	return (mipUnorderedAccessViews!=nullptr||layerMipUnorderedAccessViews!=nullptr);
}

bool Texture::HasRenderTargets() const
{
	return (renderTargetViews!=nullptr);
}

void Texture::InitFromExternalTexture2D(crossplatform::RenderPlatform *renderPlatform,void *t,void *srv,bool make_rt)
{
	InitFromExternalD3D11Texture2D(renderPlatform,(ID3D11Texture2D*)t,(ID3D11ShaderResourceView*)srv,make_rt);
}

void Texture::InitFromExternalD3D11Texture2D(crossplatform::RenderPlatform *r,ID3D11Texture2D *t,ID3D11ShaderResourceView *srv,bool make_rt)
{
	// If it's the same as before, return.
	if ((texture == t && srv==mainShaderResourceView) && mainShaderResourceView != NULL && (make_rt ==( renderTargetViews != NULL)))
		return;
	// If it's the same texture, and we created our own srv, that's fine, return.
	if (texture!=NULL&&texture == t&&mainShaderResourceView != NULL&&srv == NULL)
		return;
	FreeSRVTables();
	renderPlatform=r;
	SAFE_RELEASE(mainShaderResourceView);
	SAFE_RELEASE(arrayShaderResourceView);
	SAFE_RELEASE(texture);
	texture=t;
	mainShaderResourceView=srv;
	if(mainShaderResourceView)
		mainShaderResourceView->AddRef();
	if(texture)
	{
		texture->AddRef();
		ID3D11Texture2D* ppd(NULL);
		D3D11_TEXTURE2D_DESC textureDesc;
		if(texture->QueryInterface( __uuidof(ID3D11Texture2D),(void**)&ppd)==S_OK)
		{
			ppd->GetDesc(&textureDesc);
			// ASSUME it's a cubemap if it's an array of six.
			if(textureDesc.ArraySize==6)
			{
				cubemap=(textureDesc.ArraySize==6);
				textureDesc.ArraySize=1;
			}
			dxgi_format=textureDesc.Format;
			pixelFormat=RenderPlatform::FromDxgiFormat(textureDesc.Format);
			width=textureDesc.Width;
			length=textureDesc.Height;
			if(!srv)
			{
				InitSRVTables(textureDesc.ArraySize,textureDesc.MipLevels);
				CreateSRVTables(textureDesc.ArraySize,textureDesc.MipLevels,cubemap,false,textureDesc.SampleDesc.Count>1);
				arraySize=textureDesc.ArraySize;
				mips=textureDesc.MipLevels;
			}
			depth=textureDesc.ArraySize;
			if(make_rt&&(textureDesc.BindFlags&D3D11_BIND_RENDER_TARGET))
			{
				FreeRTVTables();
				// Setup the description of the render target view.
				D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
				renderTargetViewDesc.Format = TypelessToSrvFormat(textureDesc.Format);
				InitRTVTables(textureDesc.ArraySize,textureDesc.MipLevels);

				arraySize=textureDesc.ArraySize;
				mips=textureDesc.MipLevels;
				if(renderTargetViews)
				{
					renderTargetViewDesc.ViewDimension		=(textureDesc.SampleDesc.Count)>1?D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				
					renderTargetViewDesc.Texture2DArray.FirstArraySlice		=0;
					renderTargetViewDesc.Texture2DArray.ArraySize			=1;
					for(int i=0;i<(int)textureDesc.ArraySize;i++)
					{
						renderTargetViewDesc.Texture2DArray.FirstArraySlice = i;
						for(int j=0;j<(int)textureDesc.MipLevels;j++)
						{
							renderTargetViewDesc.Texture2DArray.MipSlice			=j;
							V_CHECK(renderPlatform->AsD3D11Device()->CreateRenderTargetView(texture,&renderTargetViewDesc,&(renderTargetViews[i][j])));
						}
					}
				}
			}
		}
		else
		{
			SIMUL_BREAK_ONCE("Not a valid D3D Texture");
		}
		SAFE_RELEASE(ppd);
	}
	dim=2;
}

void Texture::InitFromExternalTexture3D(crossplatform::RenderPlatform *r,void *ta,void *srv,bool make_uav)
{
	// If it's the same as before, return.
	if ((texture == ta && srv==mainShaderResourceView) && mainShaderResourceView != NULL && (make_uav ==( mipUnorderedAccessViews != NULL)))
		return;
	// If it's the same texture, and we created our own srv, that's fine, return.
	if (texture!=NULL&&texture == ta&&mainShaderResourceView != NULL&&srv == NULL)
		return;
	FreeSRVTables();
	renderPlatform=r;
	SAFE_RELEASE(mainShaderResourceView);
	SAFE_RELEASE(arrayShaderResourceView);
	SAFE_RELEASE(texture);
	texture=(ID3D11Resource*)ta;
	mainShaderResourceView=(ID3D11ShaderResourceView*)srv;
	if(mainShaderResourceView)
		mainShaderResourceView->AddRef();
	if(texture)
	{
		texture->AddRef();
		ID3D11Texture3D* ppd3(NULL);
		D3D11_TEXTURE3D_DESC textureDesc3;
		if(texture->QueryInterface( __uuidof(ID3D11Texture3D),(void**)&ppd3)==S_OK)
		{
			ppd3->GetDesc(&textureDesc3);
			dxgi_format=TypelessToSrvFormat(textureDesc3.Format);
			pixelFormat=RenderPlatform::FromDxgiFormat(dxgi_format);
			width=textureDesc3.Width;
			length=textureDesc3.Height;
			if(!srv)
			{
				InitSRVTables(1,textureDesc3.MipLevels);
				CreateSRVTables(1,textureDesc3.MipLevels,false,true);
				arraySize=1;
				mips=textureDesc3.MipLevels;
			}
			depth=textureDesc3.Depth;
			FreeUAVTables();
			InitUAVTables(1,textureDesc3.MipLevels);// 1 layer, m mips.
	
			D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
			ZeroMemory(&uav_desc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
			uav_desc.Format					=dxgi_format;
			uav_desc.ViewDimension			=D3D11_UAV_DIMENSION_TEXTURE3D;
			uav_desc.Texture3D.MipSlice		=0;
			uav_desc.Texture3D.WSize		=textureDesc3.Depth;
			uav_desc.Texture3D.FirstWSlice	=0;
		
			if(mipUnorderedAccessViews)
			for(int i=0;i<textureDesc3.MipLevels;i++)
			{
				uav_desc.Texture3D.MipSlice=i;
				V_CHECK(r->AsD3D11Device()->CreateUnorderedAccessView(texture, &uav_desc, &mipUnorderedAccessViews[i]));
				uav_desc.Texture3D.WSize/=2;
			}
		
			uav_desc.Texture3D.WSize	= textureDesc3.Depth;
			if(layerMipUnorderedAccessViews)
			for(int i=0;i<textureDesc3.MipLevels;i++)
			{
				uav_desc.Texture3D.MipSlice=i;
				V_CHECK(r->AsD3D11Device()->CreateUnorderedAccessView(texture, &uav_desc, &layerMipUnorderedAccessViews[0][i]));
				uav_desc.Texture3D.WSize/=2;
			}
		}
		else
		{
			SIMUL_BREAK_ONCE("Not a valid D3D Texture");
		}
		SAFE_RELEASE(ppd3);
	}
	dim=3;
}
bool Texture::ensureTexture3DSizeAndFormat(crossplatform::RenderPlatform *r,int w,int l,int d,crossplatform::PixelFormat pf,bool computable,int m,bool rendertargets)
{
	pixelFormat = pf;
	DXGI_FORMAT f=dx11::RenderPlatform::ToDxgiFormat(pixelFormat);
	dim=3;
	D3D11_TEXTURE3D_DESC textureDesc;
	bool ok=true;
	if(texture)
	{
		ID3D11Texture3D* ppd(NULL);
		if(texture->QueryInterface( __uuidof(ID3D11Texture3D),(void**)&ppd)!=S_OK)
			ok=false;
		else
		{
			ppd->GetDesc(&textureDesc);
			if(textureDesc.Width!=w||textureDesc.Height!=l||textureDesc.Depth!=d||textureDesc.Format!=f||mips!=m)
				ok=false;
			if(computable!=((textureDesc.BindFlags&D3D11_BIND_UNORDERED_ACCESS)==D3D11_BIND_UNORDERED_ACCESS))
				ok=false;
			if(rendertargets!=((textureDesc.BindFlags&D3D11_BIND_RENDER_TARGET)==D3D11_BIND_RENDER_TARGET))
				ok=false;
		}
		SAFE_RELEASE(ppd);
	}
	else
		ok=false;
	bool changed=!ok;
	if(!ok)
	{
		renderPlatform=r;
		SIMUL_ASSERT(w > 0 && l > 0 && w <= 65536 && l <= 65536);
		InvalidateDeviceObjects();
		memset(&textureDesc,0,sizeof(textureDesc));
		textureDesc.Width			=width=w;
		textureDesc.Height			=length=l;
		textureDesc.Depth			=depth=d;
		textureDesc.Format			=dxgi_format=f;
		textureDesc.MipLevels		=m;
		D3D11_USAGE usage			=D3D11_USAGE_DYNAMIC;
		//if(((dx11::RenderPlatform*)renderPlatform)->UsesFastSemantics())
		//	usage=D3D11_USAGE_DEFAULT;
		textureDesc.Usage			=(computable|rendertargets)?D3D11_USAGE_DEFAULT:usage;
		textureDesc.BindFlags		=D3D11_BIND_SHADER_RESOURCE|(computable?D3D11_BIND_UNORDERED_ACCESS:0)|(rendertargets?D3D11_BIND_RENDER_TARGET:0);
		textureDesc.CPUAccessFlags	=(computable|rendertargets)?0:D3D11_CPU_ACCESS_WRITE;
		textureDesc.MiscFlags		=0;
		
		V_CHECK(r->AsD3D11Device()->CreateTexture3D(&textureDesc,0,(ID3D11Texture3D**)(&texture)));

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		ZeroMemory(&srv_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srv_desc.Format						= f;
		srv_desc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE3D;
		srv_desc.Texture3D.MipLevels		= m;
		srv_desc.Texture3D.MostDetailedMip	= 0;
		FreeSRVTables();
		InitSRVTables(1,m);
		V_CHECK(r->AsD3D11Device()->CreateShaderResourceView(texture,&srv_desc,&mainShaderResourceView));
		if(mainMipShaderResourceViews)
		for(int j=0;j<m;j++)
		{
			srv_desc.Texture3D.MipLevels=1;
			srv_desc.Texture3D.MostDetailedMip=j;
			V_CHECK(r->AsD3D11Device()->CreateShaderResourceView(texture, &srv_desc, &mainMipShaderResourceViews[j]));
		}
	}
	if(computable&&(!layerMipUnorderedAccessViews||!ok))
	{
		FreeUAVTables();
		InitUAVTables(1,m);// 1 layer, m mips.
		changed=true;
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
		ZeroMemory(&uav_desc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
		uav_desc.Format					=f;
		uav_desc.ViewDimension			=D3D11_UAV_DIMENSION_TEXTURE3D;
		uav_desc.Texture3D.MipSlice		=0;
		uav_desc.Texture3D.WSize		=d;
		uav_desc.Texture3D.FirstWSlice	=0;
		
		if(mipUnorderedAccessViews)
		for(int i=0;i<m;i++)
		{
			uav_desc.Texture3D.MipSlice=i;
			V_CHECK(r->AsD3D11Device()->CreateUnorderedAccessView(texture, &uav_desc, &mipUnorderedAccessViews[i]));
			uav_desc.Texture3D.WSize/=2;
		}
		
		uav_desc.Texture3D.WSize	= d;
		if(layerMipUnorderedAccessViews)
		for(int i=0;i<m;i++)
		{
			uav_desc.Texture3D.MipSlice=i;
			V_CHECK(r->AsD3D11Device()->CreateUnorderedAccessView(texture, &uav_desc, &layerMipUnorderedAccessViews[0][i]));
			uav_desc.Texture3D.WSize/=2;
		}
	}
	if(d<=8&&rendertargets&&(!renderTargetViews||!renderTargetViews[0]||!ok))
	{
		SIMUL_BREAK("Render targets for 3D textures are not currently supported.");
	/*	changed=true;
		FreeRTVTables();
		InitRTVTables(int l,int m);
		renderTargetViews=new ID3D11RenderTargetView*[num_rt];
		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		renderTargetViewDesc.Format				=f;
		renderTargetViewDesc.ViewDimension		=D3D11_RTV_DIMENSION_TEXTURE3D;
		renderTargetViewDesc.Texture2D.MipSlice	=0;
		renderTargetViewDesc.Texture3D.WSize	=1;
		for(int i=0;i<num_rt;i++)
		{
			// Setup the description of the render target view.
			renderTargetViewDesc.Texture3D.FirstWSlice=i;
			// Create the render target in DX11:
			V_CHECK(r->AsD3D11Device()->CreateRenderTargetView(texture,&renderTargetViewDesc,&(renderTargetViews[i])));
		}*/
	}
	mips=m;
	arraySize=1;
	return changed;
}

bool Texture::ensureTexture2DSizeAndFormat(crossplatform::RenderPlatform *r
												 ,int w,int l
												 ,crossplatform::PixelFormat f
												 ,bool computable,bool rendertarget,bool depthstencil
												 ,int num_samples,int aa_quality,bool )
{
	int m=1;
	renderPlatform=r;
	pixelFormat=f;
	dxgi_format=(DXGI_FORMAT)dx11::RenderPlatform::ToDxgiFormat(pixelFormat);
	DXGI_FORMAT texture2dFormat=dxgi_format;
	DXGI_FORMAT srvFormat=dxgi_format;
	if(texture2dFormat==DXGI_FORMAT_D32_FLOAT)
	{
		texture2dFormat	=DXGI_FORMAT_R32_TYPELESS;
		srvFormat		=DXGI_FORMAT_R32_FLOAT;
	}
	if(texture2dFormat==DXGI_FORMAT_D16_UNORM)
	{
		texture2dFormat	=DXGI_FORMAT_R16_TYPELESS;
		srvFormat		=DXGI_FORMAT_R16_UNORM;
	}
	if(texture2dFormat==DXGI_FORMAT_D24_UNORM_S8_UINT)
	{
		texture2dFormat	=DXGI_FORMAT_R24G8_TYPELESS ;
		srvFormat		=DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	dim=2;
	ID3D11Device *pd3dDevice=renderPlatform->AsD3D11Device();
	D3D11_TEXTURE2D_DESC textureDesc;
	bool ok=true;
	if(texture)
	{
		ID3D11Texture2D* ppd(NULL);
		if(texture->QueryInterface( __uuidof(ID3D11Texture2D),(void**)&ppd)!=S_OK)
			ok=false;
		else
		{
			ppd->GetDesc(&textureDesc);
			if(textureDesc.Width!=w||textureDesc.Height!=l||textureDesc.Format!=texture2dFormat)
				ok=false;
			if(computable!=((textureDesc.BindFlags&D3D11_BIND_UNORDERED_ACCESS)==D3D11_BIND_UNORDERED_ACCESS))
				ok=false;
			if(rendertarget!=((textureDesc.BindFlags&D3D11_BIND_RENDER_TARGET)==D3D11_BIND_RENDER_TARGET))
				ok=false;
		}
		SAFE_RELEASE(ppd);
	}
	else
		ok=false; 
	if(!ok)
	{
		InvalidateDeviceObjects();

		unsigned int numQualityLevels=0;
		while(numQualityLevels==0&&num_samples>1)
		{
			V_CHECK(renderPlatform->AsD3D11Device()->CheckMultisampleQualityLevels(
				texture2dFormat,
				num_samples,
				&numQualityLevels	));
			//if(aa_quality>=numQualityLevels)
			//	aa_quality=numQualityLevels-1;
			if(numQualityLevels==0)
				num_samples/=2;
		};

		memset(&textureDesc,0,sizeof(textureDesc));
		textureDesc.Width					=width=w;
		textureDesc.Height					=length=l;
		depth								=1;
		textureDesc.Format					=texture2dFormat;
		textureDesc.MipLevels				=m;
		textureDesc.ArraySize				=1;
		D3D11_USAGE usage					=D3D11_USAGE_DYNAMIC;
	//	if(((dx11::RenderPlatform*)renderPlatform)->UsesFastSemantics())
	//		usage							=D3D11_USAGE_DEFAULT;
		textureDesc.Usage					=(computable||rendertarget||depthstencil)?D3D11_USAGE_DEFAULT:usage;
		textureDesc.BindFlags				=D3D11_BIND_SHADER_RESOURCE|(computable?D3D11_BIND_UNORDERED_ACCESS:0)|(rendertarget?D3D11_BIND_RENDER_TARGET:0)|(depthstencil?D3D11_BIND_DEPTH_STENCIL:0);
		textureDesc.CPUAccessFlags			=(computable||rendertarget||depthstencil)?0:D3D11_CPU_ACCESS_WRITE;
		textureDesc.MiscFlags				=rendertarget?D3D11_RESOURCE_MISC_GENERATE_MIPS:0;
		textureDesc.SampleDesc.Count		=num_samples;
		textureDesc.SampleDesc.Quality		=aa_quality;
		
		V_CHECK(pd3dDevice->CreateTexture2D(&textureDesc,0,(ID3D11Texture2D**)(&texture)));

		SetDebugObjectName(texture,"dx11::Texture::ensureTexture2DSizeAndFormat");
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		ZeroMemory(&srv_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srv_desc.Format						=srvFormat;
		srv_desc.ViewDimension				=num_samples>1?D3D11_SRV_DIMENSION_TEXTURE2DMS:D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels		=m;
		srv_desc.Texture2D.MostDetailedMip	=0;
		SAFE_RELEASE(mainShaderResourceView);
		SAFE_RELEASE(arrayShaderResourceView);
		V_CHECK(pd3dDevice->CreateShaderResourceView(texture,&srv_desc,&mainShaderResourceView));
		SetDebugObjectName(mainShaderResourceView,"dx11::Texture::ensureTexture2DSizeAndFormat mainShaderResourceView");
	}
	if(computable&&(!layerMipUnorderedAccessViews||!ok))
	{
		FreeUAVTables();
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
		ZeroMemory(&uav_desc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
		uav_desc.Format						=texture2dFormat;
		uav_desc.ViewDimension				=D3D11_UAV_DIMENSION_TEXTURE2D;
		uav_desc.Texture2D.MipSlice			=0;
		
		if(m<1)
			m=1;
		InitUAVTables(1,m);
		if(mipUnorderedAccessViews)
		for(int i=0;i<m;i++)
		{
			uav_desc.Texture2D.MipSlice=i;
			V_CHECK(pd3dDevice->CreateUnorderedAccessView(texture, &uav_desc, &mipUnorderedAccessViews[i]));
			SetDebugObjectName(mipUnorderedAccessViews[i],"dx11::Texture::ensureTexture2DSizeAndFormat unorderedAccessView");
		}
		if(layerMipUnorderedAccessViews)
		for(int i=0;i<m;i++)
		{
			uav_desc.Texture2D.MipSlice=i;
			V_CHECK(pd3dDevice->CreateUnorderedAccessView(texture, &uav_desc, &layerMipUnorderedAccessViews[0][i]));
			SetDebugObjectName(layerMipUnorderedAccessViews[0][i],"dx11::Texture::ensureTexture2DSizeAndFormat unorderedAccessView");
		}
	}
	if(rendertarget&&(!renderTargetViews||!ok))
	{
		FreeRTVTables();
		InitRTVTables(1,m);
		// Setup the description of the render target view.
		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		renderTargetViewDesc.Format				=texture2dFormat;
		renderTargetViewDesc.ViewDimension		=num_samples>1?D3D11_RTV_DIMENSION_TEXTURE2DMS:D3D11_RTV_DIMENSION_TEXTURE2D;
		// Create the render target in DX11:
		if(renderTargetViews)
		for(int j=0;j<m;j++)
		{
			renderTargetViewDesc.Texture2D.MipSlice	=j;
			V_CHECK(pd3dDevice->CreateRenderTargetView(texture,&renderTargetViewDesc,&(renderTargetViews[0][j])));
			SetDebugObjectName((renderTargetViews[0][j]),"dx11::Texture::ensureTexture2DSizeAndFormat renderTargetView");
		}
	}
	if(depthstencil&&(!depthStencilView||!ok))
	{
		D3D11_TEX2D_DSV dsv;
		dsv.MipSlice=0;
		D3D11_DEPTH_STENCIL_VIEW_DESC depthDesc;
		depthDesc.ViewDimension		=num_samples>1?D3D11_DSV_DIMENSION_TEXTURE2DMS:D3D11_DSV_DIMENSION_TEXTURE2D;
		depthDesc.Format			=dxgi_format;
		depthDesc.Flags				=0;
		depthDesc.Texture2D			=dsv;
		SAFE_RELEASE(depthStencilView);
		V_CHECK(pd3dDevice->CreateDepthStencilView(texture,&depthDesc,&depthStencilView));
	}
	SetDebugObjectName(texture,"ensureTexture2DSizeAndFormat");
	mips=m;
	arraySize=1;
	return !ok;
}

bool Texture::ensureTextureArraySizeAndFormat(crossplatform::RenderPlatform *r,int w,int l,int num,int m,crossplatform::PixelFormat f,bool computable,bool rendertarget,bool cubemap)
{
	renderPlatform=r;

	int total_num			=cubemap?6*num:num;
	dxgi_format=(DXGI_FORMAT)dx11::RenderPlatform::ToDxgiFormat(pixelFormat);
	D3D11_TEXTURE2D_DESC textureDesc;
	bool ok=true;
	if(texture)
	{
		ID3D11Texture2D* ppd(NULL);
		if(texture->QueryInterface( __uuidof(ID3D11Texture2D),(void**)&ppd)!=S_OK)
			ok=false;
		else
		{
			ppd->GetDesc(&textureDesc);
			if(textureDesc.ArraySize!=total_num||textureDesc.MipLevels!=m||textureDesc.Width!=w||textureDesc.Height!=l||textureDesc.Format!=dxgi_format)
				ok=false;
			if(computable!=((textureDesc.BindFlags&D3D11_BIND_UNORDERED_ACCESS)==D3D11_BIND_UNORDERED_ACCESS))
				ok=false;
			if(rendertarget!=((textureDesc.BindFlags&D3D11_BIND_RENDER_TARGET)==D3D11_BIND_RENDER_TARGET))
				ok=false;
		}
		SAFE_RELEASE(ppd);
	}
	else
		ok=false; 
	if(ok)
		return false;

	pixelFormat=f;
	InvalidateDeviceObjects();
	dxgi_format=(DXGI_FORMAT)dx11::RenderPlatform::ToDxgiFormat(pixelFormat);
	D3D11_TEXTURE2D_DESC desc;

	width					=w;
	length					=l;
	depth					=1;
	dim						=2;
	desc.Width				=w;
	desc.Height				=l;
	desc.Format				=dxgi_format;
	desc.BindFlags			=D3D11_BIND_SHADER_RESOURCE|(computable?D3D11_BIND_UNORDERED_ACCESS:0)|(rendertarget?D3D11_BIND_RENDER_TARGET:0);
	desc.Usage				=D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags		=0;
	desc.ArraySize			=total_num;
	desc.MiscFlags			=(rendertarget?D3D11_RESOURCE_MISC_GENERATE_MIPS:0)|(cubemap?D3D11_RESOURCE_MISC_TEXTURECUBE:0);
	desc.MipLevels			=m;
	desc.SampleDesc.Count	=1;
	desc.SampleDesc.Quality	=0;
	ID3D11Texture2D *pArrayTexture;
	V_CHECK(renderPlatform->AsD3D11Device()->CreateTexture2D(&desc,NULL,&pArrayTexture));
	SAFE_RELEASE(texture);
	texture=pArrayTexture;
	
	FreeSRVTables();
	FreeRTVTables();
	FreeUAVTables();
	if(!texture)
		return false;
	InitSRVTables(total_num,m);

	
	CreateSRVTables(num,m,cubemap);
	
	if(computable)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
		ZeroMemory(&uav_desc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
		uav_desc.Format							=dxgi_format;
		uav_desc.ViewDimension					=D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		uav_desc.Texture2DArray.ArraySize		=total_num;
		uav_desc.Texture2DArray.FirstArraySlice	=0;
		InitUAVTables(total_num,m);
		if(mipUnorderedAccessViews)
		for(int i=0;i<m;i++)
		{
			uav_desc.Texture2DArray.MipSlice=i;
			V_CHECK(renderPlatform->AsD3D11Device()->CreateUnorderedAccessView(texture, &uav_desc, &mipUnorderedAccessViews[i]));
			SetDebugObjectName(mipUnorderedAccessViews[i],"dx11::Texture::ensureTexture2DSizeAndFormat unorderedAccessView");
		}
		if(layerMipUnorderedAccessViews)
		for(int i=0;i<total_num;i++)
		for(int j=0;j<m;j++)
		{
			uav_desc.Texture2DArray.FirstArraySlice=i;
			uav_desc.Texture2DArray.ArraySize=1;
			uav_desc.Texture2DArray.MipSlice=j;
			V_CHECK(renderPlatform->AsD3D11Device()->CreateUnorderedAccessView(texture, &uav_desc, &layerMipUnorderedAccessViews[i][j]));
			SetDebugObjectName(layerMipUnorderedAccessViews[i][j],"dx11::Texture::ensureTexture2DSizeAndFormat unorderedAccessView");
		}
	}

	if(rendertarget)
	{
		InitRTVTables(total_num, m);
		// Create the multi-face render target view
		D3D11_RENDER_TARGET_VIEW_DESC DescRT;
		DescRT.Format							=dxgi_format;
		DescRT.ViewDimension					=D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		DescRT.Texture2DArray.FirstArraySlice	=0;
		DescRT.Texture2DArray.ArraySize			=total_num;
		if(renderTargetViews)
		for(int i=0;i<total_num;i++)
		{
			DescRT.Texture2DArray.FirstArraySlice = i;
			DescRT.Texture2DArray.ArraySize = 1;
			for(int j=0;j<m;j++)
			{
				DescRT.Texture2DArray.MipSlice			=j;
				V_CHECK(renderPlatform->AsD3D11Device()->CreateRenderTargetView(pArrayTexture, &DescRT, &(renderTargetViews[i][j])));
			}
		}
	}
	SetDebugObjectName(texture,"ensureTextureArraySizeAndFormat");

	mips=m;
	arraySize=num;
	this->cubemap=cubemap;
	return true;
}

void Texture::CreateSRVTables(int num,int m,bool cubemap,bool volume,bool msaa)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	ZeroMemory( &SRVDesc, sizeof(SRVDesc) );

	if(!volume)
	{
		SRVDesc.Format						=TypelessToSrvFormat(dxgi_format);
		SRVDesc.ViewDimension				=D3D11_SRV_DIMENSION_TEXTURE2D;
		int total_num						=cubemap?6*num:num;
		if(cubemap)
		{
			if(num<=1)
			{
				SRVDesc.ViewDimension				=D3D11_SRV_DIMENSION_TEXTURECUBE;
				SRVDesc.TextureCube.MipLevels		=m;
				SRVDesc.TextureCube.MostDetailedMip =0;
			}
			else
			{
				SRVDesc.ViewDimension						=D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
				SRVDesc.TextureCubeArray.MipLevels			=m;
				SRVDesc.TextureCubeArray.MostDetailedMip	=0;
				SRVDesc.TextureCubeArray.First2DArrayFace	=0;
				SRVDesc.TextureCubeArray.NumCubes			=num;
			}
		}
		else if(num>1)
		{
			SRVDesc.ViewDimension					=D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			SRVDesc.Texture2DArray.ArraySize		=num;
			SRVDesc.Texture2DArray.FirstArraySlice	=0;		
			SRVDesc.Texture2DArray.MipLevels		=m;
			SRVDesc.Texture2DArray.MostDetailedMip	=0;
		}
		else if(msaa)
		{
			SRVDesc.ViewDimension					=D3D11_SRV_DIMENSION_TEXTURE2DMS;
		}
		else
		{ 
			SRVDesc.Texture2D.MipLevels				=m;
			SRVDesc.Texture2D.MostDetailedMip		 =0;
		}
		SAFE_RELEASE(mainShaderResourceView);
		V_CHECK(renderPlatform->AsD3D11Device()->CreateShaderResourceView(texture,&SRVDesc,&mainShaderResourceView));
	
		SAFE_RELEASE(arrayShaderResourceView);
		if(cubemap)
		{
			SRVDesc.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			SRVDesc.Texture2DArray.ArraySize=total_num;
			SRVDesc.Texture2DArray.FirstArraySlice=0;
			SRVDesc.Texture2DArray.MipLevels=m;
			SRVDesc.Texture2DArray.MostDetailedMip=0;
			V_CHECK(renderPlatform->AsD3D11Device()->CreateShaderResourceView(texture,&SRVDesc, &arrayShaderResourceView));
		}
		if(mainMipShaderResourceViews)
		for(int j=0;j<m;j++)
		{
			SRVDesc.Texture3D.MipLevels=1;
			SRVDesc.Texture3D.MostDetailedMip=j;
			V_CHECK(renderPlatform->AsD3D11Device()->CreateShaderResourceView(texture, &SRVDesc, &mainMipShaderResourceViews[j]));
		}
		if(layerShaderResourceViews||layerMipShaderResourceViews)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC face_srv_desc;
			ZeroMemory(&face_srv_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			face_srv_desc.Format					= SRVDesc.Format;
			face_srv_desc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			for(int i=0;i<total_num;i++)
			{
				face_srv_desc.Texture2DArray.ArraySize=1;
				face_srv_desc.Texture2DArray.FirstArraySlice=i;
				face_srv_desc.Texture2DArray.MipLevels=m;
				face_srv_desc.Texture2DArray.MostDetailedMip=0;
				if(layerShaderResourceViews)
					V_CHECK(renderPlatform->AsD3D11Device()->CreateShaderResourceView(texture, &face_srv_desc, &layerShaderResourceViews[i]));
				if(layerMipShaderResourceViews)
					for(int j=0;j<m;j++)
					{
						face_srv_desc.Texture2DArray.MipLevels=1;
						face_srv_desc.Texture2DArray.MostDetailedMip=j;
						V_CHECK(renderPlatform->AsD3D11Device()->CreateShaderResourceView(texture, &face_srv_desc, &layerMipShaderResourceViews[i][j]));
					}
			}
		}
	}
	else
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		ZeroMemory(&srv_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srv_desc.Format						= TypelessToSrvFormat(dxgi_format);
		srv_desc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE3D;
		srv_desc.Texture3D.MipLevels		= m;
		srv_desc.Texture3D.MostDetailedMip	= 0;
		V_CHECK(renderPlatform->AsD3D11Device()->CreateShaderResourceView(texture,&srv_desc,&mainShaderResourceView));
		if(mainMipShaderResourceViews)
		for(int j=0;j<m;j++)
		{
			srv_desc.Texture3D.MipLevels=1;
			srv_desc.Texture3D.MostDetailedMip=j;
			V_CHECK(renderPlatform->AsD3D11Device()->CreateShaderResourceView(texture, &srv_desc, &mainMipShaderResourceViews[j]));
		}
	}
}

void Texture::ensureTexture1DSizeAndFormat(ID3D11Device *pd3dDevice,int w,crossplatform::PixelFormat pf,bool computable)
{
	pixelFormat=pf;
	DXGI_FORMAT f=dx11::RenderPlatform::ToDxgiFormat(pixelFormat);
	dim=1;
	int m=1;
	D3D11_TEXTURE1D_DESC textureDesc;
	bool ok=true;
	if(texture)
	{
		ID3D11Texture1D* ppd(NULL);
		if(texture->QueryInterface( __uuidof(ID3D11Texture1D),(void**)&ppd)!=S_OK)
			ok=false;
		else
		{
			ppd->GetDesc(&textureDesc);
			if(textureDesc.Width!=w||textureDesc.Format!=f)
				ok=false;
			if(computable!=((textureDesc.BindFlags&D3D11_BIND_UNORDERED_ACCESS)==D3D11_BIND_UNORDERED_ACCESS))
				ok=false;
		}
		SAFE_RELEASE(ppd);
	}
	else
		ok=false;
	if(!ok)
	{
		InvalidateDeviceObjects();
		memset(&textureDesc,0,sizeof(textureDesc));
		textureDesc.Width			=width=w;
		length						=depth=1;
		textureDesc.Format			=dxgi_format=f;
		textureDesc.MipLevels		=m;
		textureDesc.ArraySize		=1;
		D3D11_USAGE usage					=D3D11_USAGE_DYNAMIC;
		//if(((dx11::RenderPlatform*)renderPlatform)->UsesFastSemantics())
		//	usage							=D3D11_USAGE_DEFAULT;
		textureDesc.Usage			=computable?D3D11_USAGE_DEFAULT:usage;
		textureDesc.BindFlags		=D3D11_BIND_SHADER_RESOURCE|(computable?D3D11_BIND_UNORDERED_ACCESS:0);
		textureDesc.CPUAccessFlags	=computable?0:D3D11_CPU_ACCESS_WRITE;
		textureDesc.MiscFlags		=0;
		
		V_CHECK(pd3dDevice->CreateTexture1D(&textureDesc,0,(ID3D11Texture1D**)(&texture)));

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		ZeroMemory(&srv_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srv_desc.Format						= f;
		srv_desc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE1D;
		srv_desc.Texture1D.MipLevels		= m;
		srv_desc.Texture1D.MostDetailedMip	= 0;
		V_CHECK(pd3dDevice->CreateShaderResourceView(texture,&srv_desc,&mainShaderResourceView));
		if(mainMipShaderResourceViews)
		for(int j=0;j<mips;j++)
		{
			srv_desc.Texture1D.MipLevels=1;
			srv_desc.Texture1D.MostDetailedMip=j;
			V_CHECK(renderPlatform->AsD3D11Device()->CreateShaderResourceView(texture, &srv_desc, &mainMipShaderResourceViews[j]));
		}
	}
	if(computable&&(!layerMipUnorderedAccessViews||!ok))
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
		ZeroMemory(&uav_desc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
		uav_desc.Format				= f;
		uav_desc.ViewDimension		= D3D11_UAV_DIMENSION_TEXTURE1D;
		uav_desc.Texture1D.MipSlice	= 0;
		
		ZeroMemory(&uav_desc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
		uav_desc.Format							=dxgi_format;
		uav_desc.ViewDimension					=D3D11_UAV_DIMENSION_TEXTURE1D;
		InitUAVTables(1,m);
		if(layerMipUnorderedAccessViews)
		for(int j=0;j<m;j++)
		{
			uav_desc.Texture1D.MipSlice=j;
			V_CHECK(renderPlatform->AsD3D11Device()->CreateUnorderedAccessView(texture, &uav_desc, &layerMipUnorderedAccessViews[0][j]));
			SetDebugObjectName(layerMipUnorderedAccessViews[0][j],"dx11::Texture::ensureTexture1DSizeAndFormat unorderedAccessView");
		}
	}
	mips=m;
}

void Texture::GenerateMips(crossplatform::DeviceContext &deviceContext)
{
	// We can't detect if this has worked or not.
	if(renderTargetViews&&*renderTargetViews)
		deviceContext.asD3D11DeviceContext()->GenerateMips(AsD3D11ShaderResourceView());
	else
		SIMUL_CERR<<"Can't use GenerateMips on texture "<<this<<" not created as rendertarget.\n";
}

void Texture::map(ID3D11DeviceContext *context)
{
	if(mapped.pData!=NULL)
		return;
	last_context=context;
	D3D11_MAP map_type=D3D11_MAP_WRITE_DISCARD;
	//if(((dx11::RenderPlatform*)renderPlatform)->UsesFastSemantics())
	///	map_type=D3D11_MAP_WRITE;
	last_context->Map(texture,0,map_type,((dx11::RenderPlatform*)renderPlatform)->GetMapFlags(),&mapped);
}

bool Texture::isMapped() const
{
	return (mapped.pData!=NULL);
}

void Texture::unmap()
{
	if(mapped.pData)
		last_context->Unmap(texture,0);
	mapped.pData=NULL;
	last_context=NULL;
}

vec4 Texture::GetTexel(crossplatform::DeviceContext &deviceContext,vec2 texCoords,bool wrap)
{
	if(!stagingBuffer)
	{
		//Create a "Staging" Resource to actually copy data to-from the GPU buffer. 
		D3D11_TEXTURE2D_DESC desc;
		memset(&desc,0,sizeof(desc));
		desc.Width			=1;
		desc.Height		=1;
		desc.ArraySize = 1;
		desc.Format		=dxgi_format;
		desc.Usage			=D3D11_USAGE_STAGING;
		desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
		desc.SampleDesc.Count=1;
		desc.SampleDesc.Quality=0;
		ID3D11Texture2D *tex;
		ID3D11Device *dev=deviceContext.renderPlatform->AsD3D11Device();
		deviceContext.renderPlatform->AsD3D11Device()->CreateTexture2D(&desc,NULL,&tex);
		stagingBuffer=tex;
	}
	if(wrap)
	{
	/*	int X=(int)(texCoords.x-0.5f);
		int Y=(int)(texCoords.y-0.5f);
		texCoords.x-=(float)X;
		texCoords.y-=(float)Y;*/
		float u,v;
		texCoords.x=modf(texCoords.x+1.0f,&u);
		texCoords.y=modf(texCoords.y+1.0f,&v);
		if(texCoords.x<0.0f)
			texCoords.x+=1.0f;
		if(texCoords.y<0.0f)
			texCoords.y+=1.0f;
		static bool flip=false;
		if(flip)
			texCoords.y=1.0f-texCoords.y;
	}
	else
	{
		texCoords.x=std::max(0.0f,texCoords.x);
		texCoords.y=std::max(0.0f,texCoords.y);
		texCoords.x=std::min(1.0f,texCoords.x);
		texCoords.y=std::min(1.0f,texCoords.y);
	}
	D3D11_BOX srcBox;
	srcBox.left		=(int)(texCoords.x*width);
	srcBox.top		=(int)(texCoords.y*length);
	
	srcBox.left		=std::max((int)0		,(int)srcBox.left);
	srcBox.top		=std::max((int)0		,(int)srcBox.top);
	srcBox.left		=std::min((int)srcBox.left		,(int)width-1);
	srcBox.top		=std::min((int)srcBox.top		,(int)length-1);
	srcBox.right	=srcBox.left + 1,width-1;
	srcBox.bottom	=srcBox.top + 1,length-1;

	srcBox.back		=1;
	srcBox.front	=0;
	void *pixel;
	if(!texture)
		return vec4(0,0,0,0);
	try
	{
		deviceContext.asD3D11DeviceContext()->CopySubresourceRegion(stagingBuffer, 0, 0, 0, 0, texture, 0, &srcBox);

		D3D11_MAPPED_SUBRESOURCE msr;
		deviceContext.asD3D11DeviceContext()->Map(stagingBuffer, 0, D3D11_MAP_READ,SIMUL_D3D11_MAP_FLAGS, &msr);
		pixel = msr.pData;
		// copy data
		deviceContext.asD3D11DeviceContext()->Unmap(stagingBuffer, 0);
	}
	catch(...)
	{
	if(!pixel)
		return vec4(0,0,0,0);
	}
	if(!pixel)
		return vec4(0,0,0,0);
	return vec4((const float*)pixel);
}

void Texture::activateRenderTarget(crossplatform::DeviceContext &deviceContext,int array_index,int mip)
{
	if(!deviceContext.asD3D11DeviceContext())
		return;
	last_context=deviceContext.asD3D11DeviceContext();
	if(array_index<0)
		array_index=0;
	if(mip<0)
		mip=0;
	if(mip>=mips)
		return;
	D3D11_VIEWPORT viewport;
	if(renderTargetViews)
	{
		last_context->OMSetRenderTargets(1,&(renderTargetViews[array_index][mip]),NULL);
		{
			viewport.Width		=(float)(std::max(1,(width>>mip)));
			viewport.Height		=(float)(std::max(1,(length>>mip)));
			viewport.TopLeftX	=0;
			viewport.TopLeftY	=0;
			viewport.MinDepth	=0.0f;
			viewport.MaxDepth	=1.0f;
			last_context->RSSetViewports(1, &viewport);
		}
	}

	targetsAndViewport.m_rt[0]=&renderTargetViews[array_index][mip];
	targetsAndViewport.m_dt=nullptr;
	targetsAndViewport.viewport.x=targetsAndViewport.viewport.y=0;
	targetsAndViewport.viewport.w=viewport.Width;
	targetsAndViewport.viewport.h=viewport.Height;
	targetsAndViewport.viewport.zfar=1.0f;
	targetsAndViewport.viewport.znear=0.0f;
	targetsAndViewport.num=1;
	
	crossplatform::BaseFramebuffer::GetFrameBufferStack().push(&targetsAndViewport);

}

void Texture::deactivateRenderTarget(crossplatform::DeviceContext &deviceContext)
{
	if(renderPlatform)
		renderPlatform->DeactivateRenderTargets(deviceContext);
	else
		crossplatform::BaseFramebuffer::GetFrameBufferStack().pop();
}

int Texture::GetSampleCount() const
{
	if(!mainShaderResourceView)
		return 0;
	bool msaa=false;
	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	mainShaderResourceView->GetDesc(&desc);
	msaa=(desc.ViewDimension==D3D11_SRV_DIMENSION_TEXTURE2DMS);
	if(!msaa)
		return 0;
	D3D11_TEXTURE2D_DESC t2d_desc;
	((ID3D11Texture2D*)texture)->GetDesc(&t2d_desc);
	return t2d_desc.SampleDesc.Count;
}

void Texture::InitSRVTables(int l,int m)
{
	if(l>1)
		layerShaderResourceViews=new ID3D11ShaderResourceView*[l];				// SRV's for each layer, including all mips
	else
		layerShaderResourceViews=nullptr;
	if(m>1)
		mainMipShaderResourceViews=new ID3D11ShaderResourceView*[m];			// SRV's for the whole texture at different mips.
	else
		mainMipShaderResourceViews=nullptr;
	if(l>1&&m>1)
	{
		layerMipShaderResourceViews=new ID3D11ShaderResourceView**[l];			// SRV's for each layer at different mips.
		for(int i=0;i<l;i++)
		{
			layerMipShaderResourceViews[i]=new ID3D11ShaderResourceView*[m];	// SRV's for each layer at different mips.
		}
	}
	else
		layerMipShaderResourceViews=nullptr;
}

void Texture::FreeSRVTables()
{
	SAFE_RELEASE(arrayShaderResourceView);
	SAFE_RELEASE(mainShaderResourceView);
	int total_num=cubemap?arraySize*6:arraySize;
	for(int i=0;i<total_num;i++)
	{
		if(layerShaderResourceViews)
			SAFE_RELEASE(layerShaderResourceViews[i]);				// SRV's for each layer, including all mips
		if(layerMipShaderResourceViews)
		{
			for(int j=0;j<mips;j++)
			{
				SAFE_RELEASE(layerMipShaderResourceViews[i][j]);	
			}
			delete [] layerMipShaderResourceViews[i];
		}
	}
	delete [] layerShaderResourceViews;
	layerShaderResourceViews=nullptr;
	delete [] layerMipShaderResourceViews;
	layerMipShaderResourceViews=nullptr;
	if(mainMipShaderResourceViews)
	{
		for(int j=0;j<mips;j++)
		{
			SAFE_RELEASE(mainMipShaderResourceViews[j]);	
		}
	}
	delete [] mainMipShaderResourceViews;
	mainMipShaderResourceViews=nullptr;
}