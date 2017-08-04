struct PerInstanceData
{
    matrix instanceMat;
};

StructuredBuffer<PerInstanceData> g_pInstanceData : register(t0);

cbuffer cbChangesEveryFrame : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Nor : NORMAL;
    float2 Tex : TEXCOORD;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    PS_INPUT output = (PS_INPUT) 0;
    //matrix instanceWVP = mul(Projection, View);
    //instanceWVP = mul(instanceWVP, World);
    //instanceWVP = mul(instanceWVP, g_pInstanceData[instanceID].instanceMat);

    //instanceWVP = transpose(instanceWVP);

    output.Pos = mul(input.Pos, g_pInstanceData[instanceID].instanceMat);
    output.Pos = mul(output.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Tex;

    return output;
}
