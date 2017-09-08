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

#include <map>
#include <string>
#include <cstring>
#include <sstream>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <time.h>

#ifndef _MSC_VER
typedef int errno_t;
#include <errno.h>
#endif

// workaround for Linux distributions that haven't yet upgraded to GLEW 1.9
#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif

#include "psfx.h"
#include "psfxClasses.h"
#include "psfxParser.h"
#include "psfxEffect.h"

#ifdef _MSC_VER
#define YY_NO_UNISTD_H
#include <windows.h>
#endif
#include "psfxScanner.h"
#include "psfxProgram.h"
#include "StringToWString.h"
#include "StringFunctions.h"
#include "FileLoader.h"

#include "Preprocessor.h"
#include "PreprocessLexer.h"
#include "PreprocessParser.h"
// These are the callback functions for file handling that we will send to the preprocessor.
//extern FILE* (*prepro_open)(const char *filename_utf8,std::string &fullPathName,double &time);
//extern void (*prepro_close)(FILE *f);
vector<string> shaderPathsUtf8;
FileLoader fileLoader;
extern std::ostringstream preproOutput;

static double GetDayNumberFromDateTime(int year,int month,int day,int hour,int min,int sec)
{
    int D = 367*year - (7*(year + ((month+9)/12)))/4 + (275*month)/9 + day - 730531;//was +2451545
	double d=(double)D;
	d+=(double)hour/24.0;
	d+=(double)min/24.0/60.0;
	d+=(double)sec/24.0/3600.0;
	return d;
}

double GetFileDate(const std::string &fullPathNameUtf8)
{
	wstring filenamew=StringToWString(fullPathNameUtf8);
	struct _stat buf;
	_wstat(filenamew.c_str(),&buf);
	buf.st_mtime;
	time_t t = buf.st_mtime;
	struct tm lt;
	gmtime_s(&lt,&t);
	double datetime=GetDayNumberFromDateTime(1900+lt.tm_year,lt.tm_mon,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec);
	return datetime;
}		   	
		   	
FILE* OpenFile(const char *filename_utf8,std::string &fullPathNameUtf8,double &datetime)
{		   	
	fullPathNameUtf8	=fileLoader.FindFileInPathStack(filename_utf8,shaderPathsUtf8);
	if(!fullPathNameUtf8.length())
		return NULL;
	wstring filenamew=StringToWString(fullPathNameUtf8);
	FILE *f=_wfopen(filenamew.c_str(),L"r");
	string path=fullPathNameUtf8;
	int last_slash=(int)path.find_last_of("/");
	int last_bslash=(int)path.find_last_of("\\");
	if(last_bslash>last_slash)
		last_slash=last_bslash;
	if(last_slash>0)
		path=path.substr(0,last_slash);
	shaderPathsUtf8.push_back(path);
	datetime=GetFileDate(fullPathNameUtf8);
	return f;
}

void CloseFile(FILE *f)
{
	if(f)
	{
	// Pop the current path of this file.
		shaderPathsUtf8.pop_back();
		fclose(f);
	}
}

using namespace std;

psfxParser::BlendOption psfxParser::toBlend(const std::string &str)
{
	if(is_equal(str,"SRC_ALPHA"))
		return BLEND_SRC_ALPHA;
	else if(is_equal(str,"INV_SRC_ALPHA"))
		return BLEND_INV_SRC_ALPHA;
	else if(is_equal(str,"ZERO"))
		return BLEND_ZERO;
	else if(is_equal(str,"ONE"))
		return BLEND_ONE;
	else if(is_equal(str,"SRC_COLOR"))
		return BLEND_SRC_COLOR;
	else if(is_equal(str,"INV_SRC_COLOR"))
		return BLEND_INV_SRC_COLOR;
	else if(is_equal(str,"DEST_ALPHA"))
		return BLEND_DEST_ALPHA;
	else if(is_equal(str,"INV_DEST_ALPHA"))
		return BLEND_INV_DEST_ALPHA;
	else if(is_equal(str,"DEST_COLOR"))
		return BLEND_DEST_COLOR;
	else if(is_equal(str,"INV_DEST_COLOR"))
		return BLEND_INV_DEST_COLOR;
	else if(is_equal(str,"SRC_ALPHA_SAT"))
	{
		ostringstream ostr;
		ostr<<"unknown blend type: "<<str;
		glfxerror(ostr.str().c_str());
		return BLEND_ONE;
	}
	else if(is_equal(str,"BLEND_FACTOR"))
	{
		return BLEND_BLEND_FACTOR;
	}
	else if(is_equal(str,"INV_BLEND_FACTOR"))
	{
		return BLEND_INV_BLEND_FACTOR;
	}
	else if(is_equal(str,"SRC1_COLOR"))
		return BLEND_SRC1_COLOR;
	else if(is_equal(str,"INV_SRC1_COLOR"))
		return BLEND_INV_SRC1_COLOR;
	else if(is_equal(str,"SRC1_ALPHA"))
		return BLEND_SRC1_ALPHA;
	else if(is_equal(str,"INV_SRC1_ALPHA"))
		return BLEND_INV_SRC1_ALPHA;
	else
	{
		ostringstream ostr;
		ostr<<"unknown blend type: "<<str;
		glfxerror(ostr.str().c_str());
	}
	return BLEND_ONE;
}

