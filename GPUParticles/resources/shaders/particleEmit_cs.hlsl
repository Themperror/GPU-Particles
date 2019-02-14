#include "csshared.hlsl"

[numthreads(numThreads, 1, 1)]
void CShader(uint3 DTid : SV_DispatchThreadID )
{
    int currentParticleAmount = ParticleCountIn[0].x;
    if (DTid.x == 0)
    {
        uint val = 0;
        InterlockedExchange(ParticleCountOut[0], 0, val);
    }
	//make sure all threads have the same value
    AllMemoryBarrier();
	
	//Any particles that are alive get added to the "alive" buffer, so we don't have to check or skip over dead particles during the simulation.
    if (DTid.x < ParticleCountIn[0].x)
    {
        if (ParticleIn[DTid.x].age > 0)
        {
            uint index = 0;
            InterlockedAdd(ParticleCountOut[0].x, 1, index);
            ParticleOut[index] = ParticleIn[DTid.x];
        }
    }
	
    AllMemoryBarrier();

    currentParticleAmount = ParticleCountOut[0].x;
	//we need any initial non-0 seed, so the +512 is arbitrarily chosen
    float2 uv = float2((float) DTid.x + _time, _time);
    uint rand = DTid.x + 512 + (uint) (Random.Gather(WrappedLinearSampler, uv).x * (float) 0xFFFFFFFF);

	//"spawn" our amount of particles

	//make sure we never write more particles than we have allocated space for
    float particlesToEmit = (float) _emitRate * _deltaTime;
    if ((float) currentParticleAmount + particlesToEmit >= (float)_maxParticles)
    {
        particlesToEmit = (float)(_maxParticles - currentParticleAmount);
    }
    const float totalThreads = (float) (_numdispatch * numThreads);
    float particlesToEmitPerThread = particlesToEmit / totalThreads;
	
    if (particlesToEmitPerThread <= 1.0)
    {
        if (particlesToEmitPerThread < 0.000001f)return;

        const float numSpawningThreads = totalThreads * particlesToEmitPerThread;
        if (DTid.x <= floor(numSpawningThreads))
        {
			SpawnParticle(rand);
        }
    }
    else
    {
        const uint NumParticlesPerThread = (uint) floor(particlesToEmitPerThread);
        const uint DivCutoff = fmod(particlesToEmitPerThread, 1.0) * totalThreads;
        const uint endEmit = (NumParticlesPerThread + (DTid.x == 0 ? DivCutoff : 0));
        for (int i = 0; i < endEmit; i++)
        {
            SpawnParticle(rand);
        }
    }
}


