// Copyright (c) 2007-2014 Simul Software Ltd
// All Rights Reserved.
//
// This source code is supplied under the terms of a license agreement or
// nondisclosure agreement with Simul Software Ltd and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.

// Framebuffer.cpp A renderer for skies, clouds and weather effects.
#define NOMINMAX
#include "FramebufferDX1x.h"

#include <tchar.h>
#ifndef SIMUL_WIN8_SDK
#include <dxerr.h>
#endif
#include <string>
#include <assert.h>

#include "Simul/Base/RuntimeError.h"
#include "Simul/Base/Timer.h"
#include "CreateEffectDX1x.h"
#include "MacrosDX1x.h"
#include "Utilities.h"
#include "Simul/Math/Pi.h"
#include "Simul/Platform/CrossPlatform/DeviceContext.h"
#include "Simul/Platform/DirectX11/RenderPlatform.h"
#pragma optimize("",off)
using namespace simul;
using namespace dx11;

// First figure out sizes, for placement create
UINT64 iCurrentESRAMOffset = 0; // We allow this to grow beyond ESRAM_SIZE
const UINT64 ESRAM_SIZE = 32 * 1024 * 1024;

Framebuffer::Framebuffer(const char *n) :
	BaseFramebuffer(n)
	,useESRAM(false)
	,useESRAMforDepth(false)
{
}

Framebuffer::~Framebuffer()
{
	InvalidateDeviceObjects();
}

void Framebuffer::RestoreDeviceObjects(crossplatform::RenderPlatform *r)
{
	BaseFramebuffer::RestoreDeviceObjects(r);
}

void Framebuffer::InvalidateDeviceObjects()
{
	BaseFramebuffer::InvalidateDeviceObjects();
}

void Framebuffer::ActivateColour(crossplatform::DeviceContext &deviceContext,const float viewportXYWH[4])
{
	ID3D11DeviceContext *pContext=(ID3D11DeviceContext *)deviceContext.asD3D11DeviceContext();
	if(!pContext)
		return;
	if((!buffer_texture||!buffer_texture->AsD3D11Texture2D())&&(!buffer_depth_texture||!buffer_depth_texture->AsD3D11Texture2D()))
		CreateBuffers();
	ID3D11RenderTargetView *renderTargetView=nullptr;
	if(buffer_texture->AsD3D11RenderTargetView())
	{
		colour_active=true;
		renderTargetView=buffer_texture->AsD3D11RenderTargetView();
		dx11::Texture *t=(dx11::Texture *)buffer_texture;
		if(is_cubemap)
			renderTargetView=t->AsD3D11RenderTargetView(current_face,0);
		pContext->OMSetRenderTargets(1,&renderTargetView,NULL);
	}
	SetViewport(deviceContext,viewportXYWH[0],viewportXYWH[1],viewportXYWH[2],viewportXYWH[3],0,1.f);
	targetsAndViewport.m_rt[0]=renderTargetView;
	targetsAndViewport.m_dt=nullptr;
	targetsAndViewport.viewport.x=targetsAndViewport.viewport.y=0;
	targetsAndViewport.viewport.w=Width;
	targetsAndViewport.viewport.h=Height;
	targetsAndViewport.viewport.zfar=1.0f;
	targetsAndViewport.viewport.znear=0.0f;
	targetsAndViewport.num=1;
	renderPlatform->PushRenderTargets(deviceContext,&targetsAndViewport);
}


void Framebuffer::MoveToFastRAM()
{
	if(useESRAM&&buffer_texture)
		buffer_texture->MoveToFastRAM();
	if(useESRAMforDepth&&buffer_depth_texture)
		buffer_depth_texture->MoveToFastRAM();
}

void Framebuffer::MoveToSlowRAM()
{
	if(useESRAM)
		buffer_texture->MoveToSlowRAM();
	if(useESRAMforDepth)
		buffer_depth_texture->MoveToSlowRAM();
}

void Framebuffer::MoveDepthToSlowRAM()
{
	if(useESRAMforDepth)
		buffer_depth_texture->MoveToSlowRAM();
}

void Framebuffer::ActivateViewport(crossplatform::DeviceContext &deviceContext, float viewportX, float viewportY, float viewportW, float viewportH)
{
	Activate(deviceContext);
	SetViewport(deviceContext,viewportX,viewportY,viewportW,viewportH,0,1.f);
}

