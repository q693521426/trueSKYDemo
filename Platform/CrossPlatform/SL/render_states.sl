//  Copyright (c) 2015 Simul Software Ltd. All rights reserved.
#ifndef RENDER_STATES_SL
#define RENDER_STATES_SL
#include "states.sl"
DepthStencilState DisableDepth
{
	DepthEnable = FALSE;
	DepthWriteMask = ZERO;
}; 

// We DO NOT here specify what kind of depth test to do - it depends on the projection matrix.
// So any shader that uses this MUST have code to set the depth state.
// Remember that for REVERSE_DEPTH projection we use DepthFunc = GREATER_EQUAL
// but for Forward Depth matrices we use DepthFunc = LESS_EQUAL
DepthStencilState TestDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ZERO;
};

DepthStencilState TestReverseDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ZERO;
	DepthFunc = GREATER_EQUAL;
};

DepthStencilState TestForwardDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ZERO;
	DepthFunc = LESS_EQUAL;
};

DepthStencilState WriteDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
	DepthFunc = ALWAYS;
};

DepthStencilState ReverseDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
	DepthFunc = GREATER_EQUAL;
};

DepthStencilState ForwardDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
	DepthFunc = LESS_EQUAL;
};

DepthStencilState EnableDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
};

BlendState MixBlend
{
	BlendEnable[0]	=TRUE;
	SrcBlend		=BLEND_FACTOR;
	DestBlend		=INV_BLEND_FACTOR;
};

BlendState DoBlend
{
	BlendEnable[0]	=TRUE;
	BlendEnable[1]	=TRUE;
	SrcBlend		=One;
	DestBlend		=INV_SRC_ALPHA;
};

BlendState AlphaToCoverageBlend
{
	BlendEnable[0]			=TRUE;
	BlendEnable[1]			=TRUE;
	AlphaToCoverageEnable	=TRUE;
	SrcBlend				=SRC_ALPHA;
	DestBlend				=INV_SRC_ALPHA;
};

BlendState CloudBlend
{
	BlendEnable[0]		= TRUE;
	BlendEnable[1]		= TRUE;
	SrcBlend			= SRC_ALPHA;
	DestBlend			= INV_SRC_ALPHA;
    BlendOp				= ADD;
    SrcBlendAlpha		= ZERO;
    DestBlendAlpha		= INV_SRC_ALPHA;
    BlendOpAlpha		= ADD;
    //RenderTargetWriteMask[0] = 0x0F;
};

BlendState AddDestInvAlphaBlend
{
	BlendEnable[0]	=TRUE;
	BlendEnable[1]	=TRUE;
	SrcBlend		=ONE;
	DestBlend		=INV_SRC_ALPHA;
    BlendOp			=ADD;
};

BlendState AlphaBlend
{
	BlendEnable[0]	=TRUE;
	BlendEnable[1]	=TRUE;
	SrcBlend		=SRC_ALPHA;
	DestBlend		=INV_SRC_ALPHA;
};

BlendState MultiplyBlend
{
	BlendEnable[0]	=TRUE;
	BlendEnable[1]	=TRUE;
	SrcBlend		=ZERO;
	DestBlend		=SRC_COLOR;
};

BlendState AddBlend
{
	BlendEnable[0]	=TRUE;
	BlendEnable[1]	=TRUE;
	SrcBlend		=ONE;
	DestBlend		=ONE;
};

BlendState AddAlphaBlend
{
	BlendEnable[0]	=TRUE;
	BlendEnable[1]	=TRUE;
	SrcBlend		=SRC_ALPHA;
	DestBlend		=ONE;
};

BlendState BlendWithoutWrite
{
	BlendEnable[0] = TRUE;
	BlendEnable[1] = TRUE;
	SrcBlend = ONE;
	DestBlend = ONE;
	RenderTargetWriteMask[0] = 0;
	RenderTargetWriteMask[1] = 0;
};

BlendState AddBlendDontWriteAlpha
{
	BlendEnable[0]	=TRUE;
	BlendEnable[1]	=TRUE;
	SrcBlend		=ONE;
	DestBlend		=ONE;
	RenderTargetWriteMask[0]=7;
	RenderTargetWriteMask[1]=7;
};

BlendState SubtractBlend
{
	BlendEnable[0]	=TRUE;
	BlendEnable[1]	=TRUE;
	BlendOp			=SUBTRACT;
	SrcBlend		=ONE;
	DestBlend		=ONE;
};

RasterizerState RenderNoCull
{
	FillMode					= SOLID;
	CullMode = none;
};

RasterizerState RenderFrontfaceCull
{
	FillMode					= SOLID;
	CullMode = front;
};

RasterizerState RenderBackfaceCull
{
	FillMode					= SOLID;
	CullMode = back;
};
#define DEPTH_BIAS_D32_FLOAT(d) (d/(1/pow(2,23)))
RasterizerState wireframeRasterizer
{
	FillMode					= WIREFRAME;
	CullMode					= none;
	FrontCounterClockwise		= false;
	DepthBias					= 0;//DEPTH_BIAS_D32_FLOAT(-0.00001);
	DepthBiasClamp				= 0.0;
	SlopeScaledDepthBias		= 0.0;
	DepthClipEnable				= false;
	ScissorEnable				= false;
	MultisampleEnable			= false;
	AntialiasedLineEnable		= true;
};


BlendState DontBlend
{
	BlendEnable[0] = FALSE;
	BlendEnable[1]	=FALSE;
};

BlendState NoBlend
{
	BlendEnable[0] = FALSE;
};


#endif
