#pragma once

#include <map>

#include "Export.h"
#include "Simul/Platform/CrossPlatform/Material.h"

namespace simul
{
	namespace orbis
	{
		class Material:public crossplatform::Material
		{
		public:
			Material();
			virtual ~Material();
			void Apply(crossplatform::DeviceContext &,crossplatform::PhysicalLightRenderData &);
			crossplatform::Effect *effect;
		};
	}
}