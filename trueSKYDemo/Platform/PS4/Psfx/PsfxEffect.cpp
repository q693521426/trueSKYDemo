#include <map>
#include <string>
#include <cstring>
#include <sstream>
#include <cstdio>
#include <cassert>
#include <fstream>
#include <set>

#ifndef _MSC_VER
typedef int errno_t;
#include <errno.h>
#endif
#include <iostream>


#include "psfx.h"
#include "psfxClasses.h"
#include "psfxParser.h"
#include "psfxEffect.h"

#ifdef _MSC_VER
#define YY_NO_UNISTD_H
#endif
#include "psfxScanner.h"
#include "psfxProgram.h"
#include "StringFunctions.h"

Effect::Effect()
    : m_includes(0)
    , m_active(true)
{}

Effect::~Effect()
{
    for(map<string,Pass*>::iterator it=m_programs.begin(); it!=m_programs.end(); ++it)
        delete it->second;
	for(auto i:m_declaredFunctions)
	{
		for(auto j:i.second)
			delete j;
	}
}

Function *Effect::DeclareFunction(const std::string &functionName, Function &buildFunction)
{
	Function *f				=new Function;
	*f						=buildFunction;
	m_declaredFunctions[functionName].push_back(f);
	return f;
}
Function *Effect::GetFunction(const std::string &functionName,int i)
{
	auto f=m_declaredFunctions.find(functionName);
	if(f==m_declaredFunctions.end())
		return nullptr;
	if(i<0||i>=f->second.size())
		return nullptr;
	return f->second[i];
}

int Effect::GetSlot(const std::string &variableName) const
{
	auto i=m_textureDeclarations.find(variableName);
	if(i!=m_textureDeclarations.end())
	{
		return i->second.slot;
	}
	return -1;
}

bool& Effect::Active()
{
    return m_active;
}

string& Effect::Filename()
{
    return m_filename;
}

string& Effect::Dir()
{
    return m_dir;
}


unsigned Effect::CompileAllShaders(string psfxoFilename,const string &sharedCode,string& log) 
{
    ostringstream sLog;
    int res=1;
	PixelOutputFormat pixelOutputFormat=FMT_UNKNOWN;
    ShaderCommand types[NUM_OF_SHADER_TYPES]={SetVertexShader,SetHullShader,SetDomainShader,SetGeometryShader,SetPixelShader,SetComputeShader};
    for(CompiledShaderMap::iterator i=m_compiledShaders.begin();i!=m_compiledShaders.end();i++)
	{
		if(i->second->shaderType==SetPixelShader)
		{
			res&=i->second->Compile(Filename(),psfxoFilename,i->second->shaderType,FMT_32_ABGR,sharedCode, sLog);
			res&=i->second->Compile(Filename(),psfxoFilename,i->second->shaderType,FMT_FP16_ABGR,sharedCode, sLog);
			res&=i->second->Compile(Filename(),psfxoFilename,i->second->shaderType,FMT_UNORM16_ABGR,sharedCode, sLog);
			res&=i->second->Compile(Filename(),psfxoFilename,i->second->shaderType,FMT_SNORM16_ABGR,sharedCode, sLog);

			// Possibly also:
			//FMT_32_GR 
			//FMT_32_AR 
			//FMT_UINT16_ABGR 
			//FMT_SINT16_ABGR 

			if(!res)
				return 0;
		}
		else if(i->second->shaderType==SetVertexShader)
		{
			res&=i->second->Compile(Filename(),psfxoFilename,VERTEX_SHADER,pixelOutputFormat,sharedCode, sLog);
			if(!res)
				return 0;
			res&=i->second->Compile(Filename(),psfxoFilename,EXPORT_SHADER,pixelOutputFormat,sharedCode, sLog);
			if(!res)
				return 0;
		}
		else
		{
			res&=i->second->Compile(Filename(),psfxoFilename,i->second->shaderType,pixelOutputFormat,sharedCode, sLog);
			if(!res)
				return 0;
		}
    }
    log=sLog.str();
    return res;
}

ostringstream& Effect::Log()
{
    return m_log;
}

TechniqueGroup &Effect::GetTechniqueGroup(const std::string &name)
{
	return m_techniqueGroups[name];
}

const vector<string>& Effect::GetProgramList() const
{
    return m_programNames;
}

const vector<string>& Effect::GetFilenameList() const
{
    return m_filenames;
}


void Effect::SetFilenameList(const char **filenamesUtf8)
{
	m_filenames.clear();
   const char **f=filenamesUtf8;
	while(*f!=NULL)
	{
		string str=*f;
		int pos=(int)str.find("\\");
		while(pos>0)
		{
			str.replace(str.begin()+pos,str.begin()+pos+1,"/");
			pos=(int)str.find("\\");
		}
		m_filenames.push_back(str);
		f++;
	}
}

void Effect::PopulateProgramList()
{
    m_programNames.clear();
    for(map<string,Pass*>::const_iterator it=m_programs.begin(); it!=m_programs.end(); ++it)
        m_programNames.push_back(it->first);
}

