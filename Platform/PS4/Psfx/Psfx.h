/* Copyright (c) 2011, Max Aizenshtein <max.sniffer@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#pragma once

extern "C" {

/**************************************************
* psfxGenEffect
* Return value: Effect id
**************************************************/
int psfxGenEffect();

/**************************************************
* psfxParseEffectFromFileSIMUL
* Input:
*   effect  -- psfx effect id
*   src    -- File contents with includes inlined
*   filenamesUtf8    -- File name list
* Return value: Status
**************************************************/
bool psfxParseEffectFromTextSIMUL(int effect, const char* src,const char **filenamesUtf8);
/**************************************************
* psfxCreateEffectFromFile
* Input:
*   effect  -- psfx effect id
*   file    -- File name
* Return value: Status
**************************************************/
bool psfxParseEffectFromFile( int effect, const char* file,const char **paths,const char *outputfile,const char **args);

/**************************************************
* psfxCreateEffectFromMemory
* Input:
*   effect  -- psfx effect id
*   src    -- Source
* Return value: Status
**************************************************/
bool psfxParseEffectFromMemory( int effect, const char* src ,const char *filename,const char *output_filename);

/**************************************************
* psfxCompileProgram
* Input:
*   effect  -- psfx effect id
*   program -- Program name
* Return value: GL program id if success, 0 otherwise
**************************************************/
bool psfxCompileProgram(int effect, const char* program);

/**************************************************
* psfxGetProgramCount
* Return value: Number of programs
**************************************************/
int psfxGetProgramCount(int effect);

/**************************************************
* psfxGetProgramName
* Input:
*   effect  -- psfx effect id
*   program -- Index of program
*   name    -- Destination address
*   bufSize -- Size of the buffer
**************************************************/
void psfxGetProgramName(int effect, int program, char* name, int bufSize);

/**************************************************
* psfxGetProgramIndex
* Input:
*   effect  -- psfx effect id
*   name -- name of program
**************************************************/
size_t psfxGetProgramIndex(int effect, const char* name);

/**************************************************
* psfxGenerateSampler
* Input:
*   effect  -- psfx effect id
*   sampler -- Sampler name
* Return value: GL sampler id if success, -1 otherwise
**************************************************/
int psfxGenerateSampler(int effect, const char* sampler);

/**************************************************
* psfxGetEffectLog
* Input:
*   effect  -- psfx effect id
*   log     -- Destination address
*   bufSize -- Size of the buffer
**************************************************/
void psfxGetEffectLog(int effect, char* log, int bufSize);


/**************************************************
* psfxDeleteEffect
* Input:
*   effect  -- psfx effect id
**************************************************/
void psfxDeleteEffect(int effect);

}

#ifdef __cplusplus

/**************************************************
* psfxGetProgramName
* Input:
*   effect  -- psfx effect id
*   program -- Index of program
**************************************************/
const char* psfxGetProgramName(int effect, int program);

/**************************************************
* psfxGetProgramIndex
* Input:
*   effect  -- psfx effect id
*   name -- name of program
**************************************************/
size_t psfxGetProgramIndex(int effect, const char* name);

/**************************************************
* psfxGetEffectLog
* Input:
*   effect  -- psfx effect id
* Return value: Log string
**************************************************/
const char* psfxGetEffectLog(int effect);

#endif
