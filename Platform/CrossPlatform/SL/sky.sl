//  Copyright (c) 2015 Simul Software Ltd. All rights reserved.
#ifndef SKY_SL
#define SKY_SL
#ifndef pi
#define pi 3.1415926536
#endif
float approx_oren_nayar(float roughness,vec3 view,vec3 normal,vec3 lightDir)
{
	float roughness2 = roughness * roughness;
	vec2 oren_nayar_fraction = roughness2 / (vec2(roughness2,roughness2)+ vec2(0.33, 0.09));
	vec2 oren_nayar = vec2(1, 0) + vec2(-0.5, 0.45) * oren_nayar_fraction;
	// Theta and phi
	vec2 cos_theta = saturate(vec2(dot(normal, lightDir), dot(normal, view)));
	vec2 cos_theta2 = cos_theta * cos_theta;
	float u=saturate((1-cos_theta2.x) * (1-cos_theta2.y));
	float sin_theta = sqrt(u);
	vec3 light_plane = normalize(lightDir - cos_theta.x * normal);
	vec3 view_plane = normalize(view - cos_theta.y * normal);
	float cos_phi = saturate(dot(light_plane, view_plane));
	// Composition
	float diffuse_oren_nayar = cos_phi * sin_theta / max(0.00001,max(cos_theta.x, cos_theta.y));
	float diffuse = cos_theta.x * (oren_nayar.x + oren_nayar.y * diffuse_oren_nayar);
	return diffuse;
}

vec4 BackgroundLatLongSphere(Texture2D backgroundTexture,vec2 texCoords)
{
	vec2 clip_pos		=vec2(-1.0,1.0);
	clip_pos.x			+=2.0*texCoords.x;
	clip_pos.y			-=2.0*texCoords.y;
	vec3 view			=normalize(mul(invViewProj,vec4(clip_pos,1.0,1.0)).xyz);
	// Plate-carree projection:
	float ang			=atan2(view.y,-view.x);
	float t				=ang/(pi*2.0);
	float t1			=frac(t);
	vec2 lat_long_texc	=vec2(t1,0.5-asin(view.z)/pi);
	float t2			=0.5+frac(t-0.5);
	//if(abs(ddx(t1))<0.1*abs(ddx(t2)))
	//	lat_long_texc.x		=t2;
	//else
		lat_long_texc.x		=t1;

	//float interp		=saturate(abs(lat_long_texc.x)*100.0);
	vec4 result1		=texture_wrap_lod(backgroundTexture,lat_long_texc,0);
	
	//vec4 result2		=texture_wrap(backgroundTexture,lat_long_texc);
	//result1.r=saturate(abs(ddx(t1))<0.1*abs(ddx(t2)));
	return starBrightness*result1;//lerp(result1,result2,interp);
}

#endif