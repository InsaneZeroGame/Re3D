#include "shader_common.hlsli"

ConstantBuffer<FrameData> frameData : register(b0);
StructuredBuffer<Light> lights : register(t1);
RWStructuredBuffer<Cluster> clusters : register(u2);
Texture2D<float4> defaultTexture : register(t5);
Texture2D<float4> normalTexture : register(t6);
Texture2D<float4> roughnessTexture : register(t7);
Texture2D<float> shadowMap : register(t8);

SamplerState defaultSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

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

// Apply fresnel to modulate the specular albedo
void FSchlick(inout float3 specular, inout float3 diffuse, float3 lightDir, float3 halfVec)
{
    float fresnel = pow(1.0 - saturate(dot(lightDir, halfVec)), 5.0);
    specular = lerp(specular, 1, fresnel);
    diffuse = lerp(diffuse, 0, fresnel);
}

float3 ApplyLightCommon(
    float3 diffuseColor, // Diffuse albedo
    float3 specularColor, // Specular albedo
    float specularMask, // Where is it shiny or dingy?
    float gloss, // Specular power
    float3 normal, // World-space normal
    float3 viewDir, // World-space vector from eye to point
    float3 lightDir, // World-space vector from point to light
    float3 lightColor // Radiance of directional light
    )
{
    float3 halfVec = normalize(lightDir - viewDir);
    float nDotH = saturate(dot(halfVec, normal));

    FSchlick(specularColor,diffuseColor , lightDir, halfVec);

    float specularFactor = nDotH;
    //float specularFactor = specularMask * pow(nDotH, gloss) * (gloss + 2) / 8;
    

    float nDotL = saturate(dot(normal, lightDir));
    
    //return nDotL * lightColor * (diffuseColor + specularColor);

    return nDotL * lightColor * (diffuseColor + specularFactor * specularColor);
}

float3 ApplyPointLight(
    float3 diffuseColor, // Diffuse albedo
    float3 specularColor, // Specular albedo
    float specularMask, // Where is it shiny or dingy?
    float gloss, // Specular power
    float3 normal, // World-space normal
    float3 viewDir, // World-space vector from eye to point
    float3 worldPos, // World-space fragment position
    float3 lightPos, // World-space light position
    float lightRadiusSq,
    float3 lightColor // Radiance of directional light
    )
{
    float3 lightDir = lightPos - worldPos;
    float lightDistSq = dot(lightDir, lightDir);
    float invLightDist = rsqrt(lightDistSq);
    lightDir *= invLightDist;

    // modify 1/d^2 * R^2 to fall off at a fixed radius
    // (R/d)^2 - d/R = [(1/d^2) - (1/R^2)*(d/R)] * R^2
    float distanceFalloff = lightRadiusSq * (invLightDist * invLightDist);
    distanceFalloff = max(0, distanceFalloff - rsqrt(distanceFalloff));

    return distanceFalloff * ApplyLightCommon(
        diffuseColor,
        specularColor,
        specularMask,
        gloss,
        normal,
        viewDir,
        lightDir,
        lightColor
        );
}


uint3 ComputeLightGridCellCoordinate(uint2 PixelPos, float SceneDepth, uint EyeIndex)
{
    uint ZSlice = (uint) (max(0, log2(SceneDepth * frameData.LightGridZParams.x + frameData.LightGridZParams.y) * frameData.LightGridZParams.z));
    ZSlice = min(ZSlice, (uint) (CLUSTER_Z - 1));
    return uint3(PixelPos.x / (frameData.ViewSizeAndInvSize.x / CLUSTER_X), PixelPos.y / (frameData.ViewSizeAndInvSize.y / CLUSTER_Y), ZSlice);
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
    return DeviceZ * frameData.InvDeviceZToWorldZTransform.x + frameData.InvDeviceZToWorldZTransform.y + 1.0f / (DeviceZ * frameData.InvDeviceZToWorldZTransform.z - frameData.InvDeviceZToWorldZTransform.w);
}


float4 main(PSInput input) : SV_TARGET
{
    
#if 0
    float diffuse = max(dot(normalize(input.normal), input.DirectionalLightDir), 0.0);
     float ambient = 0.0f;
    float gamma = 2.2f;
     float4 colorAfterCorrection = pow(input.color, 1.0 / gamma);
    return (diffuse + ambient) * colorAfterCorrection;
#else
    float4 DirLightViewSpace = mul(float4(input.DirectionalLightDir.xyz, 0.0), frameData.ViewMatrix);
    float DirLightIntense = 0.2f;
    float3 directionalLight = ApplyLightCommon(
    input.color.xyz,
    input.color.xyz,
    0, 0, input.normalViewSpace.xyz, input.viewsSpacePos, 
    DirLightViewSpace, input.DirectionalLightColor) * DirLightIntense;
    
    float3 pointLight = float3(0.0,0.0,0.0);
    uint GirdIndex = ComputeLightGridCellIndex(uint2(input.position.xy), input.position.w);
    uint3 GridCoord = ComputeLightGridCellCoordinate(uint2(input.position.xy), input.position.w, 0);
    for (int i = 0; i < 256; ++i)
    {
        if ((clusters[GirdIndex].lightMask[i / 32] >> (i % 32)) & 0x1  == 1)
        {
            float4 lightViewSpace = mul(lights[i].pos, frameData.ViewMatrix);
            float4 lightDir = lightViewSpace - input.viewsSpacePos;
            float d = length(lightDir);
            if (d < lights[i].radius_attenu[0])
            {
                pointLight += ApplyPointLight(
                input.color.xyz,
                input.color.xyz,
                0, 0, input.normalViewSpace.xyz,
                input.viewsSpacePos.xyz,
                input.viewsSpacePos.xyz,
                lightViewSpace.xyz,
                lights[i].radius_attenu[0],
                lights[i].color.xyz);
                //pointLight += lights[i].color.xyz;
            }
           
        }
    }
    pointLight += directionalLight;
    float gamma = 2.2f;
    float4 ambient = float4(0.15, 0.15, 0.15, 1.0f);
    float4 color = float4(pointLight, 1.0f);
    float4 diffuseColor = defaultTexture.Sample(defaultSampler, input.UVCoord);
    color *= diffuseColor;
    float4 colorAfterCorrection = pow(color, 1.0 / gamma);
    float2 shadowCoord = input.shadowCoord.xy /input.shadowCoord.w;
    //shadwo map
    shadowCoord = shadowCoord * float2(0.5, -0.5) + 0.5;
    float shadow = shadowMap.SampleCmpLevelZero(shadowSampler, shadowCoord, input.shadowCoord.z/input.shadowCoord.w);
    //shadow = 1.0f;
    return colorAfterCorrection * (0.15 + 0.85 * shadow);
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
    //return zliceColor[GridCoord.x % 7];
    
#endif
} 