struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float3 normal : NORMAL0;
    float4 normalViewSpace : NORMAL1;
    float3 DirectionalLightDir : COLOR1;
    float3 DirectionalLightColor : COLOR2;
    float4 viewsSpacePos : COLOR3;
    float2 UVCoord : TEXCOORD0;
    float4 shadowCoord : POSITION0;
};


struct Light
{
    float4 color;
    float4 pos;
    float4 radius_attenu;
};

struct Cluster
{
    //32bit * 8
    uint lightMask[8];
};


struct FrameData 
{
    float4x4 ViewPrj;
    float4x4 View;
    float4x4 Prj;
    float4x4 NormalMatrix;
    float4x4 ShadowViewMatrix;
    float4x4 ShadowViewPrjMatrix;
    float4 DirectionalLightDir;
    float4 DirectionalLightColor;
    float4 ViewSizeAndInvSize;
    float4x4 ClipToView;
    float4x4 ViewMatrix;
    float4 LightGridZParams;
    float4 InvDeviceZToWorldZTransform;
    
};

struct ObjectData
{
    float4x4 ModelMatrix;
};

struct SkyBoxPsInput
{
    float4 pos : SV_Position;
    float3 texcoord : TEXCOORD0;
};