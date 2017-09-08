#include <map>
#include <string>
#include <cstring>
#include <sstream>	// for ostringstream
#include <cstdio>
#include <cassert>
#include <fstream>
#include <iostream>

#ifndef _MSC_VER
typedef int errno_t;
#include <errno.h>
#endif
#ifdef _MSC_VER
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <io.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "psfx.h"
#include "psfxClasses.h"
#include "psfxParser.h"

#ifdef _MSC_VER
#define YY_NO_UNISTD_H
#include <windows.h>
#endif
#include "psfxScanner.h"
#include "psfxProgram.h"
#include "StringFunctions.h"
#include "StringToWString.h"
#include "psfxEffect.h"
#include "psfxErrorCheck.h"
#include <direct.h>

//#pragma comment(lib,"libSceShaderCompiler.lib")
#pragma optimize("",off)

static string WStringToUtf8(const wchar_t *src_w)
{
	int src_length=(int)wcslen(src_w);
#ifdef _MSC_VER
	int size_needed = WideCharToMultiByte(CP_UTF8, 0,src_w, (int)src_length, NULL, 0, NULL, NULL);
#else
	int size_needed=2*src_length;
#endif
	char *output_buffer = new char [size_needed+1];
#ifdef _MSC_VER
	WideCharToMultiByte (CP_UTF8,0,src_w,(int)src_length,output_buffer, size_needed, NULL, NULL);
#else
	wcstombs(output_buffer, src_w, (size_t)size_needed );
#endif
	output_buffer[size_needed]=0;
	std::string str_utf8=std::string(output_buffer);
	delete [] output_buffer;
	return str_utf8;
}

static void FixRelativePaths(std::string &str,const string &sourcePathUtf8)
{
	int pos=0;
	int eol=(int)str.find("\n");
	if(eol<0)
		eol=(int)str.length();
	while(eol>=0)
	{
		string line=str.substr(pos,eol-pos);
		if(line[0]=='.')
		{
			line=sourcePathUtf8+line;
			str.replace(pos,eol-pos,line);
		}
		pos=(int)str.find("\n",pos+1);
		if(pos<0)
			pos=(int)str.length();
		else
			pos++;
		eol=(int)str.find("\n",pos);
	}
}

bool RunDOSCommand(const wchar_t *wcommand, const string &sourcePathUtf8, ostringstream& log)
{
	bool pipe_compiler_output=true;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );
	si.wShowWindow=false;
	wchar_t com[500];
	wcscpy(com,wcommand);
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;

	HANDLE hReadOutPipe = NULL;
	HANDLE hWriteOutPipe = NULL;
	HANDLE hReadErrorPipe = NULL;
	HANDLE hWriteErrorPipe = NULL;
	SECURITY_ATTRIBUTES saAttr; 
