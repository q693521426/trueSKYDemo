#pragma once
#include "Simul/Platform/PS4/Render/Export.h"
#include "Simul/Platform/CrossPlatform/Mesh.h"
#include "Simul/Platform/CrossPlatform/Buffer.h"
#include <vector>
#include <gnmx/lwgfxcontext.h>

namespace simul
{
	namespace orbis
	{
		class Mesh:public crossplatform::Mesh
		{
		public:
			Mesh();
			~Mesh();
			void InvalidateDeviceObjects();
			bool Initialize(crossplatform::RenderPlatform *renderPlatform, int lPolygonVertexCount, const float *lVertices, const float *lNormals, const float *lUVs, int lPolygonCount, const unsigned int *lIndices);
			void GetVertices(void *target, void *indices);
			void releaseBuffers();
			// Bind buffers, set vertex arrays, turn on lighting and texture.
			void BeginDraw		(crossplatform::DeviceContext &deviceContext, crossplatform::ShadingMode pShadingMode)const;
			// Draw all the faces wich specific material with given shading mode.
			void Draw			(crossplatform::DeviceContext &deviceContext,int pMaterialIndex, crossplatform::ShadingMode pShadingMode)const;
			// Unbind buffers, reset vertex arrays, turn off lighting and texture.
			void EndDraw		(crossplatform::DeviceContext &deviceContext)const;
			// Get the count of material groups
			template<class T,typename U> void init(sce::Gnmx::LightweightGfxContext *pd3dDevice,const std::vector<T> &vertices, std::vector<U> indices)
			{
				int num_vertices		=(int)vertices.size();
				int num_indices			=(int)indices.size();
				T *v					=new T[num_vertices];
				U *ind					=new U[num_indices];
				for(int i=0;i<num_vertices;i++)
					v[i]=vertices[i];
				for(int i=0;i<num_indices;i++)
					ind[i]=indices[i];
				init(pd3dDevice,num_vertices,num_indices,v,ind);
				delete [] v;
				delete [] ind;
			}
			template<class T,typename U> void init(sce::Gnmx::LightweightGfxContext *pd3dDevice,int num_vertices, int num_indices,T *vertices,U *indices)
			{
				
			}
			
			void apply(crossplatform::DeviceContext &deviceContext,unsigned instanceStride,crossplatform::Buffer *instanceBuffer);
			crossplatform::Buffer *vertexBuffer;
			crossplatform::Buffer *indexBuffer;
			crossplatform::Layout *inputLayout;
			unsigned stride;
			unsigned numVertices;
			unsigned numIndices;
		protected:
			void UpdateVertexPositions(int lVertexCount, float *lVertices) const;
			mutable crossplatform::Layout *previousInputLayout;
			mutable crossplatform::Topology *previousTopology;
			
		};
	}
}