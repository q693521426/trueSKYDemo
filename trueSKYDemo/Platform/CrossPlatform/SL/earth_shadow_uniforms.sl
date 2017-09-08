//  Copyright (c) 2015 Simul Software Ltd. All rights reserved.
#ifndef EARTH_SHADOW_UNIFORMS_SL
#define EARTH_SHADOW_UNIFORMS_SL

SIMUL_CONSTANT_BUFFER(EarthShadowConstants,9)
	uniform vec3 sunDir;
	uniform float radiusOnCylinder;

	uniform vec3 earthShadowNormal;
	uniform float maxFadeDistance;

	uniform vec2 targetTextureSize;
	uniform float terminatorDistance;
	uniform float earthShadowEffectStrength;

	uniform float planetRadiusKm;
	uniform float pad21473485;
	uniform float pad2754763;
	uniform float pad24633;
SIMUL_CONSTANT_BUFFER_END

#endif