// Set the bInheritHandle flag so pipe handles are inherited. 
	
	if(pipe_compiler_output)
	{
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
		saAttr.bInheritHandle = TRUE; 
		saAttr.lpSecurityDescriptor = NULL; 
		CreatePipe( &hReadOutPipe, &hWriteOutPipe, &saAttr, 100 );
		CreatePipe( &hReadErrorPipe, &hWriteErrorPipe, &saAttr, 100 );
	}

	si.hStdOutput = hWriteOutPipe;
	si.hStdError= hWriteErrorPipe;
	CreateProcessW( NULL,		// No module name (use command line)
			com,				// Command line
			NULL,				// Process handle not inheritable
			NULL,				// Thread handle not inheritable
			TRUE,				// Set handle inheritance to FALSE
			CREATE_NO_WINDOW,	//CREATE_NO_WINDOW,	// No nasty console windows
			NULL,				// Use parent's environment block
			NULL,				// Use parent's starting directory 
			&si,				// Pointer to STARTUPINFO structure
			&pi )				// Pointer to PROCESS_INFORMATION structure
		;
	// Wait until child process exits.
	if(pi.hProcess==nullptr)
	{
		std::cerr<<"Error: Could not find the executable for "<<WStringToUtf8(com)<<std::endl;
		return false;
	}
	HANDLE WaitHandles[] = {
			pi.hProcess, hReadOutPipe, hReadErrorPipe
		};

	const DWORD BUFSIZE = 4096;
	BYTE buff[BUFSIZE];
	bool has_errors=false;
	while (1)
	{
		DWORD dwBytesRead, dwBytesAvailable;
		DWORD dwWaitResult = WaitForMultipleObjects(pipe_compiler_output?3:1, WaitHandles, FALSE, 60000L);

		// Read from the pipes...
		if(pipe_compiler_output)
		{
			while( PeekNamedPipe(hReadOutPipe, NULL, 0, NULL, &dwBytesAvailable, NULL) && dwBytesAvailable )
			{
			  ReadFile(hReadOutPipe, buff, BUFSIZE-1, &dwBytesRead, 0);
			  log << std::string((char*)buff, (size_t)dwBytesRead).c_str();
			}
			while( PeekNamedPipe(hReadErrorPipe, NULL, 0, NULL, &dwBytesAvailable, NULL) && dwBytesAvailable )
			{
				ReadFile(hReadErrorPipe, buff, BUFSIZE-1, &dwBytesRead, 0);
				std::string str((char*)buff, (size_t)dwBytesRead);
				size_t pos = str.find("Error");
				if(pos>=str.length())
					pos = str.find("error");
				if(pos>=str.length())
					pos=str.find("failed");
				if(pos<str.length())
					has_errors=true;
				FixRelativePaths(str,sourcePathUtf8);
				log << str.c_str();
				int bracket_pos=(int)str.find("(");
				if(bracket_pos>0)
				{
					int close_bracket_pos	=(int)str.find(")",bracket_pos);
					int comma_pos			=(int)str.find(",",bracket_pos);
					if(comma_pos>bracket_pos&&comma_pos<close_bracket_pos)
					{
						str.replace(comma_pos,close_bracket_pos-comma_pos,"");
					}
				}
			}
		}
		// Process is done, or we timed out:
		if(dwWaitResult == WAIT_OBJECT_0 || dwWaitResult == WAIT_TIMEOUT)
			break;
		if (dwWaitResult == WAIT_FAILED)
		{
			DWORD err = GetLastError();
			char* msg;
			// Ask Windows to prepare a standard message for a GetLastError() code:
			if (!FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL))
				break;

			PSFX_CERR<<"Error message: "<<msg<<std::endl;
			exit(1);
		}
	  }

		//WaitForSingleObject( pi.hProcess, INFINITE );
 		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );

		if(has_errors)
			return false;
	return true;
}

Technique::Technique(const map<std::string, Pass>& passes)
	:m_passes(passes)
{
}

Pass::Pass()
{
}

Pass::Pass(const map<ShaderType,string>& shaders,const PassState &passState)
{
	this->passState=passState;
    map<ShaderType,string>::const_iterator it;
    
    ShaderType types[NUM_SHADER_TYPES]={VERTEX_SHADER,
		TESSELATION_CONTROL_SHADER,		 
		TESSELATION_EVALUATION_SHADER,	 
		GEOMETRY_SHADER,
		FRAGMENT_SHADER,
		COMPUTE_SHADER};
    for(int i=0;i<NUM_OF_SHADER_TYPES;i++)
	{
        it=shaders.find(types[i]);
        if(it==shaders.end())
			continue;
		if(it->second==string("gsConstructed"))
			continue;
       m_compiledShaderNames[i]=it->second;
    }
}

string ToString(PixelOutputFormat pixelOutputFormat)
{
	switch(pixelOutputFormat)
	{
	case FMT_UNKNOWN:
		return "";
	case FMT_32_GR:
		return "32gr";
	case FMT_32_AR:
		return "32ar";
	case FMT_FP16_ABGR:
		return "float16abgr";
	case FMT_UNORM16_ABGR:
		return "unorm16abgr";
	case FMT_SNORM16_ABGR:
		return "snorm16abgr";
	case FMT_UINT16_ABGR:
		return "uint16abgr";
	case FMT_SINT16_ABGR:
		return "sint16abgr";
	case FMT_32_ABGR:
		return "float32abgr";
	default:
		return "invalid";
	}
}

string ToPragmaString(PixelOutputFormat pixelOutputFormat)
{
	switch(pixelOutputFormat)
	{
	case FMT_UNKNOWN:
		return "FMT_UNKNOWN";
	case FMT_32_GR:
		return "FMT_32_GR";
	case FMT_32_AR:
		return "FMT_32_AR";
	case FMT_FP16_ABGR:
		return "FMT_FP16_ABGR";
	case FMT_UNORM16_ABGR:
		return "FMT_UNORM16_ABGR";
	case FMT_SNORM16_ABGR:
		return "FMT_SNORM16_ABGR";
	case FMT_UINT16_ABGR:
		return "FMT_UINT16_ABGR";
	case FMT_SINT16_ABGR:
		return "FMT_SINT16_ABGR";
	case FMT_32_ABGR:
		return "FMT_32_ABGR";
	default:
		return "FMT_UNKNOWN";
	}
}

