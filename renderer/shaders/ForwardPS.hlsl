#include "shader_common.hlsli"

StructuredBuffer<Light> lights : register(t1);
RWStructuredBuffer<Cluster> clusters : register(u2);
ConstantBuffer<LightCullViewData> View : register(b3);
static const int CLUSTER_X = 32;
static const int CLUSTER_Y = 16;
static const int CLUSTER_Z = 16;

static float4 zliceColor[] = 
    {
        float4(1.0,0.0,0.0,1.0f),
        float4(0.0,1.0,0.0,1.0f),
        float4(0.0,0.0,1.0,1.0f),
        float4(1.0, 1.0, 0.0, 1.0f),
        float4(1.0, 1.0, 0.0, 1.0f),
        float4(0.0, 1.0, 1.0, 1.0f),
        float4(1.0, 0.0, 1.0, 1.0f),
};


uint3 ComputeLightGridCellCoordinate(uint2 PixelPos, float SceneDepth, uint EyeIndex)
{
    uint ZSlice = (uint) (max(0, log2(SceneDepth * View.LightGridZParams.x + View.LightGridZParams.y) * View.LightGridZParams.z));
    ZSlice = min(ZSlice, (uint) (CLUSTER_Z - 1));
    return uint3(PixelPos.x / (View.ViewSizeAndInvSize.x / CLUSTER_X), PixelPos.y / (View.ViewSizeAndInvSize.y / CLUSTER_Y), ZSlice);
}

uint ComputeLightGridCellIndex(uint3 GridCoordinate, uint EyeIndex)
{
    return (GridCoordinate.z * CLUSTER_Y + GridCoordinate.y) * CLUSTER_X + GridCoordinate.x;
}

uint ComputeLightGridCellIndex(uint2 PixelPos, float SceneDepth, uint EyeIndex)
{
    return ComputeLightGridCellIndex(ComputeLightGridCellCoordinate(PixelPos, SceneDepth, EyeIndex), EyeIndex);
}

uint ComputeLightGridCellIndex(uint2 PixelPos, float SceneDepth)
{
    return ComputeLightGridCellIndex(PixelPos, SceneDepth, 0);
}

float ConvertFromDeviceZ(float DeviceZ)
{
	// Supports ortho and perspective, see CreateInvDeviceZToWorldZTransform()
    return DeviceZ * View.InvDeviceZToWorldZTransform.x + View.InvDeviceZToWorldZTransform.y + 1.0f / (DeviceZ * View.InvDeviceZToWorldZTransform.z - View.InvDeviceZToWorldZTransform.w);
}

static int screen_size = 1280;

float4 main(PSInput input) : SV_TARGET
{
    
#if 0
    float diffuse = max(dot(normalize(input.normal), input.DirectionalLightDir), 0.0);
     float ambient = 0.0f;
    float gamma = 2.2f;
     float4 colorAfterCorrection = pow(input.color, 1.0 / gamma);
    return (diffuse + ambient) * colorAfterCorrection;
#else
    float4 diffuse = 0.25;
    uint GirdIndex = ComputeLightGridCellIndex(uint2(input.position.xy), input.position.w);
    uint3 GridCoord = ComputeLightGridCellCoordinate(uint2(input.position.xy), input.position.w, 0);
    for (int i = 0; i < 128; ++i)
    {
        if (clusters[GirdIndex].lightMask[i] == 1)
        {
            //diffuse += lights[i].color;
            float4 lightViewSpace = mul(float4(lights[i].pos.xyz, 1.0f), View.ViewMatrix);
            float3 lightDir = (lightViewSpace - input.viewsSpacePos).xyz;
            float d = length(lightDir);
            if (d < lights[i].radius)
            {
                //Todo:attenutation
                //float atten = 1.0f / (lights[i].radius_attenu.y + d * lights[i].radius_attenu.z, d * d * lights[i].radius_attenu.w);
                float3 n = input.normalViewSpace.xyz;
                float nDotl = max(dot(normalize(n), normalize(lightDir)), 0.0f);
                diffuse += lights[i].color;
            }
            
        }
    }
    return diffuse;
    //if (input.position.x / screen_size  < 0.5)
    //{
    // return diffuse;
    //    
    //}
    //else
    //{
    //return zliceColor[GridCoord.z % 7];
    //}
    //
    //return zliceColor[GridCoord.z % 7];
    
#endif
}