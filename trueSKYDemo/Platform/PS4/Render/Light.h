#pragma once

#ifndef PS4_RENDER_LIGHT_H
#define PS4_RENDER_LIGHT_H

#include "Export.h"
#include "Material.h"
#include "Simul/Platform/CrossPlatform/Light.h"

namespace simul
{
	namespace orbis
	{
		class Light:public crossplatform::Light
		{
		public:
			Light();
			~Light();
			void UpdateLight(const double *mat,float lConeAngle,const float lLightColor[4]) const;
		protected:
			//GLuint mLightIndex;
		};
	}
}
#endif