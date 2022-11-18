struct WorldMatrix
{
    matrix mat;
};

ConstantBuffer<WorldMatrix> objectWorld : register(b0);

cbuffer cbPass : register(b1)
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

cbuffer directionLight : register(b2)
{
	float3 Direction;
    float padding1;

    float3 Color;
    float padding2;
};

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : UV;
	float3 TangentU : TANGENT;
    float3 BiTangentU : BITANGENT;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
    float4 PosW     : WORLD_POSITION;
    float3 NormalW  : NORMAL;
	float3 TangentW : UV;
	float2 TexC     : TEXCOORD;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)objectWorld.mat);
	vout.TangentW = mul(vin.TangentU, (float3x3)objectWorld.mat);

    // Transform to homogeneous clip space.
    vout.PosW = mul(float4(vin.PosL, 1.0f), objectWorld.mat);;
    vout.PosH = mul(vout.PosW, gViewProj);

    return vout;
}

PS_OUTPUT PS(VertexOut pin)
{
    PS_OUTPUT output;
    output.Color = float4(0, 0, 1, 1);
    return output;
}
