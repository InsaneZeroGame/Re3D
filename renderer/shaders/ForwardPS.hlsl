
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
    float diffuse = max(dot(normalize(input.normal), input.DirectionalLightDir), 0.0);
    float ambient = 0.15f;
    float gamma = 2.2f;
    float4 colorAfterCorrection = pow(input.color, 1.0 / gamma);
    return (diffuse + ambient) * colorAfterCorrection;
}