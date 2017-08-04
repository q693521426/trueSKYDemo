#pragma once

#ifndef OPENGL_LIGHT_H
#define OPENGL_LIGHT_H

#include "Export.h"
#include "Material.h"
#include "Simul/Platform/CrossPlatform/Light.h"

namespace simul
{
	namespace opengl
	{
		class SIMUL_OPENGL_EXPORT Light:public crossplatform::Light
		{
		public:
			Light();
			~Light();
			void UpdateLight(const double *mat,float lConeAngle,const float lLightColor[4]) const;
		protected:
			GLuint mLightIndex;
		};
	}
}
#endif