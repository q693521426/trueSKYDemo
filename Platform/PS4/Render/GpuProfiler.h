#pragma once

//#define ENABLE_THREAD_TRACE 1

#include "gnmx/lwgfxcontext.h"
#include "Simul/Base/Timer.h"
#include "Simul/Base/MemoryInterface.h"
#include "Simul/Base/ProfilingInterface.h"
#include "Simul/Platform/CrossPlatform/GpuProfiler.h"
#include <string>
#include <map>

namespace simul
{
	namespace orbis
	{
		struct ProfilingQuery
		{
			ProfilingQuery():timestamp(NULL){}
			~ProfilingQuery(){}
			unsigned long *timestamp;
		};
		class GpuProfiler:public simul::crossplatform::GpuProfiler
		{
		public:
			static GpuProfiler &GetGlobalGpuProfiler();
			~GpuProfiler();
			void Begin(crossplatform::DeviceContext &deviceContext,const char *name);
			void End(crossplatform::DeviceContext &deviceContext);

			void StartFrame(crossplatform::DeviceContext &deviceContext);
			void EndFrame(crossplatform::DeviceContext &deviceContext);
			void AfterEndFrame();
			void SaveTrace();

		};
	}
}