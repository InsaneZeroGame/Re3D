cbuffer FrameData: register(b0)
{
    float4x4 ViewPrj;
    float4 DirectionalLightDir;
    float4 DirectionalLightColor;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float3 normal : NORMAL;
    float3 DirectionalLightDir : COLOR1;
    float3 DirectionalLightColor : COLOR2;
};

PSInput main(float4 pos : POSITION,float4 normal : NORMAL,float4 color : COLOR,float2 textureCoord:TEXCOORD)
{
    PSInput input;
    input.position = mul(pos,ViewPrj);
    //input.position = pos;
    //input.color = float4(1.0, 0.0f, 0.0f, 1.0f);
    input.color = color;
    input.normal = normal.xyz;
    input.DirectionalLightDir = DirectionalLightDir.xyz;
    input.DirectionalLightColor = DirectionalLightColor.xyz;
    return input;
}