void Framebuffer::Activate(crossplatform::DeviceContext &deviceContext)
{
	if((!buffer_texture||!buffer_texture->IsValid())&&(!buffer_depth_texture||!buffer_depth_texture->IsValid()))
		CreateBuffers();
	SIMUL_ASSERT(IsValid());
	ID3D11RenderTargetView *renderTargetView=NULL;
	dx11::Texture *t=(dx11::Texture *)buffer_texture;
	if(is_cubemap)
		renderTargetView=t->AsD3D11RenderTargetView(current_face);
	else
		renderTargetView=buffer_texture->AsD3D11RenderTargetView();
	if(renderTargetView)
	{
		colour_active=true;
		depth_active=buffer_depth_texture&&(buffer_depth_texture->AsD3D11DepthStencilView()!=NULL);
		deviceContext.asD3D11DeviceContext()->OMSetRenderTargets(1,&renderTargetView,buffer_depth_texture?buffer_depth_texture->AsD3D11DepthStencilView():NULL);
	}
	else if(buffer_depth_texture&&buffer_depth_texture->IsValid())
	{
		depth_active=buffer_depth_texture&&(buffer_depth_texture->AsD3D11DepthStencilView()!=NULL);
		deviceContext.asD3D11DeviceContext()->OMSetRenderTargets(0,nullptr,buffer_depth_texture?buffer_depth_texture->AsD3D11DepthStencilView():NULL);
	}
	else
	{
		SIMUL_BREAK_ONCE("No valid textures in framebuffer.");
		return;
	}
	SetViewport(deviceContext,0,0,1.f,1.f,0,1.f);
	targetsAndViewport.m_rt[0]=renderTargetView;
	targetsAndViewport.m_dt=nullptr;
	targetsAndViewport.viewport.x=targetsAndViewport.viewport.y=0;
	targetsAndViewport.viewport.w=Width;
	targetsAndViewport.viewport.h=Height;
	targetsAndViewport.viewport.zfar=1.0f;
	targetsAndViewport.viewport.znear=0.0f;
	targetsAndViewport.num=1;
	
	crossplatform::BaseFramebuffer::GetFrameBufferStack().push(&targetsAndViewport);
}

void Framebuffer::SetViewport(crossplatform::DeviceContext &deviceContext,float X,float Y,float W,float H,float Z,float D)
{
	D3D11_VIEWPORT viewport;
	viewport.Width = floorf((float)Width*W + 0.5f);
	viewport.Height = floorf((float)Height*H + 0.5f);
	viewport.TopLeftX = floorf((float)Width*X + 0.5f);
	viewport.TopLeftY = floorf((float)Height*Y+ 0.5f);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	deviceContext.asD3D11DeviceContext()->RSSetViewports(1, &viewport);
}

void Framebuffer::ActivateDepth(crossplatform::DeviceContext &deviceContext)
{
	ID3D11DeviceContext *pContext=(ID3D11DeviceContext *)deviceContext.asD3D11DeviceContext();
	if(!pContext)
		return;
	if((!buffer_texture||!buffer_texture->AsD3D11Texture2D())&&(!buffer_depth_texture||!buffer_depth_texture->AsD3D11Texture2D()))
		CreateBuffers();
	HRESULT hr=S_OK;
	
	ID3D11RenderTargetView *renderTargetView=buffer_texture->AsD3D11RenderTargetView();
//	if(m_pOldRenderTarget==NULL&&m_pOldDepthSurface==NULL)
	{
	//	pContext->RSGetViewports(&num_OldViewports,NULL);
	//	if(num_OldViewports>0)
	//		pContext->RSGetViewports(&num_OldViewports,m_OldViewports);
	//	pContext->OMGetRenderTargets(	1,
	//									&m_pOldRenderTarget,
	//									&m_pOldDepthSurface
	//									);
	//	pContext->OMSetRenderTargets(1,&m_pOldRenderTarget,buffer_depth_texture?buffer_depth_texture->AsD3D11DepthStencilView():NULL);
	}
	//else
	{
		pContext->OMSetRenderTargets(1,&renderTargetView,buffer_depth_texture?buffer_depth_texture->AsD3D11DepthStencilView():NULL);
	}
	depth_active=buffer_depth_texture&&(buffer_depth_texture->AsD3D11DepthStencilView()!=NULL);
	D3D11_VIEWPORT viewport;
		// Setup the viewport for rendering.
	viewport.Width = (float)Width;
	viewport.Height = (float)Height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.f;
	viewport.TopLeftY = 0.f;

	// Create the viewport.
	pContext->RSSetViewports(1, &viewport);
}

