struct VertexIn
{
    float4 pos : POSITION;
	float4 vel : VELOCITY;
	float4 normal : NORMAL;
	float pinned : PINNED;
    float3 _pad0 : PAD;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
    float3 NormalW  : NORMAL;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
};
struct ViewProj
{
    matrix mat;
};

ConstantBuffer<ViewProj> gViewProj : register(b0);

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;

    vout.NormalW = vin.normal.xyz;

    vout.PosH = mul(vin.pos, gViewProj.mat);
    return vout;
}

PS_OUTPUT PS(VertexOut pin)
{
    float3 albedo = float3(1, 1, 1);
    float3 surfaceNormal = normalize(pin.NormalW);
    float3 lightDir = float3(0, 1, 0);
    float ndotl = dot(surfaceNormal, lightDir);

    PS_OUTPUT output;
    output.Color = float4(albedo * ndotl, 1);
    return output;
}
