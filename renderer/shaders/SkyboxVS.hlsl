#include "shader_common.hlsli"

ConstantBuffer<FrameData> frameData;

SkyBoxPsInput main(float4 pos : POSITION, float4 normal : NORMAL, float2 textureCoord : TEXCOORD)
{
    SkyBoxPsInput output;
    float4x4 viewMatrixWithOutTranslation = frameData.View;
    viewMatrixWithOutTranslation._14_24_34_44 = float4(0.0, 0.0, 0.0, 1.0);
    viewMatrixWithOutTranslation._41_42_43_44 = float4(0.0, 0.0, 0.0, 1.0);
    output.pos = mul(mul(pos, viewMatrixWithOutTranslation), frameData.Prj);
    output.texcoord = textureCoord;
    return output;
}