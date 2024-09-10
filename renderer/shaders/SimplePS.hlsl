struct MeshVertex
{
    float4 position : SV_Position;
};

float4 main(MeshVertex msOutput) : SV_Target
{
    return float4(1, 0, 0, 1);
}