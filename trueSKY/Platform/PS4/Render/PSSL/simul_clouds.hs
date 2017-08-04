#ifndef SIMUL_CLOUDS_HS
#define SIMUL_CLOUDS_HS

struct vertexInput
{
    vec3 position		: POSITION;
	// Per-instance data:
	vec2 noiseOffset	: TEXCOORD0;
	float layerDistance	: TEXCOORD1;
	float noiseScale	: TEXCOORD2;
	float layerFade		: TEXCOORD3;
};


struct vertexOutput
{
    vec3 position		: TEXCOORD0;
	// Per-instance data:
	float layerDistance	: TEXCOORD1;
	float noiseScale	: TEXCOORD2;
	float layerFade		: TEXCOORD3;
	vec2 noiseOffset	: TEXCOORD4;
};

struct geomOutput
{
    vec4 hPosition		: S_POSITION;
    vec2 noise_texc		: TEXCOORD0;
    vec4 texCoords		: TEXCOORD1;
	vec3 view			: TEXCOORD2;
    vec2 fade_texc		: TEXCOORD4;
	float layerFade		: TEXCOORD5;
};

#endif