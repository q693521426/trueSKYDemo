/* SCE CONFIDENTIAL
ORBIS Programmer Tool Runtime Library Release 00.920.020
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef __HTILE_H__
#define __HTILE_H__

#include "shader_common/shader_base.h"
#include "shared_symbols.h"

uint bitmask( uint count )
{
	return (1<<count)-1;
}

int extractsigned( uint bits, uint offset, uint count )
{
	return (bits>>offset) & bitmask(count);
}

uint extractunsigned( uint bits, uint offset, uint count )
{
	return (bits>>offset) & bitmask(count);
}

float extractfloat( uint bits, uint offset, uint count )
{
	return extractunsigned(bits,offset,count) / (float)bitmask(count);
}

struct HTileVertexInput
{
	float4 position : S_POSITION;
};

struct HTileVertexOutput
{
	float4 position : S_POSITION;
	float2 tile     : TEXCOORD0;
};

struct HTileFragmentOutput
{
	float4 color_edge   : S_TARGET_OUTPUT0;
};

uint PadTo( uint value, uint pitch )
{
	return ( value + pitch-1 ) / pitch * pitch;
}

ConstantBuffer HTileTableShaderConstants : register(c1)
{
	uint4 m_remap[8];
	int4 m_decodeDelta[64];
};

uint getbits( uint bits, uint shift, uint a )
{
	return ((bits>>a)&1)<<shift;
}

uint notbits( uint bits, uint shift, uint a )
{
	return (((bits>>a)&1)^1)<<shift;
}

uint xorbits( uint bits, uint shift, uint a, uint b )
{
	return (((bits>>a)^(bits>>b))&1)<<shift;
}

uint xorbit3( uint bits, uint shift, uint a, uint b, uint c )
{
	return (((bits>>a)^(bits>>b)^(bits>>c))&1)<<shift;
}

uint getbit( uint word, uint bit )
{
	return (word>>bit)&1;
}

struct Encoder
{
	uint base;
	uint shift;
	uint mask;
};	

ConstantBuffer HTilePixelShaderConstantBuffer : register(c0)
{
	uint4 ps_tiles;
	uint4 ps_htileField;
	uint4 ps_format;
	uint4 ps_layout;
	uint4 ps_offset;
}

uint TilesWide()
{
	return ( ps_tiles.x + 15 ) / 16 * 16;
}

uint CachelinesWide()
{
	return ( ps_tiles.x + kTilesPerCachelineWide - 1 ) / kTilesPerCachelineWide;
}

uint MacroTilesWide() 
{
	return ( ps_tiles.x + kTilesPerMacrotileWide-1 ) / kTilesPerMacrotileWide;
}

uint HtileOffsetLinear( uint tile_x, uint tile_y )
{
	uint macrotile_x = tile_x / kTilesPerMacrotileWide; // x of macrotile in buffer
	uint macrotile_y = tile_y / kTilesPerMacrotileTall; // y of macrotile in buffer
	uint macrotile_index = macrotile_y * PadTo( MacroTilesWide(), 2 ) + macrotile_x; // raster index of macrotile in buffer
	uint cacheline_x = macrotile_index % kMacrotilesPerCachelineWide; // macrotile x, if raster were 8 macrotiles wide instead of what it actually is
	uint cacheline_y = macrotile_index / kMacrotilesPerCachelineWide; // macrotile y, if raster were 8 macrotiles wide instead of what it actually is
	uint tile_in_macrotile_x = tile_x % kTilesPerMacrotileWide;
	uint tile_in_macrotile_y = tile_y % kTilesPerMacrotileTall;

	tile_in_macrotile_y ^= m_remap[tile_in_macrotile_x].x;
	
	uint final_x = cacheline_x * kTilesPerMacrotileWide + tile_in_macrotile_x;
	uint final_y = cacheline_y * kTilesPerMacrotileTall + tile_in_macrotile_y;
	
	uint offset = final_y * kTilesPerCachelineWide + final_x;
	return offset;	
}

uint getPipeIndexOfTile(uint x, uint y)
{
	uint pipe = 0;
	pipe |= ( ((x>>0) ^ (y>>0) ^ (x>>1))	& 0x1 ) << 0;
	pipe |=	( ((x>>1) ^ (y>>1))				& 0x1 ) << 1;
	pipe |=	( ((x>>2) ^ (y>>2))				& 0x1 ) << 2;
	return pipe;
}

uint HtileOffsetTiled( uint x, uint y )
{
	const uint cl_x = x >> 6;
	const uint cl_y = y >> 6;
	const uint cl_offset = (cl_y * CachelinesWide() + cl_x) << 9;

	const uint macro_x = (x>>2) & 15;
	const uint macro_y = (y>>2) & 15;
	uint macro_offset = ( (macro_y << 4) + macro_x ) << 1;

	const uint tile_x = x & 3;
	const uint tile_y = y & 3;

	macro_offset &= ~3;
	macro_offset |= (( (tile_x>>1) ^ (tile_y>>0) )&1) << 0;
	macro_offset |= (( (tile_x>>1)               )&1) << 1;

	const uint tile_number = cl_offset + macro_offset;
	const uint device_address = tile_number * 4;
	const uint pipe = getPipeIndexOfTile(x, y);
	const uint pipe_interleave = 256;
	const uint num_pipes = 8;
	const uint final_address = (device_address % pipe_interleave) + (pipe * pipe_interleave) + (device_address / pipe_interleave) * pipe_interleave * num_pipes;

	return final_address / 4; // convert from bytes to dwords
}

#endif
