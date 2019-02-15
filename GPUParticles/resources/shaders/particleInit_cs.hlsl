#include "csshared.hlsl"

[numthreads(numThreads, 1, 1)]
void CShader(uint3 DTid : SV_DispatchThreadID)
{
	//make sure we never init more than we have space for
    uint particlesToEmit = clamp(_newParticles, 0, _maxParticles);

    ParticleOut[DTid.x].age = 0;
    ParticleOut[DTid.x].color = float4(1, 1, 1, 1);
    ParticleOut[DTid.x].position = float4(0, 0, 0, 0);
    ParticleOut[DTid.x].scale = 0;
    ParticleOut[DTid.x].velocity = float4(1, 1, 1,0);
	
	//we need any initial non-0 seed, so the +512 is arbitrarily chosen
	//as an init we don't need as much random as we use on emit since this only happens the very first frame
    uint rand = DTid.x + 512;

	//"spawn" our initial amount of particles
    if(DTid.x <= particlesToEmit)
    {
        SpawnParticle(rand);
    }
}