static int do_mkdir(const char *path_utf8)
{
    int             status = 0;
#ifdef _MSC_VER
    struct _stat64i32            st;
	std::wstring wstr=Utf8ToWString(path_utf8);
    if (_wstat (wstr.c_str(), &st) != 0)
#else
    Stat            st;
    if (stat(path_utf8, &st)!=0)
#endif
    {
        /* Directory does not exist. EEXIST for race condition */
#ifdef _MSC_VER
        if (_wmkdir(wstr.c_str()) != 0 && errno != EEXIST)
#else
        if (mkdir(path_utf8,S_IRWXU) != 0 && errno != EEXIST)
#endif
            status = -1;
    }
    else if (!(st.st_mode & S_IFDIR))
    {
        //errno = ENOTDIR;
        status = -1;
    }
	errno=0;
    return(status);
}
static int nextslash(const std::string &str,int pos)
{
	int slash=(int)str.find('/',pos);
	int back=(int)str.find('\\',pos);
	if(slash<0||(back>=0&&back<slash))
		slash=back;
	return slash;
}
static int mkpath(const std::string &filename_utf8)
{
    int status = 0;
	int pos=0;
    while (status == 0 && (pos = nextslash(filename_utf8,pos))>=0)
    {
		status = do_mkdir(filename_utf8.substr(0,pos).c_str());
		pos++;
    }
    return (status);
}

CompiledShader::CompiledShader(const std::set<int> &tSlots
						,const std::set<int> &bSlots
						,const std::set<int> &sSlots)
{
	for(auto i:tSlots)
		textureSlots.push_back(i);
	for(auto i:bSlots)
		bufferSlots.push_back(i);
	for(auto i:sSlots)
		samplerSlots.push_back(i);
}
CompiledShader::CompiledShader(const CompiledShader &cs)
{
		shaderType			=cs.shaderType			;
		m_profile			=cs.m_profile			;
		m_functionName		=cs.m_functionName		;
		m_preamble			=cs.m_preamble			;
		m_augmentedSource	=cs.m_augmentedSource	;
		sbFilenames			=cs.sbFilenames			;
		textureSlots		=cs.textureSlots		;
		bufferSlots			=cs.bufferSlots			;
		samplerSlots		=cs.samplerSlots		;
}

