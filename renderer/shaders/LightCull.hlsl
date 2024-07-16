#include "shader_common.hlsli"

ConstantBuffer<LightCullViewData> View : register(b0);
StructuredBuffer<Light> lights : register(t1);
RWStructuredBuffer<Cluster> clusters: register(u2);

static const int CLUSTER_X = 32;
static const int CLUSTER_Y = 16;
static const int CLUSTER_Z = 16;

//Use UE's impl
//Todo: Compare with idTechs' impl
float ComputeCellNearViewDepthFromZSlice(uint ZSlice)
{
    float SliceDepth = (exp2(ZSlice / View.LightGridZParams.z) - View.LightGridZParams.y) / View.LightGridZParams.x;

    if (ZSlice == (uint) CLUSTER_Z)
    {
		// Extend the last slice depth max out to world max
		// This allows clamping the depth range to reasonable values, 
		// But has the downside that any lights falling into the last depth slice will have very poor culling,
		// Since the view space AABB will be bloated in x and y
        SliceDepth = 2000000.0f;
    }

    if (ZSlice == 0)
    {
		// The exponential distribution of z slices contains an offset, but some screen pixels
		// may be nearer to the camera than this offset. To avoid false light rejection, we set the
		// first depth slice to zero to ensure that the AABB includes the [0, offset] depth range.
        SliceDepth = 0.0f;
    }

    return SliceDepth;
}

float ConvertToDeviceZ(float SceneDepth)
{
    //Todo: Support Ortho
    if (false)
    {
		// Ortho
        //return SceneDepth * View.ViewToClip[2][2] + View.ViewToClip[3][2];
    }
    else
    {
		// Perspective
        return 1.0f / ((SceneDepth + View.InvDeviceZToWorldZTransform.w) * View.InvDeviceZToWorldZTransform.z);
    }
}

/** Computes squared distance from a point in space to an AABB. */
float ComputeSquaredDistanceFromBoxToPoint(float3 BoxCenter, float3 BoxExtent, float3 InPoint)
{
    float3 AxisDistances = max(abs(InPoint - BoxCenter) - BoxExtent, 0);
    return dot(AxisDistances, AxisDistances);
}



