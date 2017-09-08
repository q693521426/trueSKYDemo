/* SCE CONFIDENTIAL
ORBIS Programmer Tool Runtime Library Release 00.920.020
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "gnf.h"
#include "texture_util.h"
#include <stdio.h>
#include <unistd.h>

using namespace sce;
using namespace sce::Gnmx;

namespace
{
	/** @brief Loads a GNF file header and verifies that header contains valid information
	  * @param outHeader Pointer to GNF header structure to be filled with this call
	  * @param gnfFile File pointer to read this data from
	  * @return Zero if successful; otherwise, a non-zero error code.
	  */
    TextureUtil::GnfError loadGnfHeader(sce::Gnf::Header *outHeader, FILE *gnfFile)
	{
		if (outHeader == NULL || gnfFile == NULL)
		{
			return TextureUtil::kGnfErrorInvalidPointer;
		}
		outHeader->m_magicNumber     = 0;
		outHeader->m_contentsSize    = 0;
		fseek(gnfFile, 0, SEEK_SET);
		fread(outHeader, sizeof(sce::Gnf::Header), 1, gnfFile);
		if (outHeader->m_magicNumber != sce::Gnf::kMagic)
		{
			return TextureUtil::kGnfErrorNotGnfFile;
		}
		if (outHeader->m_contentsSize>sce::Gnf::kMaxContents)
		{
			return TextureUtil::kGnfErrorCorruptHeader;
		}
		return TextureUtil::kGnfErrorNone;
	}

	/** @brief Loads GNF contents and verifies that the contents contain valid information
	  * @param outContents Pointer to gnf contents to be read
	  * @param elementCnt The number of elements to read into gnfContents. Usually read from the Gnf::Header object.
	  * @param gnfFile File pointer to read this data from
	  * @return Zero if successful; otherwise, a non-zero error code.
	*/
	TextureUtil::GnfError readGnfContents(sce::Gnf::Contents *outContents, uint32_t contentsSizeInBytes, FILE *gnfFile)
	{
		if(outContents == NULL || gnfFile == 0)
		{
			return TextureUtil::kGnfErrorInvalidPointer;
		}
		fseek(gnfFile, sizeof(sce::Gnf::Header), SEEK_SET);

		size_t bytesRead = fread(outContents, 1, contentsSizeInBytes, gnfFile);
		if(bytesRead!=contentsSizeInBytes)
		{
			return TextureUtil::kGnfErrorFileIsTooShort;
		}

		if( outContents->m_version != sce::Gnf::kVersion )
		{
			return TextureUtil::kGnfErrorVersionMismatch;
		}
		if(outContents->m_alignment>31)
		{
			return TextureUtil::kGnfErrorAlignmentOutOfRange;
		}
		if( (outContents->m_numTextures*sizeof(sce::Gnm::Texture) + sizeof(sce::Gnf::Contents)) != contentsSizeInBytes )
		{
			return TextureUtil::kGnfErrorContentsSizeMismatch;
		}
		return TextureUtil::kGnfErrorNone;
	}
}

TextureUtil::GnfError TextureUtil::loadTextureFromGnf(sce::Gnm::Texture *outTexture, const char *fileName, uint8_t textureIndex, Allocator* allocator)
{
	if( (fileName==NULL) || (outTexture==NULL) )
	{
		return kGnfErrorInvalidPointer;
	}
	// SCE_GNM_VALIDATE(access(fileName, R_OK) == 0, "** Asset Not Found: %s\n", fileName);

	GnfError result = kGnfErrorNone;
	FILE *gnfFile = NULL;
	sce::Gnf::Contents *gnfContents = NULL;
	do
	{
		gnfFile = fopen(fileName, "rb");
		if (gnfFile == 0)
		{
			result = kGnfErrorCouldNotOpenFile;
			break;
		}

		sce::Gnf::Header header;
		result = loadGnfHeader(&header,gnfFile);
		if (result != 0)
		{
			break;
		}

		gnfContents = (sce::Gnf::Contents *)malloc(header.m_contentsSize);
		if(!gnfContents)
		{
			printf("Memory allocation failed \n");
			break;
		}
		result = readGnfContents(gnfContents, header.m_contentsSize, gnfFile );
		if (result)
		{
			break;
		}

		sce::Gnm::SizeAlign pixelsSa = getTexturePixelsSize(gnfContents, textureIndex);
		void *pixelsAddr = allocator->Allocate(pixelsSa, SCE_KERNEL_WC_GARLIC_NONVOLATILE);
		if( pixelsAddr==0 ) // memory allocation failed
		{
			result = kGnfErrorOutOfMemory;
			break;
		}

		fseek(gnfFile, sizeof(sce::Gnf::Header) + header.m_contentsSize + getTexturePixelsByteOffset(gnfContents, textureIndex) , SEEK_SET);
		fread(pixelsAddr, pixelsSa.m_size, 1, gnfFile);
		*outTexture = *patchTextures(gnfContents, textureIndex, 1, &pixelsAddr);
	}
	while(0);

	free(gnfContents);
	if (gnfFile != NULL)
		fclose(gnfFile);
	return result;
}

