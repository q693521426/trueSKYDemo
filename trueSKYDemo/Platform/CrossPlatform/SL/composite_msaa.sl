//  Copyright (c) 2017 Simul Software Ltd. All rights reserved.
#ifndef COMPOSITE_MSAA_SL
#define COMPOSITE_MSAA_SL

#ifndef PI
#define PI (3.1415926536)
#endif


// NOTE: Performance here is ALL about texture bandwidth. If the textures can be made to use smaller formats: 16-bit floats,
// R11G11B10 etc, the shader will go much faster.
TwoColourCompositeOutput CompositeAtmospherics_MSAA(vec4 clip_pos
													,vec2 depth_texc
													,TextureCubeArray cloudCubeArray
													,TextureCube nearFarTexture
													,TextureCube lightpassTexture
													,Texture2D loss2dTexture
													,TEXTURE2DMS_FLOAT4 depthTextureMS
													,int numSamples
													,uint2 fullResDims
													,mat4 invViewProj
													,vec3 viewPos
													,vec4 viewportToTexRegionScaleBias
													,DepthInterpretationStruct depthInterpretationStruct
													,Texture3D inscatterVolumeTexture
													,Texture3D godraysVolumeTexture
													,float nearDist
													,bool do_lightpass
													,bool do_godrays
													,bool do_interp
													,bool do_near)
{
	TwoColourCompositeOutput res;
	vec3 view							=normalize(mul(invViewProj,clip_pos).xyz);
	int2 fullres_depth_pos2		=int2(depth_texc*vec2(fullResDims.xy));
	
	float sine							=view.z;
	vec4 nearFarCloud;
	if(do_interp)
		nearFarCloud					=texture_cube_lod(nearFarTexture	,view		,0);
	
	vec3 worldspaceVolumeTexCoords		=vec3(atan2(view.x,view.y)/(2.0*pi),0.5*(1.0+2.0*asin(sine)/pi),0);
	
	res.add						=vec4(0,0,0,1.0);
	res.multiply				=vec4(0,0,0,0);
	for(int k=0;k<numSamples;k++)
	{
		float depth				=TEXTURE_LOAD_MSAA(depthTextureMS,fullres_depth_pos2,k).x;

		float dist				=depthToLinearDistance(depth	,depthInterpretationStruct);
		float dist_rt			=sqrt(dist);
		vec3 offsetMetres		=view*dist;
		worldspaceVolumeTexCoords.z=dist_rt;

		vec4 insc				=texture_3d_wmc_lod(inscatterVolumeTexture,worldspaceVolumeTexCoords,0);
		vec2 loss_texc			=vec2(dist_rt,0.5*(1.0-sine));
		float cloudLevel;
		float cloudLevel_0					=0.0;
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

		vec4 cloud;
		if(do_interp)
		{
			vec4 cloudNear					=cloudCubeArray.SampleLevel(cubeSamplerState,vec4(view,cloudLevel_0),0);
			vec4 cloudFar					=cloudCubeArray.SampleLevel(cubeSamplerState,vec4(view,cloudLevel_0+1.0),0);
			cloud							=lerp(cloudNear, cloudFar,frac(cloudLevel));
			if(do_lightpass)
			{
				cloud.rgb					+=lp.rgb;
			}
			if(do_near)
			{
				float nearInterp			=saturate((dist-nearDist)/max(0.00000001,2.0*nearDist));
				cloud						=lerp(vec4(0,0,0,1.0),cloud,nearInterp);
			}
			insc.rgb				*=cloud.a;
			insc					+=cloud;
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
		res.multiply				+=cloud.a*texture_clamp_mirror_lod(loss2dTexture, loss_texc, 0);
		res.add						+=insc;
	}
	res.multiply/=float(numSamples);
	res.add/=float(numSamples);
    return res;
}

#endif