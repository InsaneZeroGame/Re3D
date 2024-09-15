struct MeshVertex
{
    float4 position : SV_Position;
    float4 color : COLOR0;
    
};

float4 main(MeshVertex msOutput) : SV_Target
{
    return msOutput.color;
}