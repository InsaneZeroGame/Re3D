#include "shader_common.hlsli"

struct Vertex
{
    float3 position : POSITION;
};

StructuredBuffer<Vertex> vertexBuffer : register(t0);

struct MeshVertex
{
    float4 position : SV_Position;
};

struct MeshPrimitive
{
    uint3 indices : SV_PrimitiveID;
};

[numthreads(1, 1, 1)]
[outputtopology("triangle")]
void main(
    in uint3 dispatchThreadID : SV_DispatchThreadID,
    out vertices MeshVertex vertices[3],
    out indices uint3 primitives[1])
{
    SetMeshOutputCounts(3, 1); // 3 vertices, 1 primitive

    // Read vertices from the buffer
    vertices[0].position = float4(vertexBuffer[0].position,1.0f);
    vertices[1].position = float4(vertexBuffer[1].position,1.0f);
    vertices[2].position = float4(vertexBuffer[2].position, 1.0f);

    // Define primitive (triangle)
    primitives[0] = uint3(0, 1, 2);
}
