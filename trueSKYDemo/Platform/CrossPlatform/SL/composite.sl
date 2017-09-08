//  Copyright (c) 2015-2017 Simul Software Ltd. All rights reserved.
#ifndef COMPOSITE_SL
#define COMPOSITE_SL
#define DEBUG_COMPOSITING


#ifndef PI
#define PI (3.1415926536)
#endif


#define CLOUD_DEFS_ONLY
#include "simul_clouds.sl"
struct TwoColourCompositeOutput
{
	vec4 add		SIMUL_RENDERTARGET_OUTPUT(0);
	vec4 multiply	SIMUL_RENDERTARGET_OUTPUT(1);
};

struct LookupQuad4
{
	vec4 _11;
	vec4 _21;
	vec4 _12;
	vec4 _22;
};

#define VOLUME_INSCATTER
#define SCREENSPACE_VOL


// NOTE: Performance here is ALL about texture bandwidth. If the textures can be made to use smaller formats: 16-bit floats,
// R11G11B10 etc, the shader will go much faster.
TwoColourCompositeOutput CompositeAtmospherics(vec4 clip_pos
				,TextureCubeArray cloudCubeArray
				,TextureCube nearFarTexture
				,TextureCube lightpassTexture
				,Texture2D loss2dTexture
				,float dist
				,mat4 invViewProj
				,vec3 viewPos
				,Texture3D inscatterVolumeTexture
				,Texture3D godraysVolumeTexture
				,float maxFadeDistanceKm
				,float nearDist
				,bool do_lightpass
				,bool do_godrays
				,bool do_interp
				,bool do_near)
{
	TwoColourCompositeOutput res;
	vec3 view							=normalize(mul(invViewProj,clip_pos).xyz);
	float sine							=view.z;
	vec4 nearFarCloud;
	if(do_interp)
		nearFarCloud					=texture_cube_lod(nearFarTexture	,view		,0);

	float dist_rt						=sqrt(dist);
	vec3 offsetMetres					=view*dist;
	vec3 worldspaceVolumeTexCoords		=vec3(atan2(view.x,view.y)/(2.0*pi),0.5*(1.0+2.0*asin(sine)/pi),dist_rt);

	// cut-off at the edges.
	vec4 insc							=texture_3d_wmc_lod(inscatterVolumeTexture,worldspaceVolumeTexCoords,0);
	vec2 loss_texc						=vec2(dist_rt,0.5*(1.0-sine));
	res.multiply						=texture_clamp_mirror_lod(loss2dTexture, loss_texc, 0);
	float cloudLevel;
	float cloudLevel_0					=0.0;
#if 1
	if(do_interp)
	{
		float f								=max(nearFarCloud.x,nearFarCloud.y);
		float n								=min(nearFarCloud.y,nearFarCloud.x);
		float hiResInterp					=1.0-saturate(( f- dist) / (f-n));
	
	// This is the interp from the near to the far clouds.
		cloudLevel						=float(NUM_CLOUD_INTERP-1)*saturate(hiResInterp);	// i.e. 0,1,2 levels of the array.
		cloudLevel_0					=floor(cloudLevel);
	}
	vec4 lp;
	if(do_lightpass)
		lp								=texture_cube_lod(lightpassTexture,view,0);
#endif
	
	vec4 cloud;
	if(do_interp)
	{
		vec4 cloudNear					=cloudCubeArray.SampleLevel(cubeSamplerState,vec4(view,cloudLevel_0),0);
		vec4 cloudFar					=cloudCubeArray.SampleLevel(cubeSamplerState,vec4(view,cloudLevel_0+1.0),0);
		cloud							=lerp(cloudNear, cloudFar,frac(cloudLevel));
		if(do_lightpass)
		{
			cloud.rgb				+=lp.rgb;
		}
		if(do_near)
		{
			float nearInterp			=saturate((dist-nearDist)/max(0.00000001,2.0*nearDist));
			cloud						=lerp(vec4(0, 0, 0, 1.0), cloud, nearInterp);
		}
		insc.rgb *= cloud.a;
		insc += cloud;
	}
	else
	{
		float cloud_visible			=step(1.0,dist);
		if(cloud_visible>0.0)
		{
			cloud					=cloudCubeArray.SampleLevel(cubeSamplerState,vec4(view,0),0);
			cloud					=lerp(vec4(0,0,0,1.0),cloud,cloud_visible);
			insc.rgb				*=cloud.a;
			insc.rgb				+=cloud.rgb;
			insc.a					=cloud.a;
		}
		else
			cloud					=vec4(0,0,0,1.0);
	}
	if(do_godrays)
	{
		vec3 lightspaceOffset			=mul(worldToScatteringVolumeMatrix,vec4(view,1.0)).xyz;
		float r							=length(lightspaceOffset);
		vec3 lightspaceVolumeTexCoords	=vec3(frac(atan2(lightspaceOffset.x,lightspaceOffset.y)/(2.0*pi))
													,0.5+0.5*asin(lightspaceOffset.z/r)*2.0/pi
													,r);
		vec4 godrays					=texture_3d_wcc_lod(godraysVolumeTexture,lightspaceVolumeTexCoords,0);
		insc							*=godrays;
	}
	res.multiply				*=cloud.a;
	res.add						=insc;
    return res;
}
#endif
