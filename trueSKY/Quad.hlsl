Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

cbuffer cbChangesEveryFrame : register(b0)
{
    matrix World;
    matrix WVP;
};

struct VS_INPUT
{
    float4 Pos : POSITION;
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

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    input.Pos.w = 1;
    output.WorldPos = mul(input.Pos, World);
    output.Pos = mul(input.Pos, WVP);
    output.Tex = input.Tex;
    output.Nor = mul(input.Nor, (float3x3) World);
    return output;
}


float4 PS(PS_INPUT input) : SV_Target
{
    return txDiffuse.Sample(samLinear, input.Tex);
}