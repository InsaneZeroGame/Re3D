
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput main()
{
    PSInput input;
    input.position = float4(1.0, 1.0, 1.0, 1.0f);
    input.color = float4(1.0, 1.0, 1.0, 1.0f);
    return input;
}