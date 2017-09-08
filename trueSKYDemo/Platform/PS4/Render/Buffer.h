#pragma once
#include "Simul/Platform/PS4/Render/Export.h"
#include "Simul/Platform/CrossPlatform/Buffer.h"
#include <string>
#include <map>
#include <gnm/buffer.h>
#ifdef _MSC_VER
#pragma warning(disable:4251)
#endif

namespace simul
{
	namespace orbis
	{
		class RenderPlatform;
		class Buffer:public simul::crossplatform::Buffer
		{
			sce::Gnm::Buffer *gnmBuffers;
			void *videoMemory;
			orbis::RenderPlatform *renderPlatformPS4;
		public:
			Buffer();
			~Buffer();
			void InvalidateDeviceObjects();
			sce::Gnm::Buffer *AsGnmBuffer();
			//void ActualApply(crossplatform::DeviceContext &deviceContext,int slot);
			void *GetVideoMemory();
			void EnsureVertexBuffer(crossplatform::RenderPlatform *renderPlatform,int num_vertices,const crossplatform::Layout *layout,const void *data,bool cpu_access=false,bool streamout_target=false);
			void EnsureIndexBuffer(crossplatform::RenderPlatform *renderPlatform,int num_indices,int index_size_bytes,const void *data);
			void *Map(crossplatform::DeviceContext &deviceContext);
			void Unmap(crossplatform::DeviceContext &deviceContext);
		};
	}
};

