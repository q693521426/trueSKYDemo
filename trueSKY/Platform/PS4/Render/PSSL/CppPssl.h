#ifndef CPP_PSSL
#define CPP_PSSL

#define REVERSE_DEPTH 1
#define STATIC static
#ifndef __cplusplus
#refine PRAGMA_FINAL_FORMAT PSSL_target_output_format (target 0 FMT_FP16_ABGR)
#define PRAGMA_32BIT_FORMAT PSSL_target_output_format (target 0 FMT_32_ABGR) // Ste Taylor fix
// Specify the target on a per-shader basis.
#endif
#include "../../../CrossPlatform/SL/CppSl.hs"
#define fract frac

#define adaptY(texel) texel.z
#define layout(a)

#define groupshared thread_group_memory
#define GroupMemoryBarrier ThreadGroupMemoryBarrier
#define GroupMemoryBarrierWithGroupSync ThreadGroupMemoryBarrierSync 

#define numthreads NUM_THREADS
#define SV_DispatchThreadID S_DISPATCH_THREAD_ID
#define SV_GroupThreadID S_GROUP_THREAD_ID
#define SV_GroupID S_GROUP_ID
#define SV_Target S_TARGET_OUTPUT

#define SV_DISPATCHTHREADID S_DISPATCH_THREAD_ID
#define SV_GROUPTHREADID S_GROUP_THREAD_ID
#define SV_GROUPID S_GROUP_ID
#define SV_TARGET S_TARGET_OUTPUT
#define SV_DEPTH S_DEPTH_OUTPUT
#define SV_TARGET0 S_TARGET_OUTPUT0
#define SV_TARGET1 S_TARGET_OUTPUT1
#define SV_TARGET2 S_TARGET_OUTPUT2
#define SV_TARGET3 S_TARGET_OUTPUT3
#define SV_TARGET4 S_TARGET_OUTPUT4
#define SV_TARGET5 S_TARGET_OUTPUT5
#define SV_TARGET6 S_TARGET_OUTPUT6
#define SV_TARGET7 S_TARGET_OUTPUT7

#define	IMAGESTORE(a,b,c) a[b]=c;


#define RWTexture3D RW_Texture3D
#define RWTexture2D RW_Texture2D
#define RWTexture1D RW_Texture1D
#define RWTexture2DArray RW_Texture2D_Array
#define RWStructuredBuffer RW_RegularBuffer

#define SampleLevel SampleLOD

#define imageStore(uav, pos, c) uav[pos]=c


#define uniform

#ifndef constant_buffer
#define constant_buffer unistruct
#endif

#define texture1D Texture1D
#define texture2D Texture2D
#define texture3D Texture3D

#define sampler1D Texture1D
#define sampler2D Texture2D
#define sampler3D Texture3D
#define TextureCubeArray TextureCube_Array

#define maxvertexcount MAX_VERTEX_COUNT
#define line Line

#ifndef __cplusplus
#define StructuredBuffer RegularBuffer

#define Texture2DMS MS_Texture2D
#define Texture2DArray Texture2D_Array
#define Vector2Unaligned		float2
#define Vector3Unaligned		float3
#define Vector4Unaligned		float4

uint FIRSTBITLOW_SLOW( uint input )
{
	for( int bit=0; bit<32; ++bit )
		if( input & (1<<bit) )
			return bit;
	return 32;
}

uint FIRSTBITHIGH_SLOW( uint input )
{
	for( int bit=31; bit>=0; --bit )
		if( input & (1<<bit) )
			return bit;
	return 32;
}

#define __CBREGISTER( i )		 : register( b ##i )

#ifdef __PSSL__
	#define Matrix4Unaligned column_major matrix
	#define unistruct	     ConstantBuffer
#else
	#define Matrix4Unaligned matrix
	#define unistruct	     cbuffer
#endif 

#define vec1 float
#define vec2 float2
#define vec3 float3
#define vec4 float4
#define char4 float4
#define uchar4 float4
#define mat4 Matrix4Unaligned
#define mat3 Matrix3Unaligned
#define mix lerp



#define SV_VERTEXID S_VERTEX_ID
#define SV_POSITION S_POSITION
#define SV_COVERAGE S_COVERAGE

#define SV_VertexID S_VERTEX_ID
#define SV_Position S_POSITION
#define SV_Coverage S_COVERAGE

#define POSITION S_POSITION
#endif
#endif