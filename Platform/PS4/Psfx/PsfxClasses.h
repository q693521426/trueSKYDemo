/* Copyright (c) 2011, Max Aizenshtein <max.sniffer@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */


#pragma once

#include <map>
#include <string>
#include <sstream>
#include <vector>

int glfxparse();
int glfxlex();

namespace psfxParser
{
	using namespace std;
	/// Values that represent ShaderType.
	enum ShaderType
	{
		VERTEX_SHADER,
		TESSELATION_CONTROL_SHADER,		//= Hull shader
		TESSELATION_EVALUATION_SHADER,	//= Domain Shader
		GEOMETRY_SHADER,
		FRAGMENT_SHADER,
		COMPUTE_SHADER,
		EXPORT_SHADER,
		NUM_SHADER_TYPES
	};
	enum RenderStateType
	{
		RASTERIZER_STATE
		,DEPTHSTENCIL_STATE
		,BLEND_STATE
		,SAMPLER_STATE			// One of these things is not like the others...
		,NUM_RENDERSTATE_TYPES
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
		BLEND_OP_NONE		//opaque
		,BLEND_OP_ADD
		,BLEND_OP_SUBTRACT
		,BLEND_OP_MAX
		,BLEND_OP_MIN
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
	} ;
	enum PixelOutputFormat
	{
		FMT_UNKNOWN
		,FMT_32_GR
		,FMT_32_AR 
		,FMT_FP16_ABGR 
		,FMT_UNORM16_ABGR 
		,FMT_SNORM16_ABGR 
		,FMT_UINT16_ABGR 
		,FMT_SINT16_ABGR 
		,FMT_32_ABGR 
		,OUTPUT_FORMAT_COUNT
	};
	enum ShaderCommand
	{
		SetVertexShader		//VS Vertex Shader			|	Vertex Shader
		,SetHullShader		//TC Tessellation Control	|	Hull Shader
		,SetDomainShader	//TE Tessellation Evaluation	|	Domain Shader
		,SetGeometryShader	//GS Geometry Shader			|	Geometry Shader
		,SetPixelShader		//FS Fragment Shader			|	Pixel Shader
		,SetComputeShader	//CS Compute Shader			|	Compute Shader
		,SetExportShader		// this is a PS4 thing. We will write Vertex shaders as export shaders when necessary.
		,NUM_OF_SHADER_TYPES
		,SetRasterizerState
		,SetDepthStencilState
		,SetBlendState
		,NUM_OF_SHADER_COMMANDS
	};
	struct BlendState
	{
		BlendState();
		BlendOption SrcBlend;			
		BlendOption DestBlend;			
		BlendOperation BlendOp;				
		BlendOption SrcBlendAlpha;		
		BlendOption DestBlendAlpha;		
		BlendOperation BlendOpAlpha;		
		bool AlphaToCoverageEnable;
		std::map<int,bool> BlendEnable;
		std::map<int,unsigned char> RenderTargetWriteMask;
	};
	enum FillMode
	{ 
		FILL_WIREFRAME
		,FILL_SOLID
		,FILL_POINT
	};
	enum CullMode
	{ 
		CULL_NONE
		,CULL_FRONT 
		,CULL_BACK  
	};
	enum FilterMode
	{ 
		MIN_MAG_MIP_LINEAR  
		,MIN_MAG_MIP_POINT
	};
	enum AddressMode
	{ 
		CLAMP,WRAP,MIRROR
	};
	struct DepthStencilState
	{
		DepthStencilState();
		bool DepthEnable;
		int DepthWriteMask;
		DepthComparison DepthFunc;
	};
	struct RasterizerState
	{
		RasterizerState();
		FillMode	fillMode;
		CullMode	cullMode;
		bool		FrontCounterClockwise;
		int			DepthBias;
		float		DepthBiasClamp;
		float		SlopeScaledDepthBias;
		bool		DepthClipEnable;
		bool		ScissorEnable;
		bool		MultisampleEnable;
		bool		AntialiasedLineEnable;
	};
	struct SamplerState
	{
		SamplerState()
		: register_number(-1)
		,Filter(MIN_MAG_MIP_LINEAR)
		,AddressU(WRAP)
		,AddressV(WRAP)
		,AddressW(WRAP)
		{
		}
		int register_number;
		FilterMode Filter;
		AddressMode AddressU;
		AddressMode AddressV;
		AddressMode AddressW;
	};
	struct PassRasterizerState
	{
		string objectName;
	};
	struct PassDepthStencilState
	{
		string objectName;
		int stencilRef;
	};
	struct PassBlendState
	{
		string objectName;
		float blendFactor[4];
		unsigned sampleMask;
	};
	struct PassState
	{
		PassState() //:transformFeedbackTopology(UNDEFINED_TOPOLOGY)
		{
		}
		//std::string depthStencilState;
		PassRasterizerState rasterizerState;
		PassDepthStencilState depthStencilState;
		PassBlendState blendState;
		//Topology transformFeedbackTopology;//POINTS, LINES, TRIANGLES
	};
}
#include "psfxProgram.h"