psfxParser::BlendOperation psfxParser::toBlendOp(const std::string &str)
{
	if(is_equal(str,"ADD"))
		return BLEND_OP_ADD;
	else if(is_equal(str,"SUBTRACT"))
		return BLEND_OP_SUBTRACT;
	else if(is_equal(str,"MAX"))
		return BLEND_OP_MAX;
	else if(is_equal(str,"MIN"))
		return BLEND_OP_MIN;
	else
	{
		ostringstream ostr;
		ostr<<"unknown blend operation: "<<str;
		glfxerror(ostr.str().c_str());
	}
	return BLEND_OP_ADD;
}

FillMode psfxParser::toFillMode(const std::string &str)
{
	if(is_equal(str,"WIREFRAME"))
		return FILL_WIREFRAME;
	else if(is_equal(str,"SOLID"))
		return FILL_SOLID;
	else if(is_equal(str,"NONE"))
		return FILL_SOLID;
	else
	{
		ostringstream err;
		err<<"unknown fill mode: "<<str;
		glfxerror(err.str().c_str());
	}
	return FILL_SOLID;
}

CullMode psfxParser::toCullMode(const std::string &str)
{
	if(is_equal(str,"FRONT"))
		return CULL_FRONT;
	else if(is_equal(str,"BACK"))
		return CULL_BACK;
	else if(is_equal(str,"NONE"))
		return CULL_NONE;
	else
	{
		ostringstream err;
		err<<"unknown cull mode: "<<str;
		glfxerror(err.str().c_str());
	}
	return CULL_NONE;
}

DepthComparison psfxParser::toDepthFunc(const std::string &str)
{
	if(is_equal(str,"ALWAYS"))
		return DEPTH_ALWAYS;
	else if(is_equal(str,"NEVER"))
		return DEPTH_NEVER;
	else if(is_equal(str,"LESS"))
		return DEPTH_LESS;
	else if(is_equal(str,"GREATER"))
		return DEPTH_GREATER;
	else if(is_equal(str,"LESS_EQUAL"))
		return DEPTH_LESS_EQUAL;
	else if(is_equal(str,"GREATER_EQUAL"))
		return DEPTH_GREATER_EQUAL;
	else
	{
		ostringstream ostr;
		ostr<<"unknown depth function: "<<str;
		glfxerror(ostr.str().c_str());
	}
	return DEPTH_ALWAYS;
}


BlendState::BlendState():SrcBlend(BLEND_SRC_ALPHA)	
					,DestBlend(BLEND_INV_SRC_ALPHA)
					,BlendOp(BLEND_OP_ADD)		
					,SrcBlendAlpha(BLEND_SRC_ALPHA)
					,DestBlendAlpha(BLEND_INV_SRC_ALPHA)
					,BlendOpAlpha(BLEND_OP_ADD)	
					,AlphaToCoverageEnable(false)
{
}

DepthStencilState::DepthStencilState()
	:DepthEnable(true)
	,DepthWriteMask(1)
	,DepthFunc(DEPTH_LESS_EQUAL)
{}

RasterizerState::RasterizerState()
	:	fillMode(FILL_SOLID)
		,cullMode(CULL_FRONT)
		,FrontCounterClockwise(true)
		,DepthBias(0)
		,DepthBiasClamp(0.0f)
		,SlopeScaledDepthBias(0.0f)
		,DepthClipEnable(false)
		,ScissorEnable(false)
		,MultisampleEnable(false)
		,AntialiasedLineEnable(false)
{
}

