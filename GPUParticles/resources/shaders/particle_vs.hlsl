#include "structs.hlsl"

struct Particle
{
    float age, scale, d1, d2;
    float3 velocity;
    float d3;
    float4 color;
    float4 position;
    float4 prevPosition;
};

StructuredBuffer<Particle> ParticleIn : register(t0);

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv : TEXCOORD0;
    float scale : SCALE;
};

PS_INPUT VShader(uint vIndex : SV_VertexID)
{
    PS_INPUT output;
    output.pos = mul(float4(ParticleIn[vIndex].position.xyz, 1.0), mul(_viewMatrix, _projectionMatrix));
    output.col = ParticleIn[vIndex].color;
    output.uv = float2(0.5, 0.5);
    output.scale = ParticleIn[vIndex].scale;
    return output;
};