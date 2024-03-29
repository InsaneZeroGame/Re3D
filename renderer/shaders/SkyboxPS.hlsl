#include "shader_common.hlsli"

float4 main(SkyBoxPsInput input) : SV_TARGET
{
    return float4(input.texcoord.x, input.texcoord.y, 0.0f, 1.0f);
}