RenderStateType	 renderState;
#pragma optimize("",off)
static std::string RewriteErrorLine(std::string line,const vector<string> &sourceFilesUtf8)
{
	bool is_error=true;
	int errpos=(int)line.find("ERROR");
	if(errpos<0)
		errpos=(int)line.find("error");
	if(errpos<0)
	{
		errpos=(int)line.find("WARNING");
		is_error=false;
	}
	if(errpos<0)
	{
		errpos=(int)line.find("warning");
		is_error=false;
	}
	if(errpos>=0)
	{
		int first_colon		=(int)line.find(":");
		int second_colon	=(int)line.find(":",first_colon+1);
		int third_colon		=(int)line.find(":",second_colon+1);
		int first_bracket	=(int)line.find("(");
		int second_bracket	=(int)line.find(")",first_bracket+1);
		int numberstart,numberlen=0;
	//somefile.glsl(263): error C2065: 'space_' : undeclared identifier
		if(third_colon>=0&&second_colon>=0&&(second_colon-first_colon)<5)
		{
			numberstart	=first_colon+1;
			numberlen	=second_colon-first_colon-1;
		}
	//	ERROR: 0:11: 'assign' :  cannot convert from '2-component vector of float' to 'float'
		else if((third_colon<0||numberlen>6)&&second_bracket>=0)
		{
			if(first_colon<first_bracket)
			{
				numberstart	=first_colon+1;
				numberlen	=first_bracket-first_colon-1;
			}
			else
			{
				numberstart=0;
				numberlen=first_bracket;
			}
		}
		else
		{
			numberstart=0;
			numberlen=first_bracket;
		}
		if(numberlen>0)
		{
			return line;
		}
	}
	return "";
}
namespace psfxParser
{

#ifndef _MSC_VER

errno_t strcpy_s(char* dst, size_t size, const char* src)
{
    assert(size >= (strlen(src) + 1));
    strncpy(dst, src, size-1);
    dst[size-1]='\0';
    return errno;
}

int fopen_s(FILE** pFile, const char *filename, const char *mode)
{
    *pFile = fopen(filename, mode);
    return errno;
}

int fdopen_s(FILE** pFile, int fildes, const char *mode)
{
    *pFile = fdopen(fildes, mode);
    return errno;
}

#endif

Effect *gEffect=NULL;
bool gLexPassthrough=true;
bool read_shader=false;
RenderStateType renderStateType=NUM_RENDERSTATE_TYPES;

vector<Effect*> gEffects;

} // psfxParser

using namespace psfxParser;

int psfxGenEffect()
{
    gEffects.push_back(new Effect);
    return (int)gEffects.size()-1;
}

std::string loadShaderSource(const char *filename_utf8,const vector<string> &shaderPathsUtf8)
{
	void *shader_source=NULL;
	unsigned fileSize=0;
	fileLoader.AcquireFileContents(shader_source,fileSize,filename_utf8,true);

	if(!shader_source)
	{
		std::cerr<<"\nERROR:\tShader file "<<filename_utf8<<" not found, exiting.\n";
		std::cerr<<"\n\t\tShader paths are:"<<std::endl;
		for(int i=0;i<(int)shaderPathsUtf8.size();i++)
			std::cerr<<" "<<shaderPathsUtf8[i].c_str()<<std::endl;
		;
		std::cerr<<"exit(1)"<<std::endl;
		exit(1);
	}
	
	std::string str((const char*)shader_source);
	fileLoader.ReleaseFileContents(shader_source);
	
	// Change Windows-style CR-LF's to simple Unix-style LF's
	find_and_replace( str,"\r\n","\n");
	// Convert any left-over CR's into LF's
	find_and_replace( str,"\r","\n");
	
	return str;
}

