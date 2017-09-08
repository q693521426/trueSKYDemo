#include "ShaderLoader.h"
#include "Simul/Base/RuntimeError.h"
#include <Gnmx/shader_parser.h>

namespace simul
{
	namespace orbis
	{
		namespace shaderloader
		{
			sce::Gnm::SizeAlign calculateMemoryRequiredForVsFetchShader(const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kVertexShader);

				sce::Gnm::SizeAlign result;
				result.m_align = sce::Gnm::kAlignmentOfBufferInBytes;
				result.m_size = computeVsFetchShaderSize(shaderInfo.m_vsShader);
				return result;
			}
			sce::Gnm::SizeAlign calculateMemoryRequiredForEsFetchShader(const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kVertexShader);

				sce::Gnm::SizeAlign result;
				result.m_align = sce::Gnm::kAlignmentOfBufferInBytes;
				result.m_size = computeEsFetchShaderSize(shaderInfo.m_esShader);
				return result;
			}
			
			uint64_t roundUpToAlignment(sce::Gnm::Alignment alignment, uint64_t bytes)
			{
				const uint64_t mask = alignment - 1;
				return (bytes + mask) & ~mask;
			}

			void *roundUpToAlignment(sce::Gnm::Alignment alignment, void *addr)
			{
				return reinterpret_cast<void*>(roundUpToAlignment(alignment, reinterpret_cast<uint64_t>(addr)));
			}