void Framebuffer::ActivateColour(crossplatform::DeviceContext &deviceContext)
{
	if((!buffer_texture||!buffer_texture->AsD3D11Texture2D())&&(!buffer_depth_texture||!buffer_depth_texture->AsD3D11Texture2D()))
		CreateBuffers();
	if(!buffer_texture->AsD3D11RenderTargetView())
		return;
	ID3D11RenderTargetView *renderTargetView=buffer_texture->AsD3D11RenderTargetView();
	deviceContext.asD3D11DeviceContext()->OMSetRenderTargets(1,&renderTargetView,NULL);
	colour_active=true;
	SetViewport(deviceContext,0,0,1.f,1.f,0,1.f);
	targetsAndViewport.m_rt[0]=renderTargetView;
	targetsAndViewport.m_dt=nullptr;
	targetsAndViewport.viewport.x=targetsAndViewport.viewport.y=0;
	targetsAndViewport.viewport.w=Width;
	targetsAndViewport.viewport.h=Height;
	targetsAndViewport.viewport.zfar=1.0f;
	targetsAndViewport.viewport.znear=0.0f;
	targetsAndViewport.num=1;
	renderPlatform->PushRenderTargets(deviceContext,&targetsAndViewport);
}

void Framebuffer::Deactivate(crossplatform::DeviceContext &deviceContext)
{
	ID3D11DeviceContext *pContext=(ID3D11DeviceContext *)deviceContext.asD3D11DeviceContext();
	renderPlatform->PopRenderTargets(deviceContext);
	if(GenerateMips)
		pContext->GenerateMips(buffer_texture->AsD3D11ShaderResourceView());
	colour_active=false;
	depth_active=false;
}

void Framebuffer::DeactivateDepth(crossplatform::DeviceContext &deviceContext)
{
	ID3D11DeviceContext *pContext=(ID3D11DeviceContext *)deviceContext.asD3D11DeviceContext();
if(!buffer_texture->AsD3D11RenderTargetView())
	{
		Deactivate(deviceContext);
		return;
	}
	ID3D11RenderTargetView *renderTargetView=buffer_texture->AsD3D11RenderTargetView();
		dx11::Texture *t=(dx11::Texture *)buffer_texture;
	if(is_cubemap)
		renderTargetView=t->AsD3D11RenderTargetView(current_face);
	pContext->OMSetRenderTargets(1,&renderTargetView,NULL);
	depth_active=false;
}

void Framebuffer::Clear(crossplatform::DeviceContext &deviceContext,float r,float g,float b,float a,float depth,int mask)
{
	if((!buffer_texture||!buffer_texture->IsValid())&&(!buffer_depth_texture||!buffer_depth_texture->IsValid()))
		CreateBuffers();
	ID3D11DeviceContext *pContext = deviceContext.asD3D11DeviceContext();
	// Clear the screen to black:
    float clearColor[4]={r,g,b,a};
    if(!mask)
		mask=D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL;
	if(is_cubemap&&buffer_texture)
	{
		dx11::Texture *t=(dx11::Texture *)buffer_texture;
		if(current_face>=0)
		{
			for(int i=0;i<t->GetMipCount();i++)
			{
				pContext->ClearRenderTargetView(t->AsD3D11RenderTargetView(current_face,i),clearColor);
			}
		}
		else
		{
			for(int i=0;i<6*t->arraySize;i++)
			{
				for(int j=0;j<t->GetMipCount();j++)
				{
					if(t->AsD3D11RenderTargetView(i,j))
						pContext->ClearRenderTargetView(t->AsD3D11RenderTargetView(i,j),clearColor);
				}
			}
		}
	}
	else
	{
		if(buffer_texture&&buffer_texture->AsD3D11RenderTargetView())
			pContext->ClearRenderTargetView(buffer_texture->AsD3D11RenderTargetView(),clearColor);
	}
	if(buffer_depth_texture&&buffer_depth_texture->AsD3D11DepthStencilView())
		pContext->ClearDepthStencilView(buffer_depth_texture->AsD3D11DepthStencilView(),mask,depth,0);
}

void Framebuffer::ClearDepth(crossplatform::DeviceContext &context,float depth)
{
	if(buffer_depth_texture->AsD3D11DepthStencilView())
		context.asD3D11DeviceContext()->ClearDepthStencilView(buffer_depth_texture->AsD3D11DepthStencilView(),D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,depth,0);
}

void Framebuffer::ClearColour(crossplatform::DeviceContext &deviceContext,float r,float g,float b,float a)
{
	float clearColor[4]={r,g,b,a};
	if(is_cubemap)
	{
		dx11::Texture *t=(dx11::Texture *)buffer_texture;
		for(int i=0;i<6;i++)
		{
			deviceContext.asD3D11DeviceContext()->ClearRenderTargetView(t->AsD3D11RenderTargetView(i), clearColor);
		}
	}
	else if(buffer_texture->AsD3D11RenderTargetView())
		deviceContext.asD3D11DeviceContext()->ClearRenderTargetView(buffer_texture->AsD3D11RenderTargetView(),clearColor);
}
