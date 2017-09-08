#ifndef PSFXEFFECT_H
#define PSFXEFFECT_H
#include <set>
namespace psfxParser
{
	struct TextureDetails
	{
		int slot;
		int dimensions;
		bool writeable;
		bool is_array;
		bool is_cubemap;
	};
	struct Function
	{
		void clear()
		{
			functionsCalled.clear();
			returnType.clear();
			name.clear();
			content.clear();
			parameters.clear();		
			globals.clear();		
		}
		std::set<Function*> functionsCalled;
		std::string returnType;
		std::string name;
		std::string content;
		std::vector<psfxstype::variable> parameters;	/// Passed in from the caller.
		std::vector<psfxstype::variable> globals;		/// Referenced but not passed - must be global.
	};
	//! A shader to be compiled. 
	struct CompiledShader
	{
		CompiledShader(const std::set<int> &tSlots
						,const std::set<int> &bSlots
						,const std::set<int> &sSlots);
		CompiledShader(const CompiledShader &cs);
		int Compile(const std::string &sourceFile,std::string targetFile
						,ShaderType t
						,PixelOutputFormat pixelOutputFormat
						,const std::string &sharedSource
						,std::ostringstream& sLog);
		ShaderType shaderType;
		std::string m_profile;
		std::string m_functionName;
		std::string m_preamble;
		std::string m_augmentedSource;
		std::map<int,std::string> sbFilenames;// maps from PixelOutputFormat for pixel shaders, or int for vertex(0) and export(1) shaders.
		std::vector<int> textureSlots;
		std::vector<int> bufferSlots;
		std::vector<int> samplerSlots;
	};
	typedef std::map<std::string,std::vector<Function*> > FunctionMap;
	typedef std::map<std::string,CompiledShader*> CompiledShaderMap;
	typedef std::map<std::string,std::string> StringMap;
	struct TechniqueGroup
	{
		map<std::string, Technique*> m_techniques;
	};
	class Effect
	{
		map<std::string,TextureDetails>		m_textureDeclarations;
		map<std::string, Pass*>				m_programs;
		vector<std::string>					m_programNames;
		map<std::string, TechniqueGroup>	m_techniqueGroups;
		map<std::string,BlendState*>		m_blendStates;
		map<std::string,DepthStencilState*>	m_depthStencilStates;
		map<std::string,RasterizerState*>	m_rasterizerStates;
		map<std::string,SamplerState*>		m_samplerStates;
		FunctionMap							m_declaredFunctions;
		map<std::string,std::string>		m_cslayout;
		map<std::string,std::string>		m_gslayout;
		CompiledShaderMap					m_compiledShaders;
		struct InterfaceDcl
		{
			string id;
			int atLine;

			InterfaceDcl(string s, int l) : id(s), atLine(l) {}
			InterfaceDcl() {}
		};
		map<string, InterfaceDcl>   m_interfaces;
		vector<string>				m_filenames;
		ostringstream               m_log;
		int                         m_includes;
		bool                        m_active;
		string                      m_dir;
		string						m_filename;
    
	public:
		ostringstream               m_sharedCode;
		ostringstream& Log();
		TechniqueGroup &GetTechniqueGroup(const std::string &name);
		const vector<string>& GetProgramList() const;
		const vector<string>& GetFilenameList() const;
		void SetFilenameList(const char **);
		void PopulateProgramList();
		bool GenerateShaderBinaries(string sfxFilename,string psfxoFilename);
		unsigned CompileAllShaders(string psfxoFilename,const string &sharedCode,string& log) ;
		bool& Active();
		string& Dir();
		string& Filename();
		~Effect();
		Effect();

		Function * DeclareFunction(const std::string &functionName,Function &buildFunction);
		Function * GetFunction(const std::string &functionName,int i);
		int GetSlot(const std::string &variableName) const;
		friend int ::glfxparse();
		friend int ::glfxlex();
	};
	extern Effect *gEffect;
}
#endif