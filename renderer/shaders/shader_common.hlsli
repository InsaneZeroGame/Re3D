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
};


struct Light
{
    float4 color;
    float4 pos;
    float radius;
    float attenu;
    float2 padding;
};

struct Cluster
{
    //32bit * 8
    uint lightMask[8];
};

struct LightCullViewData
{
    float4 ViewSizeAndInvSize;
    float4x4 ClipToView;
    float4x4 ViewMatrix;
    float4 LightGridZParams;
    float4 InvDeviceZToWorldZTransform;
    
};