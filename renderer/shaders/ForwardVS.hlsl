cbuffer FrameData: register(b0)
{
    float4x4 Prj;
    float4x4 View;
    float4x4 ViewPrj;
    
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput main(float4 pos : POSITION,float4 normal : NORMAL,float2 textureCoord:TEXCOORD)
{
    PSInput input;
    float4x4 prjViewMatrix = mul(View, Prj);
    input.position = mul(pos, ViewPrj);
    //input.position = pos;
    //input.color = float4(1.0, 0.0f, 0.0f, 1.0f);
    input.color = normal;
    return input;
}