			////////////////////////////////////////////////////////////////////////////////
			//Shader Loading : SB File Format (with PSSL metadata)
			sce::Gnmx::VsShader *loadAndAllocateVertexProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes,simul::base::MemoryInterface *allocator)
			{
				sbp->loadFromMemory(pointer, bytes);
				const sce::Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForVsShader(pointer);
				void * dest = allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
				sce::Gnmx::VsShader *vertexShader = parseVsShader(dest, pointer);
				return vertexShader;
			}

			sce::Gnmx::PsShader* loadAndAllocatePixelProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, simul::base::MemoryInterface *allocator)
			{
				sbp->loadFromMemory(pointer, bytes);
				const sce::Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForPsShader(pointer);
				void * dest = allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
				sce::Gnmx::PsShader *pixelShader = parsePsShader(dest, pointer);
				return pixelShader;
			}

			sce::Gnmx::CsShader* loadAndAllocateComputeProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, simul::base::MemoryInterface *allocator)
			{
				sbp->loadFromMemory(pointer, bytes);
				const sce::Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForCsShader(pointer);
				void * dest = allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
				sce::Gnmx::CsShader *computeShader = parseCsShader(dest, pointer);
				return computeShader;
			}

			sce::Gnmx::LsShader* loadAndAllocateLocalProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, simul::base::MemoryInterface *allocator)
			{
				sbp->loadFromMemory(pointer, bytes);
				const sce::Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForLsShader(pointer);
				void * dest = allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
				sce::Gnmx::LsShader *localShader = parseLsShader(dest, pointer);
				return localShader;
			}

			sce::Gnmx::HsShader* loadAndAllocateHullProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, simul::base::MemoryInterface *allocator)
			{
				sbp->loadFromMemory(pointer, bytes);
				const sce::Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForHsShader(pointer);
				void * dest = allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
				sce::Gnmx::HsShader *hullShader = parseHsShader(dest, pointer);
				return hullShader;
			}

			sce::Gnmx::EsShader* loadAndAllocateExportProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, simul::base::MemoryInterface *allocator)
			{
				sbp->loadFromMemory(pointer, bytes);
				const sce::Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForEsShader(pointer);
				void * dest = allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
				sce::Gnmx::EsShader *exportShader = parseEsShader(dest, pointer);
				return exportShader;
			}

			sce::Gnmx::GsShader* loadAndAllocateGeometryProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, simul::base::MemoryInterface *allocator)
			{
				sbp->loadFromMemory(pointer, bytes);
				const sce::Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForGsShader(pointer);
				void * dest = allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
				sce::Gnmx::GsShader *geometryShader = parseGsShader(dest, pointer);
				return geometryShader;
			}

			sce::Gnmx::CsVsShader *loadAndAllocateComputeVertexProgram(sce::Shader::Binary::Program *sbp, const void* pointer, uint32_t bytes, simul::base::MemoryInterface *allocator)
			{
				sbp->loadFromMemory(pointer, bytes);
				const sce::Gnm::SizeAlign sizeAlign = calculateMemoryRequiredForCsVsShader(pointer);
				void * dest = allocator->AllocateVideoMemory(sizeAlign.m_size,sizeAlign.m_align);
				sce::Gnmx::CsVsShader *computeVertexShader = parseCsVsShader(dest, pointer);
				return computeVertexShader;
			}

			void releaseShaderProgram(sce::Shader::Binary::Program *sbp, simul::base::MemoryInterface *allocator)
			{
				SIMUL_ASSERT(sbp!=NULL);
				assert(sbp->m_dataChunk);
				allocator->DeallocateVideoMemory(sbp->m_dataChunk);
				sbp->m_dataChunk = NULL;
			}

			void saveShaderProgramToFile(sce::Shader::Binary::Program *sbp, const char *sbFilename)
			{
				SIMUL_ASSERT(sbp!=NULL);
				SIMUL_ASSERT(sbFilename!=NULL);

				// calculate size of this shader program
				uint32_t size = sbp->calculateSize();
				assert(size);

				// open output file
				FILE * file = fopen( (char*)sbFilename, "wb" );
				if ( !file )
				{
					SIMUL_BREAK("Failed to open file for writing."); exit(-1);
				}

				// save the data
				if ( sbp->m_dataChunk )
				{
					fwrite( sbp->m_dataChunk, size, 1, file );
				}
				else
				{
					void* data = malloc(size);
					sbp->saveToMemory(data, size);
					fwrite( data, size, 1, file );
					free(data);
				}

				fclose( file );
			}

			// runtime helper methods for shader binary element search
			ElementNode::ElementNode()
			: m_element(NULL)
			, m_buffer(NULL)
			, m_numSubNodes(0)
			, m_subNodes(NULL)
			{
			}

			ElementNode::~ElementNode()
			{
				if (m_subNodes)
					delete [] m_subNodes;
				m_subNodes = NULL;
				m_numSubNodes = 0;
			}

			ElementNode *ElementNode::getSubNodeAt(uint32_t index)
			{
				if ( index < m_numSubNodes )
					return m_subNodes[index];

				return NULL;
			}

			static uint32_t InitRecursiveElementNode(ElementNode *nodeList, uint32_t offset) // returns offset
			{
				ElementNode *node = nodeList + offset;
				sce::Shader::Binary::Element *element = node->m_element;

				if ( element->m_type == sce::Shader::Binary::kTypeStructure )
				{
					node->m_numSubNodes = element->m_numElements;
					node->m_subNodes = new ElementNode*[node->m_numSubNodes];
					for ( uint32_t i = 0; i < node->m_numSubNodes; i++ )
					{
						ElementNode *subNode = nodeList + offset + 1;
						subNode->m_fullNameStr = std::string(node->m_fullNameStr);
						subNode->m_fullNameStr.append(element->m_isPointer ? "->" : ".");
						subNode->m_fullNameStr.append(subNode->m_element->getName());
						node->m_subNodes[i] = subNode;

						offset = InitRecursiveElementNode(nodeList, offset+1);
					}
				}

				return offset;
			}

			ElementNode *InitElementNodes(const sce::Shader::Binary::Program *program)
			{
				ElementNode *nodeList = NULL;

				uint32_t numBuffers = program->m_numBuffers;
				uint32_t numElements = program->m_numElements;

				if (numBuffers && numElements)
				{
					nodeList = new ElementNode[numElements + numBuffers + 1]; // +1 for root

					// init root
					ElementNode *root = &nodeList[0];
					root->m_fullNameStr = std::string("root");
					root->m_numSubNodes = numBuffers;
					root->m_subNodes = new ElementNode*[root->m_numSubNodes];

					// init element nodes and names
					for( uint32_t i = 0; i < numElements; i++ )
					{
						ElementNode *node = &nodeList[numBuffers + 1 + i];
						sce::Shader::Binary::Element *element = &program->m_elements[i];
						node->m_element = element;
						const char* name = element->getName();
						if ( name && strlen(name) && strcmp(name, "(no_name)") != 0)
						{
							node->m_fullNameStr = std::string(element->getName());
						}
						else
						{
							node->m_fullNameStr = std::string("<struct>");
						}
					}

					// init buffer nodes and sub node pointers in the root
					for( uint32_t i = 0; i < numBuffers; i++ )
					{
						ElementNode *bufferNode = &nodeList[i+1];
						sce::Shader::Binary::Buffer *buffer = program->m_buffers + i;
						SIMUL_ASSERT ( strlen((const char*)buffer->getName())>0);
						bufferNode->m_fullNameStr = std::string(buffer->getName());
						bufferNode->m_buffer = buffer;
						root->m_subNodes[i] = bufferNode;

						// init element nodes
						bufferNode->m_numSubNodes = buffer->m_numElements;
						if (bufferNode->m_numSubNodes)
						{
							bufferNode->m_subNodes = new ElementNode*[bufferNode->m_numSubNodes];
							uint32_t offset = numBuffers + 1 + buffer->m_elementOffset;
							for( uint32_t j = 0; j < bufferNode->m_numSubNodes; j++ )
							{
								bufferNode->m_subNodes[j] = nodeList + offset;
								offset = InitRecursiveElementNode(nodeList, offset) + 1;
							}
						}
					}
				}

				return nodeList;
			}

			void ReleaseElementNodes(ElementNode *nodeList)
			{
				if(nodeList)
					delete [] nodeList;
			}

			static ElementNode *GetRecursiveNodeWithName(ElementNode *node, const char *elementName)
			{
				if ( node->m_fullNameStr.compare(elementName) == 0 )
				{
					return node;
				}

				for ( uint32_t i = 0; i < node->m_numSubNodes; i++ )
				{
					ElementNode *foundNode = GetRecursiveNodeWithName(node->getSubNodeAt(i), elementName);

					if ( foundNode )
						return foundNode;
				}

				return NULL;
			}

			ElementNode *GetElementNodeWithName(ElementNode *nodeList, const char *bufferName, const char *elementName)
			{
				if(nodeList)
				{
					ElementNode *root = &nodeList[0];
					SIMUL_ASSERT(root->m_fullNameStr.compare("root") == 0);

					// find a buffer node matches the buffer name
					if ( bufferName && strlen(bufferName) )
					{
						for ( uint32_t i = 0; i < root->m_numSubNodes; i++ )
						{
							ElementNode *bufferNode = root->m_subNodes[i];
							if ( bufferNode->m_fullNameStr.compare( bufferName ) == 0 )
							{
								// find an element node matches the element name
								for ( uint32_t j = 0; j < bufferNode->m_numSubNodes; j++ )
								{
									ElementNode *foundNode = GetRecursiveNodeWithName(bufferNode->m_subNodes[j], elementName);

									if ( foundNode )
										return foundNode;
								}
							}
						}
					}
				}

				return NULL;
			}

			sce::Gnm::SizeAlign calculateMemoryRequiredForVsShader(const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kVertexShader);

				sce::Gnm::SizeAlign result;
				result.m_align = sce::Gnm::kAlignmentOfShaderInBytes;
				result.m_size = roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_vsShader->computeSize())
							  + shaderInfo.m_gpuShaderCodeSize;
				return result;
			}

			sce::Gnm::SizeAlign calculateMemoryRequiredForPsShader(const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kPixelShader);

				sce::Gnm::SizeAlign result;
				result.m_align = sce::Gnm::kAlignmentOfShaderInBytes;
				result.m_size = roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_psShader->computeSize())
							  + shaderInfo.m_gpuShaderCodeSize;
				return result;
			}

			sce::Gnm::SizeAlign calculateMemoryRequiredForCsShader(const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kComputeShader);

				sce::Gnm::SizeAlign result;
				result.m_align = sce::Gnm::kAlignmentOfShaderInBytes;
				result.m_size = roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_csShader->computeSize())
							  + shaderInfo.m_gpuShaderCodeSize;
				return result;
			}

			sce::Gnm::SizeAlign calculateMemoryRequiredForLsShader(const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kLocalShader);

				sce::Gnm::SizeAlign result;
				result.m_align = sce::Gnm::kAlignmentOfShaderInBytes;
				result.m_size = roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_lsShader->computeSize())
							  + shaderInfo.m_gpuShaderCodeSize;
				return result;
			}

			sce::Gnm::SizeAlign calculateMemoryRequiredForHsShader(const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kHullShader);

				sce::Gnm::SizeAlign result;
				result.m_align = sce::Gnm::kAlignmentOfShaderInBytes;
				result.m_size = roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_hsShader->computeSize())
							  + shaderInfo.m_gpuShaderCodeSize;
				return result;
			}

			sce::Gnm::SizeAlign calculateMemoryRequiredForEsShader(const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kExportShader);

				sce::Gnm::SizeAlign result;
				result.m_align = sce::Gnm::kAlignmentOfShaderInBytes;
				result.m_size = roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_esShader->computeSize())
							  + shaderInfo.m_gpuShaderCodeSize;
				return result;
			}

			sce::Gnm::SizeAlign calculateMemoryRequiredForGsShader(const void *src)
			{
				sce::Gnmx::ShaderInfo gsShaderInfo;
				sce::Gnmx::ShaderInfo vsShaderInfo;
				sce::Gnmx::parseGsShader(&gsShaderInfo, &vsShaderInfo, src);

				sce::Gnm::SizeAlign result;
				result.m_align = sce::Gnm::kAlignmentOfShaderInBytes;
				result.m_size = roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, gsShaderInfo.m_gsShader->computeSize())
							  + roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, gsShaderInfo.m_gpuShaderCodeSize) 
							  + vsShaderInfo.m_gpuShaderCodeSize;
				return result;
			}

			sce::Gnm::SizeAlign calculateMemoryRequiredForCsVsFetchShaderVs(const void *src)
			{
				sce::Gnmx::ShaderInfo csvsShaderInfo;
				sce::Gnmx::ShaderInfo csShaderInfo;
				sce::Gnmx::parseCsVsShader(&csvsShaderInfo, &csShaderInfo, src);

				sce::Gnm::SizeAlign result;
				result.m_align = sce::Gnm::kAlignmentOfBufferInBytes;
				result.m_size = computeVsFetchShaderSize(csvsShaderInfo.m_csvsShader);
				return result;
			}

			sce::Gnm::SizeAlign calculateMemoryRequiredForCsVsFetchShaderCs(const void *src)
			{
				sce::Gnmx::ShaderInfo csvsShaderInfo;
				sce::Gnmx::ShaderInfo csShaderInfo;
				sce::Gnmx::parseCsVsShader(&csvsShaderInfo, &csShaderInfo, src);

				sce::Gnm::SizeAlign result;
				result.m_align = sce::Gnm::kAlignmentOfBufferInBytes;
				result.m_size = computeCsFetchShaderSize(csvsShaderInfo.m_csvsShader);
				return result;
			}

			sce::Gnm::SizeAlign calculateMemoryRequiredForCsVsShader(const void *src)
			{
				sce::Gnmx::ShaderInfo csvsShaderInfo;
				sce::Gnmx::ShaderInfo csShaderInfo;
				sce::Gnmx::parseCsVsShader(&csvsShaderInfo, &csShaderInfo, src);

				sce::Gnm::SizeAlign result;
				result.m_align = sce::Gnm::kAlignmentOfShaderInBytes;
				result.m_size = roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, csvsShaderInfo.m_csvsShader->computeSize())
							  + roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, csvsShaderInfo.m_gpuShaderCodeSize) 
							  + csShaderInfo.m_gpuShaderCodeSize;
				return result;
			}

			sce::Gnmx::VsShader* parseVsShader(void *dest, const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kVertexShader);

				sce::Gnmx::VsShader *const final = reinterpret_cast<sce::Gnmx::VsShader*>(dest);
				memcpy(dest, shaderInfo.m_vsShader, shaderInfo.m_vsShader->computeSize());
				dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_vsShader->computeSize());

				void * vsBlockAddr = dest;
				memcpy(dest, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);

				final->patchShaderGpuAddress(vsBlockAddr);
				return final;
			}

			sce::Gnmx::PsShader* parsePsShader(void * dest, const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kPixelShader);

				sce::Gnmx::PsShader *const final = reinterpret_cast<sce::Gnmx::PsShader*>(dest);
				memcpy(dest, shaderInfo.m_psShader, shaderInfo.m_psShader->computeSize());
				dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_psShader->computeSize());

				void * psBlockAddr = dest;
				memcpy(dest, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);

				final->patchShaderGpuAddress(psBlockAddr);
				return final;
			}

			sce::Gnmx::CsShader* parseCsShader(void * dest, const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kComputeShader);

				sce::Gnmx::CsShader *const final = reinterpret_cast<sce::Gnmx::CsShader*>(dest);
				memcpy(dest, shaderInfo.m_csShader, shaderInfo.m_csShader->computeSize());
				dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_csShader->computeSize());

				void * csBlockAddr = dest;
				memcpy(dest, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);

				final->patchShaderGpuAddress(csBlockAddr);
				return final;
			}

			sce::Gnmx::LsShader* parseLsShader(void * dest, const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kLocalShader);

				sce::Gnmx::LsShader *const final = reinterpret_cast<sce::Gnmx::LsShader*>(dest);
				memcpy(dest, shaderInfo.m_lsShader, shaderInfo.m_lsShader->computeSize());
				dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_lsShader->computeSize());

				void * lsBlockAddr = dest;
				memcpy(dest, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);

				final->patchShaderGpuAddress(lsBlockAddr);
				return final;
			}

			sce::Gnmx::HsShader* parseHsShader(void * dest, const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kHullShader);

				sce::Gnmx::HsShader *const final = reinterpret_cast<sce::Gnmx::HsShader*>(dest);
				memcpy(dest, shaderInfo.m_hsShader, shaderInfo.m_hsShader->computeSize());
				dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_hsShader->computeSize());

				void * hsBlockAddr = dest;
				memcpy(dest, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);

				final->patchShaderGpuAddress(hsBlockAddr);
				return final;
			}

			sce::Gnmx::EsShader* parseEsShader(void * dest, const void *src)
			{
				sce::Gnmx::ShaderInfo shaderInfo;
				sce::Gnmx::parseShader(&shaderInfo, src);//, sce::Gnmx::kExportShader);

				sce::Gnmx::EsShader *const final = reinterpret_cast<sce::Gnmx::EsShader*>(dest);
				memcpy(dest, shaderInfo.m_esShader, shaderInfo.m_esShader->computeSize());
				dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, shaderInfo.m_esShader->computeSize());

				void * esBlockAddr = dest;
				memcpy(dest, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);

				final->patchShaderGpuAddress(esBlockAddr);
				return final;
			}

			sce::Gnmx::GsShader *parseGsShader(void * dest, const void *src)
			{
				sce::Gnmx::ShaderInfo gsShaderInfo;
				sce::Gnmx::ShaderInfo vsShaderInfo;
				sce::Gnmx::parseGsShader(&gsShaderInfo, &vsShaderInfo, src);

				sce::Gnmx::GsShader *const final = reinterpret_cast<sce::Gnmx::GsShader*>(dest);
				memcpy(dest, gsShaderInfo.m_gsShader, gsShaderInfo.m_gsShader->computeSize());
				dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, gsShaderInfo.m_gsShader->computeSize());

				void * gsBlockAddr = dest;
				memcpy(dest, gsShaderInfo.m_gpuShaderCode, gsShaderInfo.m_gpuShaderCodeSize);
				dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, gsShaderInfo.m_gpuShaderCodeSize);

				void * vsBlockAddr = dest;
				memcpy(dest, vsShaderInfo.m_gpuShaderCode, vsShaderInfo.m_gpuShaderCodeSize);

				final->patchShaderGpuAddresses(gsBlockAddr, vsBlockAddr);
				return final;
			}

			sce::Gnmx::CsVsShader *parseCsVsShader(void *dest, const void *src)
			{
				sce::Gnmx::ShaderInfo csvsShaderInfo;
				sce::Gnmx::ShaderInfo csShaderInfo;
				sce::Gnmx::parseCsVsShader(&csvsShaderInfo, &csShaderInfo, src);

				sce::Gnmx::CsVsShader *const final = reinterpret_cast<sce::Gnmx::CsVsShader*>(dest);
				memcpy(dest, csvsShaderInfo.m_csvsShader, csvsShaderInfo.m_csvsShader->computeSize());
				dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, csvsShaderInfo.m_csvsShader->computeSize());

				void * vsBlockAddr = dest;
				memcpy(dest, csvsShaderInfo.m_gpuShaderCode, csvsShaderInfo.m_gpuShaderCodeSize);
				dest = reinterpret_cast<char*>(dest) + roundUpToAlignment(sce::Gnm::kAlignmentOfShaderInBytes, csvsShaderInfo.m_gpuShaderCodeSize);

				void * csBlockAddr = dest;
				memcpy(dest, csShaderInfo.m_gpuShaderCode, csShaderInfo.m_gpuShaderCodeSize);

				final->patchShaderGpuAddresses(vsBlockAddr, csBlockAddr);
				return final;
			}
		}
	}
}