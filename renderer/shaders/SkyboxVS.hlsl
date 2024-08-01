#include "shader_common.hlsli"

ConstantBuffer<FrameData> frameData : register(b0);

SkyBoxPsInput main(VSInput vsInput)
{
    SkyBoxPsInput output;
    float4x4 viewMatrixWithOutTranslation = frameData.View;
    viewMatrixWithOutTranslation._14_24_34_44 = float4(0.0, 0.0, 0.0, 1.0);
    viewMatrixWithOutTranslation._41_42_43_44 = float4(0.0, 0.0, 0.0, 1.0);
    output.pos = mul(mul(vsInput.pos, viewMatrixWithOutTranslation), frameData.Prj);
    output.texcoord = vsInput.pos.xyz;
    return output;
}