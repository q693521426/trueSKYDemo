/* SCE CONFIDENTIAL
ORBIS Programmer Tool Runtime Library Release 00.920.020
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef TEXTURE_UTIL_H
#define TEXTURE_UTIL_H

#include <gnm/texture.h>
#include "allocator.h"

namespace TextureUtil
{
	/**
	 * @brief Indicates the result of a GNF load operation
	 */
	enum GnfError
	{
		kGnfErrorNone                   =  0, // Operation was successful; no error
		kGnfErrorInvalidPointer         = -1, // Caller passed an invalid/NULL pointer to a GNF loader function
		kGnfErrorNotGnfFile             = -2, // Attempted to load a file that isn't a GNF file (bad magic number in header)
		kGnfErrorCorruptHeader          = -3, // Attempted to load a GNF file with corrupt header data
		kGnfErrorFileIsTooShort         = -4, // Attempted to load a GNF file whose size is smaller than the size reported in its header
		kGnfErrorVersionMismatch        = -5, // Attempted to load a GNF file created by a different version of the GNF code
		kGnfErrorAlignmentOutOfRange    = -6, // Attempted to load a GNF file with corrupt header data (surface alignment > 2^31 bytes)
		kGnfErrorContentsSizeMismatch   = -7, // Attempted to load a GNF file with corrupt header data (wrong size in GNF header contents)
		kGnfErrorCouldNotOpenFile       = -8, // Unable to open a file for reading
		kGnfErrorOutOfMemory            = -9, // Internal memory allocation failed
	};

	
    GnfError loadTextureFromGnf(sce::Gnm::Texture *outTexture, const char *fileName, uint8_t textureIndex, Allocator* allocator);
}

#endif // TEXTURE_UTIL_H