void ComputeCellViewAABB(uint3 GridCoordinate, out float3 ViewTileMin, out float3 ViewTileMax)
{
	// Compute extent of tiles in clip-space. Note that the last tile may extend a bit outside of view if view size is not evenly divisible tile size.
    //const float2 InvCulledGridSizeF = (1u << LightGridPixelSizeShift) * View.ViewSizeAndInvSize.zw;
    const float2 GridPixelSize = float2(View.ViewSizeAndInvSize.x/CLUSTER_X,View.ViewSizeAndInvSize.y / CLUSTER_Y);
    const float2 InvCulledGridSizeF = GridPixelSize * View.ViewSizeAndInvSize.zw;
    const float2 TileSize = float2(2.0f, -2.0f) * InvCulledGridSizeF.xy;
    const float2 UnitPlaneMin = float2(-1.0f, 1.0f);

    float2 UnitPlaneTileMin = GridCoordinate.xy * TileSize + UnitPlaneMin;
    float2 UnitPlaneTileMax = (GridCoordinate.xy + 1) * TileSize + UnitPlaneMin;

    float MinTileZ = ComputeCellNearViewDepthFromZSlice(GridCoordinate.z);
    float MaxTileZ = ComputeCellNearViewDepthFromZSlice(GridCoordinate.z + 1);

    float MinTileDeviceZ = ConvertToDeviceZ(MinTileZ);
    float4 MinDepthCorner0 = mul(float4(UnitPlaneTileMin.x, UnitPlaneTileMin.y, MinTileDeviceZ, 1), View.ClipToView);
    float4 MinDepthCorner1 = mul(float4(UnitPlaneTileMax.x, UnitPlaneTileMax.y, MinTileDeviceZ, 1), View.ClipToView);
    float4 MinDepthCorner2 = mul(float4(UnitPlaneTileMin.x, UnitPlaneTileMax.y, MinTileDeviceZ, 1), View.ClipToView);
    float4 MinDepthCorner3 = mul(float4(UnitPlaneTileMax.x, UnitPlaneTileMin.y, MinTileDeviceZ, 1), View.ClipToView);

    float MaxTileDeviceZ = ConvertToDeviceZ(MaxTileZ);
    float4 MaxDepthCorner0 = mul(float4(UnitPlaneTileMin.x, UnitPlaneTileMin.y, MaxTileDeviceZ, 1), View.ClipToView);
    float4 MaxDepthCorner1 = mul(float4(UnitPlaneTileMax.x, UnitPlaneTileMax.y, MaxTileDeviceZ, 1), View.ClipToView);
    float4 MaxDepthCorner2 = mul(float4(UnitPlaneTileMin.x, UnitPlaneTileMax.y, MaxTileDeviceZ, 1), View.ClipToView);
    float4 MaxDepthCorner3 = mul(float4(UnitPlaneTileMax.x, UnitPlaneTileMin.y, MaxTileDeviceZ, 1), View.ClipToView);

    float2 ViewMinDepthCorner0 = MinDepthCorner0.xy / MinDepthCorner0.w;
    float2 ViewMinDepthCorner1 = MinDepthCorner1.xy / MinDepthCorner1.w;
    float2 ViewMinDepthCorner2 = MinDepthCorner2.xy / MinDepthCorner2.w;
    float2 ViewMinDepthCorner3 = MinDepthCorner3.xy / MinDepthCorner3.w;
    float2 ViewMaxDepthCorner0 = MaxDepthCorner0.xy / MaxDepthCorner0.w;
    float2 ViewMaxDepthCorner1 = MaxDepthCorner1.xy / MaxDepthCorner1.w;
    float2 ViewMaxDepthCorner2 = MaxDepthCorner2.xy / MaxDepthCorner2.w;
    float2 ViewMaxDepthCorner3 = MaxDepthCorner3.xy / MaxDepthCorner3.w;

	//@todo - derive min and max from quadrant
    ViewTileMin.xy = min(ViewMinDepthCorner0, ViewMinDepthCorner1);
    ViewTileMin.xy = min(ViewTileMin.xy, ViewMinDepthCorner2);
    ViewTileMin.xy = min(ViewTileMin.xy, ViewMinDepthCorner3);
    ViewTileMin.xy = min(ViewTileMin.xy, ViewMaxDepthCorner0);
    ViewTileMin.xy = min(ViewTileMin.xy, ViewMaxDepthCorner1);
    ViewTileMin.xy = min(ViewTileMin.xy, ViewMaxDepthCorner2);
    ViewTileMin.xy = min(ViewTileMin.xy, ViewMaxDepthCorner3);

    ViewTileMax.xy = max(ViewMinDepthCorner0, ViewMinDepthCorner1);
    ViewTileMax.xy = max(ViewTileMax.xy, ViewMinDepthCorner2);
    ViewTileMax.xy = max(ViewTileMax.xy, ViewMinDepthCorner3);
    ViewTileMax.xy = max(ViewTileMax.xy, ViewMaxDepthCorner0);
    ViewTileMax.xy = max(ViewTileMax.xy, ViewMaxDepthCorner1);
    ViewTileMax.xy = max(ViewTileMax.xy, ViewMaxDepthCorner2);
    ViewTileMax.xy = max(ViewTileMax.xy, ViewMaxDepthCorner3);

    ViewTileMin.z = MinTileZ;
    ViewTileMax.z = MaxTileZ;
}
//light max count 256
//32 * 8
[numthreads(32, 8, 1)]
void main(
    uint3 GroupId : SV_GroupID,
	uint3 DispatchThreadId : SV_DispatchThreadID,
	uint3 GroupThreadId : SV_GroupThreadID,
    uint threadIndex:SV_GroupIndex)
{
    uint clusterID = GroupId.x + (GroupId.y) * CLUSTER_X + GroupId.z * (CLUSTER_X * CLUSTER_Y);
    if (GroupThreadId.x == 0 && GroupThreadId.y == 0)
    {
        for (int i = 0; i < 8;++i)
        {
            clusters[clusterID].lightMask[i] = 0;
        }
    }
    GroupMemoryBarrier();
    float3 ViewTileMin;
    float3 ViewTileMax;
    ComputeCellViewAABB(GroupId, ViewTileMin, ViewTileMax);
    
    float3 ViewTileCenter = .5f * (ViewTileMin + ViewTileMax);
    float3 ViewTileExtent = ViewTileMax - ViewTileCenter;
   
    uint LocalLightIndex = GroupThreadId.x + GroupThreadId.y * 32;
    float3 ViewSpaceLightPosition = mul(lights[LocalLightIndex].pos,View.ViewMatrix).xyz;
    float LightRadius = lights[LocalLightIndex].radius_attenu[0];
    float BoxDistanceSq = ComputeSquaredDistanceFromBoxToPoint(ViewTileCenter, ViewTileExtent, ViewSpaceLightPosition);
    if (BoxDistanceSq < LightRadius * LightRadius)
    {
        InterlockedOr(clusters[clusterID].lightMask[GroupThreadId.y], (0x1 << GroupThreadId.x));
    }
   
}