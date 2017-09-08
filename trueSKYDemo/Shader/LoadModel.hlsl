Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

cbuffer cbChangesEveryFrame : register(b0)
{
    matrix World;
    matrix WVP;
};

struct Material
{
    float4 Ambient;
    float4 Diffuse;
    float3 Specular;
    float Power;
    float4 Emisive;
};

cbuffer cbMaterial : register(b1)
{
    Material mat;
};

struct LightPos
{
    float4 LightPos;
    float4 LightColor;

    float4 Ambient;
    float4 Diffuse;
    float4 Specular;

    float Constant;
    float Linear;
    float Quadratic;

    float Lpad;
};

cbuffer LightBuffer : register(b2)
{
    LightPos light;
};

cbuffer FrustumBuffer : register(b3)
{
    float4 ViewPos;
};

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Nor : NORMAL;
    float2 Tex : TEXCOORD;

};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Nor : NORMAL;
    float2 Tex : TEXCOORD0;
    float4 WorldPos : POSITION;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
	float4 pos = float4(input.Pos,1.f);
    output.WorldPos = mul(pos, World);
    output.Pos = mul(pos, WVP);
    output.Tex = input.Tex;
    output.Nor = normalize(mul(input.Nor, (float3x3) World));
    return output;
}

float4 CalculateLight(LightPos light, float4 worldPos, float3 normal);

float4 PS(PS_INPUT input) : SV_Target
{
    float4 color = CalculateLight(light, input.WorldPos, input.Nor);
    return saturate(color);
}

float4 CalculateLight(LightPos light, float4 worldPos, float3 normal)
{
    float3 lightDir = light.LightPos.xyz - float3(0.0f,0.0f,0.0f);
    float distance = length(lightDir);
    float attenuation = 1.0f / (light.Constant + light.Linear * distance + light.Quadratic * (distance * distance));
    
    lightDir = normalize(lightDir);
    float3 viewDir = normalize(ViewPos.xyz - worldPos.xyz);
    float3 halfDir = normalize(lightDir.xyz + viewDir.xyz);
    float spec = pow(max(dot(halfDir.xyz, normal), 0.0), mat.Power);

    float diff = max(dot(normal, lightDir), 0.0);

    float3 color = saturate((light.Ambient.xyz * mat.Ambient.xyz + spec * light.Specular.xyz * mat.Specular + diff * light.Diffuse.xyz * mat.Diffuse.xyz)
                         * light.LightColor.xyz * attenuation);
    
    return float4(color, 1.0f);
}