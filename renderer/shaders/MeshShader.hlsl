#include "shader_common.hlsli"

struct Meshlet
{
   uint VertCount;
   uint VertOffset;
   uint PrimCount;
   uint PrimOffset;
};

struct Vertex
{
    float3 position;
    //float3 normal;
    //float2 texCoord;
};

StructuredBuffer<Meshlet> Meshlets : register(t0);
StructuredBuffer<Vertex> Vertices : register(t1);
ByteAddressBuffer UniqueVertexIndices : register(t2);
StructuredBuffer<uint> PrimitiveIndices : register(t3);


struct MeshShaderConstants
{
    float4x4 modelMatrix;
    uint meshletOffset;
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
    return UnpackPrimitive(PrimitiveIndices[m.PrimOffset + index]);
}

uint GetVertexIndex(Meshlet m, uint localIndex)
{
    localIndex = m.VertOffset + localIndex;
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

[numthreads(128, 1, 1)]
[outputtopology("triangle")]
void main(
    in uint gtid : SV_GroupThreadID,
    in uint groupID : SV_GroupID,
    out vertices VertexOut vertices[64], // Max possible vertices
    out indices uint3 tris[126]) // Max possible triangles
{
    // Calculate the global meshlet ID
    uint meshletID = ObjectConstants.meshletOffset + groupID;

    Meshlet meshlet = Meshlets[meshletID];

    // Get the actual number of vertices and primitives to emit
    uint numVertices = meshlet.VertCount;
    uint numPrimitives = meshlet.PrimCount;

    SetMeshOutputCounts(numVertices, numPrimitives);

    // Each thread processes a single vertex
    if (gtid < numVertices)
    {
        uint vertexIndex = GetVertexIndex(meshlet, gtid);
        float4 modelSpacePos = mul(float4(Vertices[vertexIndex].position, 1.0f), ObjectConstants.modelMatrix);
        vertices[gtid].position = mul(modelSpacePos, frameData.ViewPrj);
        vertices[gtid].color = zliceColor[gtid % 7];
    }
    
    if (gtid < numPrimitives)
    {
        tris[gtid] = GetPrimitive(meshlet, gtid);
    }
}