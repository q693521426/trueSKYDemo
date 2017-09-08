#pragma once
#include "Export.h"
#include "Simul/Platform/CrossPlatform/Layout.h"
#include <gnm/constants.h>
namespace simul
{
	namespace orbis
	{
		class Layout:public crossplatform::Layout
		{
		public:
			Layout();
			~Layout();
			void InvalidateDeviceObjects();
			void Apply(crossplatform::DeviceContext &deviceContext);
			void Unapply(crossplatform::DeviceContext &deviceContext);
		};
	}
}