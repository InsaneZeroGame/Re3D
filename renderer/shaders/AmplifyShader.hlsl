struct MeshShaderConstants
{
    float4x4 modelMatrix;
    uint MeshletOffset;
    uint VertexOffsetWithinScene;
    uint PrimitiveOffset;
    uint IndexOffsetWithinScene;
};

struct Payload
{
    uint meshletID[128];
};

groupshared Payload payload;

ConstantBuffer<MeshShaderConstants> ObjectConstants : register(b0);


[numthreads(128, 1, 1)]
void as_main(
    in uint gtid : SV_GroupThreadID,
    in uint groupID : SV_GroupID
)
{
    payload.meshletID[gtid] = ObjectConstants.MeshletOffset + gtid;
    DispatchMesh(128, 1, 1, payload);
}