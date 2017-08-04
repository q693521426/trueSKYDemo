/* SCE CONFIDENTIAL
ORBIS Programmer Tool Runtime Library Release 00.920.020
* Copyright (C) 2012 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef SHARED_SYMBOLS_H
#define SHARED_SYMBOLS_H

#define kVerticesPerTriangle ( 3 )
#define kIndicesPerTriangle ( 3 )
#define kTrianglesPerQuad ( 2 )
#define kVerticesPerQuad ( 4 )

#define kPixelsPerTileWide ( 8 )
#define kPixelsPerTileTall ( 8 )
#define kTilesPerMacrotileWide ( 8 )
#define kTilesPerMacrotileTall ( 8 )
#define kTilesPerCachelineWide ( 64 )
#define kTilesPerCachelineTall ( 64 )
#define kMacrotilesPerCachelineWide ( 8 )
#define kMacrotilesPerCachelineTall ( 8 )
#define kPixelsPerCachelineWide ( kPixelsPerTileWide * kTilesPerCachelineWide )
#define kPixelsPerCachelineTall ( kPixelsPerTileTall * kTilesPerCachelineTall )
#define kTilesPerCacheline ( kTilesPerCachelineWide * kTilesPerCachelineTall )

#define kHTileVisualizationMethodA ( 0 )
#define kHTileVisualizationMethodB ( 1 )
#define kHTileVisualizationMethodBMinusA ( 2 )
#define kHTileVisualizationMethodAMinusB ( 3 )

#define kHTileLayoutLinear ( 0 )
#define kHTileLayoutTiled  ( 1 )

#define kHTileNoStencilZMask      ( 0 )
#define kHTileNoStencilMinZ       ( 1 )
#define kHTileNoStencilMaxZ       ( 2 )
#define kHTileNoStencilDelta      ( 3 )

#define kHTileStencilZMask        ( 4 )
#define kHTileStencilSR0          ( 5 )
#define kHTileStencilSR1          ( 6 )
#define kHTileStencilSMem         ( 7 )
#define kHTileStencilXX           ( 8 )
#define kHTileStencilDeltaZ       ( 9 )
#define kHTileStencilBaseZ       ( 10 )
#define kHTileStencilMinZMaxBase ( 11 )
#define kHTileStencilMaxZMinBase ( 12 )

#define kThreadsPerWavefront      ( 64 )
#define kTwoBitMask               ( 3 )
#define kThreeBitMask             ( 7 ) 

#define kIntegerZMaximum ( (1<<14) - 1 )

#define kHTileFormatNoStencil    ( 0 )
#define kHTileFormatStencilMinZ  ( 1 )
#define kHTileFormatStencilMaxZ  ( 2 )

#define kZMaskExpanded ( 0xF )

#endif