extern std::vector<std::string> extra_arguments;
int CompiledShader::Compile(const string &sourceFile,string targetFile,ShaderType t,PixelOutputFormat pixelOutputFormat,const string &sharedSource, ostringstream& sLog)
{
	string preamble=m_preamble;
	if(t==COMPUTE_SHADER)
	{
		preamble="#define USE_COMPUTE_SHADER 1\n";
	}
	// Pssl recognizes the shader type using a suffix to the filename, before the .pssl extension:
	wstring shaderTypeSuffix;
	switch(t)
	{
	case SetExportShader:
		shaderTypeSuffix=L"ve";
		break;
	case SetVertexShader:
		shaderTypeSuffix=L"vv";
		break;
	case SetHullShader:
		break;
	case SetDomainShader:
		break;
	case SetGeometryShader:
		shaderTypeSuffix=L"g";
		break;
	case SetPixelShader:
		{
			string frm=ToString(pixelOutputFormat);
			preamble+="#pragma PSSL_target_output_format(default ";
			preamble+=ToPragmaString(pixelOutputFormat);
			preamble+=")\n";
			if(frm=="invalid")
				return 0;
			if(frm.size())
				shaderTypeSuffix=StringToWString(frm)+L"_";
			shaderTypeSuffix+=L"p";
		}
		break;
	case SetComputeShader:
		shaderTypeSuffix=L"c";
		break;
	default:
		break;
	};
	string src=preamble+sharedSource+m_augmentedSource;
	const char *strSrc=src.c_str();
	string targetDir=GetDirectoryFromFilename(targetFile);
	if(targetDir.size())
		targetDir+="/";
	mkpath(targetDir);
	string filenameOnly = GetFilenameOnly( sourceFile);
	wstring tempFilename = StringToWString(targetDir + filenameOnly);
	int pos=(int)tempFilename.find_last_of(L".");
	if(pos>=0)
		tempFilename=tempFilename.substr(0,pos);
	tempFilename+=L"_"+StringToWString(m_functionName);
	tempFilename+=L"_"+shaderTypeSuffix+L".pssl";
	ofstream ofs(tempFilename.c_str());
	ofs.write(strSrc,strlen(strSrc));
	ofs.close();
	// Nowe delete the corresponding sdb's
	wstring sdbFile=tempFilename.substr(0,tempFilename.length()-5);
	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW((sdbFile+L"*.sdb").c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			DeleteFileW((StringToWString(targetDir)+ fd.cFileName).c_str());
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
	// We will now compile using Psslc.exe
	/*
	"D:\SCE\ORBIS SDKs\2.500\host_tools\bin\orbis-psslc.exe"
		-I"C:\Simul\master\Simul\Platform\PS4\." 
		-o "C:\Simul\master\Simul\Platform\PS4\Render\PSSL\light_altitude_interp_c.sb"
		"C:\Simul\master\Simul\Platform\PS4\Render\PSSL\light_altitude_interp_c.pssl"
		-cache -cachedir "ORBIS/VC11/Debug/\."
	*/
	// Get PlayStation(R)4 SDK path for finding the compiler.
	string sce=GetEnv("SCE_ORBIS_SDK_DIR");
	if(sce.length()==0)
	{
		std::cerr<<"Error: SCE_ORBIS_SDK_DIR is not set.";
		return false;
	}
	wstring psslc =wstring(L"\"")+Utf8ToWString(sce);

	psslc += L"\\host_tools\\bin\\orbis-wave-psslc.exe\"";// -profile  -ttrace 1 -debug -fx  -xmlcache-ast";
	
	psslc += L" -cache";
//	psslc += L" -cache";
	//psslc += L" -Od";	// only for debugging.
	for(auto i:extra_arguments)
	{
		psslc+=L" ";
		psslc+=Utf8ToWString(i);
	}
	wstring outputFile=tempFilename.substr(0,tempFilename.length()-5)+L".sb";
	int slash=(int)outputFile.find_last_of(L"/");
	int backslash=(int)outputFile.find_last_of(L"\\");
	if(backslash>slash)
		slash=backslash;
	string sbf=WStringToUtf8(outputFile.substr(slash+1,outputFile.length()-slash-1).c_str());
	if(t==SetPixelShader)
	{
		sbFilenames[pixelOutputFormat]=sbf;
	}
	else if(t==SetExportShader)
	{
		sbFilenames[1]=sbf;
	}
	else
		sbFilenames[0]=sbf;
	psslc += L" -o \"";
	psslc += outputFile;
	psslc += L"\"";
	
	psslc += L" \"";
	psslc += tempFilename.c_str();
	psslc += L"\"";
	psslc += L" -ttrace 1";
	
	char buffer[MAX_PATH];
	string effectPath="";
	if(_getcwd(buffer,_MAX_PATH))
		 effectPath=string(buffer)+"/";
	ostringstream log;
	bool res=RunDOSCommand(psslc.c_str(),effectPath,log);
	textureSlots.clear();
	bufferSlots.clear();
	samplerSlots.clear();
	if (res)
	{
		if (log.str().size())
		{
			std::cerr << (sourceFile).c_str() << "(0): Warning: warnings compiling " << m_functionName.c_str() << std::endl;
			std::cerr << WStringToUtf8(tempFilename).c_str() << "(0): Warning: Generated PSSL file " << std::endl;
			std::cerr << log.str() << std::endl;
		}
		//DeleteFileW(tempFilename.c_str());
	}
	else
	{
		std::cerr << sourceFile.c_str() << "(0): error: failed building shader " << m_functionName.c_str()<<std::endl;
		std::cerr << WStringToUtf8(tempFilename.c_str()).c_str() << "(0): warning: generated temporary pssl file for " << m_functionName.c_str()<<std::endl;
		std::cerr << log.str() << std::endl;
	}
	return res;
	/*
	//using the libraries?
	sce::Shader::Compiler::Options options;
	options.mainSourceFile=tempFilename.c_str();
	sce::Shader::Compiler::CallbackList callbacks;
	const sce::Shader::Compiler::Output *output=sce::Shader::Compiler::run(&options,&callbacks);*/
}