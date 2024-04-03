#include "shader_common.hlsli"

TextureCube<float4> skyboxTexture : register(t1);
SamplerState cubeSampler : register(s0);

float4 main(SkyBoxPsInput input) : SV_TARGET
{
    //return float4(input.texcoord.x, input.texcoord.y, 0.0f, 1.0f);
    return skyboxTexture.Sample(cubeSampler, input.texcoord);
}