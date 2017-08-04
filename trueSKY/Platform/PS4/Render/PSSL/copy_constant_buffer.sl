#ifndef COPY_CONSTANT_BUFFER_SL
#define COPY_CONSTANT_BUFFER_SL

constant_buffer CopyConstantBuffer SIMUL_TEXTURE_REGISTER(11)
{
	uint start_texel;
	uint num_texels;
	uint width;
	uint height;
	uint z_start;
};

#endif