#pragma once
#include "Simul/Platform/OpenGL/Export.h"
#include "Simul/Platform/CrossPlatform/Buffer.h"
#include <string>
#include <map>

#pragma warning(disable:4251)

namespace simul
{
	namespace opengl
	{
		class SIMUL_OPENGL_EXPORT Buffer:public simul::crossplatform::Buffer
		{
			GLuint buf;
			GLuint tf_buffer;
			void* mapped;
		public:
			Buffer();
			~Buffer();
			void InvalidateDeviceObjects();
			ID3D11Buffer *AsD3D11Buffer();
			GLuint AsGLuint();
			GLuint TransformFeedbackAsGLuint();
			void EnsureVertexBuffer(crossplatform::RenderPlatform *renderPlatform,int num_vertices,const crossplatform::Layout *layout,const void *data,bool cpu_access=false,bool streamout_target=false);
			void EnsureIndexBuffer(crossplatform::RenderPlatform *renderPlatform,int num_indices,int index_size_bytes,const void *data);
			void *Map(crossplatform::DeviceContext &deviceContext) override;
			void Unmap(crossplatform::DeviceContext &deviceContext) override;
			GLuint vao;
		};
	}
};

