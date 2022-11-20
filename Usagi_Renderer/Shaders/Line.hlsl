cbuffer cbPass : register(b0)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 gViewProjTex;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
};

struct VertexIn
{
	float3 PosL    : POSITION;
    uint vertexID : SV_VertexID;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
    float4 PosW     : WORLD_POSITION;
    uint vertexID : COLOR;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;

    // Transform to homogeneous clip space.
    vout.PosH = mul(float4(vin.PosL, 1.0f), gViewProj);
    vout.vertexID = vin.vertexID;

    return vout;
}

PS_OUTPUT PS(VertexOut pin)
{
    float3 albedo;
    if(pin.vertexID % 3 == 0)
    {
        albedo = float3(1, 0, 0);
    }
    else if(pin.vertexID % 3 == 1)
    {
        albedo = float3(0, 1, 0);
    }
    else
    {
        albedo = float3(0, 0, 1);
    }
    PS_OUTPUT output;
    output.Color = float4(albedo, 1);
    return output;
}
