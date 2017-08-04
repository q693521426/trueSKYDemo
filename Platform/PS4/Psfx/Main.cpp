
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
//

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <SDKDDKVer.h>

#include <stdio.h>
#include <vector>
#include "psfx.h"
#include "VisualStudioDebugOutput.h"
VisualStudioDebugOutput d(true);
	std::string outputfile;
#pragma optimize("",off)
int main(int argc, char** argv) 
{
    int ret = 1;
    int effect;
	//std::cout<<("psfx cout");
	//std::cerr<<"psfx cerr";
    
    if (argc <2)
	{
        std::cout<<"Usage: psfx.exe <effect file> <program name>\n"<<argv[0];
        goto error_exit;
    }
    
    effect = psfxGenEffect();
    
    char log[10000];
	const char **paths=NULL;
	const char *sourcefile="";
	const char **args=NULL;
	if(argc>1) 
	{
		paths=new const char *[argc];
		args=new const char *[argc];
		int n=0;
		int a=0;
		for(int i=1;i<argc;i++)
		{
			//printf(argv[i]);
		//	std::cerr<<(argv[i]);
			if(strlen(argv[i])>=2&&(argv[i][0]=='/'||argv[i][0]=='-'))
			{
				const char *arg=argv[i]+2;
				for(int j=0;j<100;j++)
				{
					if(*arg==0||*arg!='='||*arg!=' ')
						break;
					arg++;
				}
				char argtype=argv[i][1];
				if(argtype=='i'||argtype=='I')
					paths[n++]=arg;
				else if(argtype=='o')
					outputfile=arg;
				else
					args[a++]=argv[i];

			}
			else
				sourcefile=argv[i];
		}
		paths[n]=0;
		args[a]=0;
	}
	if(strlen(sourcefile)==0)
	{
        std::cerr<<("No source file from args :\n");
        psfxGetEffectLog(effect, log, sizeof(log));
        std::cerr<<log<<std::endl;
        goto error_exit;
    }
    if (!psfxParseEffectFromFile(effect,sourcefile,paths,outputfile.c_str(),args))
	{
        std::cerr<<("Error creating effect:\n");
        psfxGetEffectLog(effect, log, sizeof(log));
        std::cerr<<log<<std::endl;
        goto parse_error;
    }
    /*
    if (!psfxCompileProgram(effect, argv[2]) )
	{
        printf("Error compiling program '%s':\n", argv[2]);
        psfxGetEffectLog(effect, log, sizeof(log));
        printf("%s\n", log);
        goto compile_error;
    }*/
    
    std::cout<<("Compiled successfully\n");
             
    ret = 0;
    
parse_error:
    psfxDeleteEffect(effect);

error_exit:    
    return ret;
}