void ProcessIncludes(string &src,string &filenameUtf8,bool line_source_filenames,vector<string> &sourceFilesUtf8,const vector<string> &shaderPathsUtf8)
{
	size_t pos			=0;
	// problem: if we insert this at line 0, SOME Glsl compilers will moan about #version not being the first line.
	//src					=src.insert(0,base::stringFormat("#line 1 \"%s\"\n",filenameUtf8.c_str()));

	// instead we find '#version' and insert after that.
	int first			=(int)src.find("#version");
	if(first>=0)
		pos				=src.find('\n',first)+1;
	// Is this file in the source list?
	int index			=(int)(find(sourceFilesUtf8.begin(), sourceFilesUtf8.end(), filenameUtf8)-sourceFilesUtf8.begin());
	// And SOME Glsl compilers will give a syntax error
	// when you provide a source filename with the #line directive, as it's not in the GLSL spec:
	src				=src.insert(pos,stringFormat("#line 1 \"%s\"\n",filenameUtf8.c_str()));
	
	pos					=src.find('\n',pos+1);
	int next			=(int)src.find('\n',pos+1);
	int line_number		=0;
	while(next>=0)
	{
		string line						=src.substr(pos+1,next-pos);
		int inc							=(int)line.find("#include");
		if(inc==0)
		{
			int start_of_line			=(int)pos+1;
			pos+=9;
			int eol						=(int)src.find("\n",pos+1);
			string include_file			=line.substr(10,line.length()-12);
			src							=src.insert(start_of_line,"//");
			// Go to after the newline at the end of the #include statement. Two for "//" and one for "\n"
			eol							+=3;
			string includeFilenameUtf8	=fileLoader.FindFileInPathStack(include_file.c_str(),shaderPathsUtf8);
			string newsrc				=loadShaderSource(includeFilenameUtf8.c_str(),shaderPathsUtf8);
			ProcessIncludes(newsrc,includeFilenameUtf8,line_source_filenames,sourceFilesUtf8,shaderPathsUtf8);
			
			//First put the "restore" #line directive after the commented-out #include.
			src=src.insert(eol,stringFormat("\n#line %d \"%s\"\n",line_number+1,filenameUtf8.c_str()));
			// Now insert the contents of the #include file before the closing #line directive.
			src								=src.insert(eol,"\n");
			src								=src.insert(eol,newsrc);
			next							+=(int)newsrc.length();
			line_number--;
		}
		else
			line_number++;
		pos=next;
		next=(int)src.find('\n',pos+1);
	}
}

std::string GetExecutableDirectory()
{
	std::wstring str;
#if defined(WIN32)|| defined(WIN64)
	wchar_t filename[_MAX_PATH];
	if(GetModuleFileNameW(NULL,filename,_MAX_PATH))
	{
		str=filename;
		int pos=(int)str.find_last_of('/');
		int back=(int)str.find_last_of('\\');
		if(back>pos)
			pos=back;
		str=str.substr(0,pos);
	}
	else
		str=L"";
#endif
	return WStringToUtf8(str);
}
std::vector<std::string> extra_arguments;
bool psfxParseEffectFromFile(int effect, const char* file,const char **paths,const char *outputfile,const char **args)
{
    bool retVal=true;
	const char *filenamesUtf8[]={file,NULL};
	gEffects[effect]->SetFilenameList(filenamesUtf8);
	const char **p=paths;
	while(p&&*p&&shaderPathsUtf8.size()<100)
	{
		shaderPathsUtf8.push_back(*p);
		p++;
	}
	const char **a=args;
	while(a&&*a&&extra_arguments.size()<100)
	{
		extra_arguments.push_back(*a);
		a++;
	}
	string psfxoFilename	=GetFilenameOnly(file);
	int dotpos				=(int)psfxoFilename.find_last_of(".");
	if(dotpos>0)
		psfxoFilename.replace(dotpos,psfxoFilename.length()-dotpos,".psfxo");
	else
		psfxoFilename+=".psfxo";
	if(outputfile!=NULL&&strlen(outputfile)>0)
	{
		string outf=outputfile;
		// Always assume it's a directory.
		if(outf.find_last_of(".psfxo")!=outf.length()-6)
		{
			if(outf.length()>0)
			outf+="\\";
			outf+=psfxoFilename;
		}
		psfxoFilename=outf;
	}
	else
	{
		string shaderbin(file);
		int slash_pos=(int)shaderbin.find_last_of("/");
		if(slash_pos<0)
			slash_pos=(int)shaderbin.find_last_of("\\");
		if(slash_pos<0)
			slash_pos=0;

		shaderbin=shaderbin.substr(0,slash_pos);
		if(shaderbin.length())
			shaderbin+="/";
		psfxoFilename=shaderbin+psfxoFilename;
	}

	
	vector<string> sourceFilesUtf8;
	string exedir = GetExecutableDirectory();
	shaderPathsUtf8.push_back(exedir);
	shaderPathsUtf8.push_back(".");
	shaderPathsUtf8.push_back(exedir+"/PSSL");
	shaderPathsUtf8.push_back(exedir+"/Render/PSSL");
	shaderPathsUtf8.push_back(GetDirectoryFromFilename(file));
	string newsrc				=loadShaderSource(file,shaderPathsUtf8);
	try
	{
		prepro_open=&OpenFile;
		prepro_close=&CloseFile;

		char exeNameUtf8[_MAX_PATH];
		DWORD res=GetModuleFileNameA(NULL, exeNameUtf8,_MAX_PATH);
		// start with the date that this exe was made, so new exe's rebuild the shaders.
		latest_datetime= GetFileDate(exeNameUtf8);

		map<string,string> defines;
		defines["__PSSL__"]="1";
		preprocess(file,defines);
		double output_filedatetime=GetFileDate(psfxoFilename);
		if(latest_datetime<output_filedatetime)
			return true;
		newsrc=preproOutput.str();
	}
	catch(...)
	{
	}
	return psfxParseEffectFromMemory(effect,newsrc.c_str(),file,psfxoFilename.c_str());
}

