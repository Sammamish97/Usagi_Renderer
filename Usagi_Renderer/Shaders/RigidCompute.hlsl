struct Particle {
	float4 pos;
	float4 vel;
	float4 normal;
	float pinned;
    float3 _pad0;
};

StructuredBuffer<Particle> particleIn : register(t0);
RWStructuredBuffer<Particle> particleOut : register(u0);

struct CommonData
{
	float deltaT;
	float particleMass;
	float springStiffness;
	float damping;
	float restDistH;
	float restDistV;
	float restDistD;
	float sphereRadius;
	float4 spherePos;
	float4 gravity;
	float2 particleCount;
};

cbuffer common : register(b0)
{
    CommonData params;
};

struct DrawNormal
{
    int data;
};

ConstantBuffer<DrawNormal> drawNormal : register(b1);

struct RemovePin
{
    float data;
};

ConstantBuffer<RemovePin> removePin : register(b2);

float3 springForce(float3 p0, float3 p1, float restDist)
{
	float3 dist = p0 - p1;
	return normalize(dist) * params.springStiffness * (length(dist) - restDist);
}

[numthreads(10, 10, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
    uint index = id.y * params.particleCount.x + id.x;
        if (index > params.particleCount.x * params.particleCount.y)
            return;

        if(removePin.data == 1.0)
        {
            particleOut[index].pinned = 0.0;
        }

        // Pinned?
        if (particleIn[index].pinned == 1.0) {
            particleOut[index].pos = particleIn[index].pos;
            particleOut[index].vel = float4(0, 0, 0, 0);
            return;
        }
        // Initial force from gravity
        float3 force = params.gravity.xyz * params.particleMass;

        float3 pos = particleIn[index].pos.xyz;
        float3 vel = particleIn[index].vel.xyz;

        // Spring forces from neighboring particles
        // left
        if (id.x > 0) {
            force += springForce(particleIn[index-1].pos.xyz, pos, params.restDistH);
        }
        // right
        if (id.x < params.particleCount.x - 1) {
            force += springForce(particleIn[index + 1].pos.xyz, pos, params.restDistH);
        }
        // upper
        if (id.y < params.particleCount.y - 1) {
            force += springForce(particleIn[index + params.particleCount.x].pos.xyz, pos, params.restDistV);
        }
        // lower
        if (id.y > 0) {
            force += springForce(particleIn[index - params.particleCount.x].pos.xyz, pos, params.restDistV);
        }
        // upper-left
        if ((id.x > 0) && (id.y < params.particleCount.y - 1)) {
            force += springForce(particleIn[index + params.particleCount.x - 1].pos.xyz, pos, params.restDistD);
        }
        // lower-left
        if ((id.x > 0) && (id.y > 0)) {
            force += springForce(particleIn[index - params.particleCount.x - 1].pos.xyz, pos, params.restDistD);
        }
        // upper-right
        if ((id.x < params.particleCount.x - 1) && (id.y < params.particleCount.y - 1)) {
            force += springForce(particleIn[index + params.particleCount.x + 1].pos.xyz, pos, params.restDistD);
        }
        // lower-right
        if ((id.x < params.particleCount.x - 1) && (id.y > 0)) {
            force += springForce(particleIn[index - params.particleCount.x + 1].pos.xyz, pos, params.restDistD);
        }

        force += (-params.damping * vel);

        // Integrate
        float3 f = force * (1.0 / params.particleMass);
        particleOut[index].pos = float4(pos + vel * params.deltaT + 0.5 * f * params.deltaT * params.deltaT, 1.0);
        particleOut[index].vel = float4(vel + f * params.deltaT, 0.0);

        // Sphere collision
        float3 sphereDist = particleOut[index].pos.xyz - params.spherePos.xyz;
        if (length(sphereDist) < params.sphereRadius + 0.01) {
            // If the particle is inside the sphere, push it to the outer radius
            particleOut[index].pos.xyz = params.spherePos.xyz + normalize(sphereDist) * (params.sphereRadius + 0.01);
            // Cancel out velocity
            particleOut[index].vel = float4(0, 0, 0, 0);
        }

        // Normals
        if (drawNormal.data == 1) 
        {
            float3 normal = float3(0, 0, 0);
            float3 a, b, c;
            if (id.y > 0) {
                if (id.x > 0) {
                    a = particleIn[index - 1].pos.xyz - pos;
                    b = particleIn[index - params.particleCount.x - 1].pos.xyz - pos;
                    c = particleIn[index - params.particleCount.x].pos.xyz - pos;
                    normal += cross(a,b) + cross(b,c);
                }
                if (id.x < params.particleCount.x - 1) {
                    a = particleIn[index - params.particleCount.x].pos.xyz - pos;
                    b = particleIn[index - params.particleCount.x + 1].pos.xyz - pos;
                    c = particleIn[index + 1].pos.xyz - pos;
                    normal += cross(a,b) + cross(b,c);
                }
            }
            if (id.y < params.particleCount.y - 1) {
                if (id.x > 0) {
                    a = particleIn[index + params.particleCount.x].pos.xyz - pos;
                    b = particleIn[index + params.particleCount.x - 1].pos.xyz - pos;
                    c = particleIn[index - 1].pos.xyz - pos;
                    normal += cross(a,b) + cross(b,c);
                }
                if (id.x < params.particleCount.x - 1) {
                    a = particleIn[index + 1].pos.xyz - pos;
                    b = particleIn[index + params.particleCount.x + 1].pos.xyz - pos;
                    c = particleIn[index + params.particleCount.x].pos.xyz - pos;
                    normal += cross(a,b) + cross(b,c);
                }
            }
            particleOut[index].normal = float4(normalize(normal), 0.0f);
        }
        
}
