
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput main(float4 pos : POSITION,float4 normal : NORMAL,float2 textureCoord:TEXCOORD)
{
    PSInput input;
    input.position = pos;
    input.color = normal;
    return input;
}