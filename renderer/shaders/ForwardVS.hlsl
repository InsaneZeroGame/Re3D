#include "shader_common.hlsli"

ConstantBuffer<FrameData> frameData : register(b0);
ConstantBuffer<ObjectData> objData : register(b4);

PSInput main(VSInput vsInput)
{
    PSInput psInput;
    float4 modelSpacePos = mul(vsInput.pos, objData.ModelMatrix);
    psInput.position = mul(modelSpacePos, frameData.ViewPrj);
    psInput.viewsSpacePos = mul(modelSpacePos, frameData.View);
    psInput.normalViewSpace = mul(float4(vsInput.normal.xyz, 0.0), frameData.NormalMatrix);
    psInput.UVCoord = vsInput.textureCoord;
    psInput.color = float4(objData.DiffuseColor, 1.0f);
    psInput.normal = vsInput.normal.xyz;
    psInput.DirectionalLightDir = frameData.DirectionalLightDir.xyz;
    psInput.DirectionalLightColor = frameData.DirectionalLightColor.xyz;
    psInput.shadowCoord = mul(modelSpacePos, frameData.ShadowViewPrjMatrix);
    psInput.tangent = vsInput.tangent;
    psInput.bitangent = vsInput.bitangent;
    return psInput;
}