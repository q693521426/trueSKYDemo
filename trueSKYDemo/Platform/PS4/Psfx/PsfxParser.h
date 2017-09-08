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

#include <string>
#include <map>
#include <vector>

#include "psfxClasses.h"

using namespace std;
using namespace psfxParser;
/// Values that represent SamplerParam.
enum SamplerParam
{
    SAMPLER_PARAM_STRING,
    SAMPLER_PARAM_INT,
    SAMPLER_PARAM_FLOAT
};
/// Values that represent RegisterParamType.
enum RegisterParamType
{
    REGISTER_NONE,
    REGISTER_INT,
    REGISTER_NAME
};

struct ShaderParameterType
{
	union
	{
        int num;
        float fnum;
        bool boolean;
	};
	std::string str;
};

struct psfxstype
{
    psfxstype() {}

    struct variable
	{
		union
		{
			int num;
			unsigned int unum;
			float fnum;
			bool boolean;
			float vec4[4];
		};
        string storage;
        string type;
        string identifier;
        string templ;
        string semantic;
    };

    struct samplerVar
	{
        string  binding;
        string  name;
    };

	int								lineno;
	int token;
    union
	{
        int			num;
        unsigned	unum;
        float		fnum;
        bool		boolean;
		Technique *	tech;
        Pass*		prog;

		map<string, Pass>*			passes;
        map<ShaderType, string>*	shaders;
        vector<variable>*			vars;
        vector<samplerVar>*			texNames;
    };
    
    union
	{
        SamplerParam		samplerParamType;
        ShaderType			sType;
        ShaderCommand		sCommand;
		RenderStateType		sRenderStateType;
        RegisterParamType	rType;
    };

    // Carrying these around is bad luck, or more like bad performance. But whatever...
    string strs[5];
};

namespace psfxParser
{
	extern RenderStateType renderStateType;
	extern bool gLexPassthrough;
	extern bool read_shader;

#ifdef LINUX
int fopen_s(FILE** pFile, const char *filename, const char *mode);
#endif

	extern BlendOption toBlend(const std::string &str);
	extern BlendOperation toBlendOp(const std::string &str);
	extern DepthComparison toDepthFunc(const std::string &str);
	extern FillMode toFillMode(const std::string &str);
	extern CullMode toCullMode(const std::string &str);

}
#undef YYSTYPE
#define YYSTYPE psfxstype

#if 1
#define YYDEBUG 0
extern int psfxdebug;
#endif

string psfxreadblock(unsigned char openChar, unsigned char closeChar);
void glfxerror(const char*);
int glfxparse();
void psfxReset();
bool getPsfxError();
extern std::string current_filename;
extern void SetLexStartState(int);
extern const char *GetStartModeText();
extern bool is_equal(const string& a, const char * b);