bool psfxParseEffectFromMemory( int effect, const char* src,const char *filename,const char *output_filename)
{
    bool retVal=true;
    try
	{
        gEffect=gEffects[effect];
        gEffect->Dir()="";
		if(filename)
		  gEffect->Filename()=filename;
		current_filename=filename;
		psfxReset();
        glfx_scan_string(src);
        glfxset_lineno(1);
        retVal&=!glfxparse();
		retVal&=!getPsfxError();
    }
    catch(const char* err)
	{
        gEffect->Log()<<err<<endl;
        gEffect->Active()=false;
        retVal=false;
    }
    catch(const string& err)
	{
        gEffect->Log()<<err<<endl;
        gEffect->Active()=false;
        retVal=false;
    }
    catch(std::runtime_error& err)
	{
		gEffect->Log()<<err.what()<<endl;
        gEffect->Active()=false;
        retVal=false;
    }
    catch(...)
	{
        gEffect->Log()<<"Unknown error occurred during parsing of source"<<endl;
        gEffect->Active()=false;
        retVal=false;
    }

    glfxpop_buffer_state();
	
	if(retVal)
	{
		retVal&= gEffect->GenerateShaderBinaries(filename,output_filename);
	}
	string &slog=gEffect->Log().str();
	// now rewrite log to use filenames.
	string newlog;
	if(slog.find("No errors")>=slog.length())
	{
		int pos=0;
		int next=(int)slog.find('\n',pos+1);
		while(next>=0)
		{
			std::string line		=slog.substr(pos,next-pos);
			std::string error_line	=RewriteErrorLine(line,gEffects[effect]->GetFilenameList());
			if(error_line.length())
			{
				newlog+=error_line+"\n";
			}
			pos=next+1;
			next=(int)slog.find('\n',pos);
		}
	}
	gEffects[effect]->Log().str("");
    gEffects[effect]->Log()<<newlog;
    return retVal;
}

void psfxDeleteEffect(int effect)
{
    if((size_t)effect<gEffects.size() && gEffects[effect]!=NULL) {
        if(gEffect==gEffects[effect])
            gEffect=NULL;
        delete gEffects[effect];
        gEffects[effect]=NULL;
    }
}

void psfxGetEffectLog(int effect, char* log, int bufSize)
{
    if((size_t)effect>=gEffects.size() || gEffects[effect]==NULL)
        return;

    if(!strcpy_s(log, bufSize, gEffects[effect]->Log().str().c_str()))
        gEffects[effect]->Log().str("");
}

static string gLog;
const char* psfxGetEffectLog(int effect)
{
    if((size_t)effect>=gEffects.size() || gEffects[effect]==NULL)
        return "";

    gLog=gEffects[effect]->Log().str();
    gEffects[effect]->Log().str("");
    return gLog.c_str();
}

int psfxGetProgramCount(int effect)
{
    return (int)gEffects[effect]->GetProgramList().size();
}

void psfxGetProgramName(int effect, int program, char* name, int bufSize)
{
    const vector<string>& tmpList = gEffects[effect]->GetProgramList();
    if(program > (int)tmpList.size())
        return;
    strcpy_s(name, bufSize, tmpList[program].c_str());
}

const char* psfxGetProgramName(int effect, int program)
{
    const vector<string>& tmpList = gEffects[effect]->GetProgramList();
    if(program > (int)tmpList.size())
        return "";
    return tmpList[program].c_str();
}

size_t psfxGetProgramIndex(int effect, const char* name)
{
    const vector<string>& tmpList = gEffects[effect]->GetProgramList();
    for(int i=0;i<(int)tmpList.size();i++)
	{
		if(strcmp(tmpList[i].c_str(),name)==0)
	        return i;
	}
    return tmpList.size();
}
