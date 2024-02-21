
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float3 normal : NORMAL;
    float3 DirectionalLightDir : COLOR1;
    float3 DirectionalLightColor : COLOR2;
};

float4 main(PSInput input) : SV_TARGET
{
    float diffuse = max(dot(normalize(input.normal), input.DirectionalLightDir), 0.0) * 2.5f;
    float ambient = 0.35f;
    return (diffuse + ambient) * input.color;
}