string stringOf(ShaderCommand t)
{
	switch(t)
	{
	case SetVertexShader:
		return "vertex";
	case SetHullShader:
		return "hull";
	case SetDomainShader:
		return "domain";
	case SetGeometryShader:
		return "geometry";
	case SetPixelShader:
		return "pixel";
	case SetComputeShader:
		return "compute";
	case SetExportShader:
		return "export";
	default:
		return "";
	};
}
extern string ToString(PixelOutputFormat pixelOutputFormat);

bool Effect::GenerateShaderBinaries(string sfxFilename,string psfxoFilename)
{
	PopulateProgramList();
	string log;
	std::set<int> usedTextureSlots;
	std::set<int> rwTextureSlots;
	for(auto t=m_textureDeclarations.begin();t!=m_textureDeclarations.end();++t)
	{
		if(t->second.writeable)
			rwTextureSlots.insert(t->second.slot);
		else
			usedTextureSlots.insert(t->second.slot);
	}
	string sharedCode=m_sharedCode.str();
	for(auto t=m_textureDeclarations.begin();t!=m_textureDeclarations.end();++t)
	{
		TextureDetails &td=t->second;
		if(td.slot<0)
		{
			td.slot=0;
			while(td.slot<32)
			{
				if(td.writeable)
				{
					if(rwTextureSlots.find(td.slot)==rwTextureSlots.end())
					{
						rwTextureSlots.insert(td.slot);
						break;
					}
				}
				else
				{	
					if(usedTextureSlots.find(td.slot)==usedTextureSlots.end())
					{
						usedTextureSlots.insert(td.slot);
						break;
					}
				}
				td.slot++;
			}
		}
		string type=string(td.writeable?"u":"t");
		find_and_replace(sharedCode,type+string("####")+t->first+"####",type+std::to_string(td.slot));
	}
	int res					=CompileAllShaders(psfxoFilename,sharedCode,log);
	if(!res)
		return 0;
	// Now we will write a psfxo definition file that enumerates all the techniques and their shader filenames.
//	ostringstream outstr;
	ofstream outstr(psfxoFilename);
	vector<std::string>  m_techniqueNames;
	for(auto t=m_textureDeclarations.begin();t!=m_textureDeclarations.end();++t)
	{
		TextureDetails &td=t->second;
		string rw=td.writeable?"read_write":"read_only";
		string ar=td.is_array?"array":"single";
		outstr<<"texture "<<t->first<<" ";
		if(td.is_cubemap)
			outstr<<"cubemap ";
		else
			outstr<<td.dimensions<<"d ";
		outstr<<rw<<" "<<td.slot<<" "<<ar<<std::endl;
		if(td.slot>=16)
		{
			std::cerr << sfxFilename.c_str() << "(0): error: by default, only 16 texture slots are enabled in Gnmx."<<std::endl;
			std::cerr << psfxoFilename.c_str() << "(0): warning: See output."<<std::endl;
			exit(1);
		}
	}
	for(map<string,SamplerState*>::const_iterator t=m_samplerStates.begin();t!=m_samplerStates.end();++t)
	{
		outstr<<"SamplerState "<<t->first<<" "
			<<(t->second->register_number)
			<<","<<ToString(t->second->Filter)
			<<","<<ToString(t->second->AddressU)
			<<","<<ToString(t->second->AddressV)
			<<","<<ToString(t->second->AddressW)
			<<","<<"\n";
	}
	for(map<string,RasterizerState*>::const_iterator t=m_rasterizerStates.begin();t!=m_rasterizerStates.end();++t)
	{
		const RasterizerState *b=t->second;
		outstr<<"RasterizerState "<<t->first<<" ("
			<<ToString(b->AntialiasedLineEnable)
			<<","<<ToString(b->cullMode)
			<<","<<ToString(b->DepthBias)
			<<","<<ToString(b->DepthBiasClamp)
			<<","<<ToString(b->DepthClipEnable)
			<<","<<ToString(b->fillMode)
			<<","<<ToString(b->FrontCounterClockwise)
			<<","<<ToString(b->MultisampleEnable)
			<<","<<ToString(b->ScissorEnable)
			<<","<<ToString(b->SlopeScaledDepthBias)
			;
		outstr<<std::dec<<")"<<"\n";
	}
	for(map<string,BlendState*>::const_iterator t=m_blendStates.begin();t!=m_blendStates.end();++t)
	{
		const BlendState *b=t->second;
		outstr<<"BlendState "<<t->first<<" "
			<<ToString(b->AlphaToCoverageEnable)
			<<",(";
		for(auto u=b->BlendEnable.begin();u!=b->BlendEnable.end();u++)
		{
			if(u!=b->BlendEnable.begin())
				outstr<<",";
			outstr<<ToString(u->second);
		}
		outstr<<"),"<<ToString(b->BlendOp)
			<<","<<ToString(b->BlendOpAlpha)
			<<","<<ToString(b->SrcBlend)
			<<","<<ToString(b->DestBlend)
			<<","<<ToString(b->SrcBlendAlpha)
			<<","<<ToString(b->DestBlendAlpha)<<",("<<std::hex;
		for(auto v=b->RenderTargetWriteMask.begin();v!=b->RenderTargetWriteMask.end();v++)
		{
			if(v!=b->RenderTargetWriteMask.begin())
				outstr<<",";
			outstr<<ToString(v->second);
		}
		outstr<<std::dec<<")"<<"\n";
	}
	for(map<string,DepthStencilState*>::const_iterator t=m_depthStencilStates.begin();t!=m_depthStencilStates.end();++t)
	{
		const DepthStencilState *d=t->second;
		outstr<<"DepthStencilState "<<t->first<<" "
			<<ToString(t->second->DepthEnable)
			<<","<<ToString(t->second->DepthWriteMask)
			<<","<<ToString((int)t->second->DepthFunc);
		outstr<<"\n";
	}
	for (map<string, TechniqueGroup>::const_iterator g = m_techniqueGroups.begin(); g != m_techniqueGroups.end(); ++g)
	{
		const TechniqueGroup &group=g->second;
		outstr<<"group "<<g->first<<"\n{\n";
		for (map<string, Technique*>::const_iterator it =group.m_techniques.begin(); it !=group.m_techniques.end(); ++it)
		{
			std::string techName=it->first;
			const Technique *tech=it->second;
			outstr<<"\ttechnique "<<techName<<"\n\t{\n";
			const std::map<string, Pass> &passes=tech->GetPasses();
			map<string,Pass>::const_iterator j=passes.begin();
			for(;j!=passes.end();j++)
			{
				const Pass *pass=&(j->second);
				string passName=j->first;
				outstr<<"\t\tpass "<<passName<<"\n\t\t{\n";
				if(pass->passState.blendState.objectName.length()>0)
				{
					outstr<<"\t\t\tblend: "<<pass->passState.blendState.objectName<<"\n";
				}
				if(pass->passState.rasterizerState.objectName.length()>0)
				{
					outstr<<"\t\t\trasterizer: "<<pass->passState.rasterizerState.objectName<<"\n";
				}
				if(pass->passState.depthStencilState.objectName.length()>0)
				{
					outstr<<"\t\t\tdepthstencil: "<<pass->passState.depthStencilState.objectName<<" "<<pass->passState.depthStencilState.stencilRef<<"\n";
				}
				if(pass->passState.blendState.objectName.length()>0)
				{
					outstr<<"\t\t\tblend: "<<pass->passState.blendState.objectName<<" (";
					outstr<<pass->passState.blendState.blendFactor[0]<<",";
					outstr<<pass->passState.blendState.blendFactor[1]<<",";
					outstr<<pass->passState.blendState.blendFactor[2]<<",";
					outstr<<pass->passState.blendState.blendFactor[3]<<") ";
					outstr<<pass->passState.blendState.sampleMask<<"\n";
				}
				for(int s=0;s<NUM_OF_SHADER_TYPES;s++)
				{
					if(!pass->HasShader((ShaderType)s))
						continue;
					if(s==ShaderType::EXPORT_SHADER)	// either/or.
						continue;
					ShaderType shaderType=(ShaderType)s;
					if(shaderType==ShaderType::VERTEX_SHADER&&pass->HasShader(GEOMETRY_SHADER))
						shaderType=EXPORT_SHADER;
					const string &compiledShaderName=pass->GetShader((ShaderType)shaderType);
					CompiledShader *compiledShader=m_compiledShaders[compiledShaderName];
					int vertex_or_export=0;
					if(shaderType==EXPORT_SHADER)
						vertex_or_export=1;
					
					for(auto v=compiledShader->sbFilenames.begin();v!=compiledShader->sbFilenames.end();v++)
					{
						string sbFilename=v->second;
						PixelOutputFormat pixelOutputFormat=(PixelOutputFormat)v->first;
						string pfm=ToString(pixelOutputFormat);
						if(shaderType==FRAGMENT_SHADER||(int)pixelOutputFormat==vertex_or_export)
						if(sbFilename.size())
						{
							outstr<<"\t\t\t"<<stringOf((ShaderCommand)shaderType);
							if(shaderType==FRAGMENT_SHADER&&pfm.length())
								outstr<<"("<<pfm<<")";
							outstr<<": "<<sbFilename;

							outstr<<",(";
							for(int w=0;w<compiledShader->textureSlots.size();w++)
							{
								if(w)
									outstr<<",";
								outstr<<ToString(compiledShader->textureSlots[w]);
							}
							outstr<<"),(";
							for(int w=0;w<compiledShader->bufferSlots.size();w++)
							{
								if(w)
									outstr<<",";
								outstr<<ToString(compiledShader->bufferSlots[w]);
							}
							outstr<<"),(";
							for(int w=0;w<compiledShader->samplerSlots.size();w++)
							{
								if(w)
									outstr<<",";
								outstr<<ToString(compiledShader->samplerSlots[w]);
							}
							outstr<<")\n";
						}
					}
				}
				outstr<<"\t\t}\n";
			}
			outstr<<"\t}\n";
		}
		outstr<<"}\n";
	}
//	psfxo.write(outstr.str().c_str(),outstr.str().length());
	std::cout<<psfxoFilename.c_str()<<std::endl;
	return res!=0;
}