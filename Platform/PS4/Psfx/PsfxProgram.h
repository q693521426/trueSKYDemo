#ifndef PSFXPROGRAM_H
#define PSFXPROGRAM_H

namespace psfxParser
{
	class Pass
	{
	public:
		PassState passState;
		Pass();
		Pass(const map<ShaderType, string>& shaders,const PassState &passState);
        bool HasShader(ShaderType t) const
		{
			return m_compiledShaderNames[(int)t].length()>0;
		}
        std::string GetShader(ShaderType t) const
		{
			return m_compiledShaderNames[(int)t];
		}

	private:
		string m_compiledShaderNames[NUM_OF_SHADER_COMMANDS];
		friend int ::glfxparse();
	};
	class Technique
	{
	public:
		Technique(const map<std::string, Pass>& passes);
		const map<string, Pass>   &GetPasses()  const
		{
			return m_passes;
		}
	private:
		map<string, Pass>   m_passes;
		friend int ::glfxparse();
	};
}
#endif