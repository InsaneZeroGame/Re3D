#include "shader_common.hlsli"

struct Meshlet
{
   uint VertCount;
   uint VertOffset;
   uint PrimCount;
   uint PrimOffset;
};

StructuredBuffer<Meshlet> Meshlets : register(t0);
StructuredBuffer<VSInput> Vertices : register(t1);
ByteAddressBuffer UniqueVertexIndices : register(t2);
StructuredBuffer<uint> PrimitiveIndices : register(t3);


struct MeshShaderConstants
{
    float4x4 modelMatrix;
    uint MeshletOffset;
    uint VertexOffsetWithinScene;
    uint PrimitiveOffset;
    uint IndexOffsetWithinScene;
};
ConstantBuffer<MeshShaderConstants> ObjectConstants : register(b0);
ConstantBuffer<FrameData> frameData : register(b1);

struct VertexOut
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

static float4 zliceColor[] =
{
        float4(1.0, 0.0, 0.0, 1.0f),
        float4(0.0, 1.0, 0.0, 1.0f),
        float4(0.0, 0.0, 1.0, 1.0f),
        float4(1.0, 1.0, 0.0, 1.0f),
        float4(1.0, 1.0, 0.0, 1.0f),
        float4(0.0, 1.0, 1.0, 1.0f),
        float4(1.0, 0.0, 1.0, 1.0f),
};


uint3 UnpackPrimitive(uint primitive)
{
    // Unpacks a 10 bits per index triangle from a 32-bit uint.
    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
}

uint3 GetPrimitive(Meshlet m, uint index)
{
    return UnpackPrimitive(PrimitiveIndices[m.PrimOffset + ObjectConstants.PrimitiveOffset + index]);
}

uint GetVertexIndex(Meshlet m, uint localIndex)
{
    localIndex = m.VertOffset + localIndex;
    localIndex = localIndex + ObjectConstants.IndexOffsetWithinScene;
    return UniqueVertexIndices.Load(localIndex * 4);
    //if (MeshInfo.IndexBytes == 4) // 32-bit Vertex Indices
    //{
    //}
    //else // 16-bit Vertex Indices
    //{
    //    // Byte address must be 4-byte aligned.
    //    uint wordOffset = (localIndex & 0x1);
    //    uint byteOffset = (localIndex / 2) * 4;
    //
    //    // Grab the pair of 16-bit indices, shift & mask off proper 16-bits.
    //    uint indexPair = UniqueVertexIndices.Load(byteOffset);
    //    uint index = (indexPair >> (wordOffset * 16)) & 0xffff;
    //
    //    return index;
    //}
}

struct Payload
{
    uint meshletID[128];
};

groupshared Payload payload;

[numthreads(128,1,1)]
void as_main(
    in uint gtid : SV_GroupThreadID,
    in uint groupID : SV_GroupID
)
{
    payload.meshletID[gtid] = ObjectConstants.MeshletOffset + gtid;
    DispatchMesh(128, 1, 1, payload);
}


[numthreads(128, 1, 1)]
[outputtopology("triangle")]
void ms_main(
    in uint gtid : SV_GroupThreadID,
    in uint groupID : SV_GroupID,
    in payload Payload meshInput,
    out vertices VertexOut vertices[64], // Max possible vertices
    out indices uint3 tris[126]) // Max possible triangles
{
    // Calculate the global meshlet ID
    //uint meshletID = ObjectConstants.MeshletOffset + groupID;
    uint meshletID = meshInput.meshletID[groupID];
    Meshlet meshlet = Meshlets[meshletID];
    SetMeshOutputCounts(meshlet.VertCount, meshlet.PrimCount);

    // Each thread processes a single vertex
    if (gtid < meshlet.VertCount)
    {
        uint vertexIndex = GetVertexIndex(meshlet, gtid) + ObjectConstants.VertexOffsetWithinScene;
        float4 modelSpacePos = mul(Vertices[vertexIndex].pos, ObjectConstants.modelMatrix);
        vertices[gtid].position = mul(modelSpacePos, frameData.ViewPrj);
        vertices[gtid].color = float4(Vertices[vertexIndex].normal, 1.0);
    }
    
    if (gtid < meshlet.PrimCount)
    {
        tris[gtid] = GetPrimitive(meshlet, gtid);
    }
}