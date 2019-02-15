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

	//Try to get as random as we can, this is such a pain.
    float2 uv = float2((float)DTid.x + _time + _random.x, _time + _random.y);
	//The texture returns a value between 0 and 1, so we multiply it with max uint32 to get a full uint range
    uint rand = DTid.x + (uint)_random.z + ((uint) (Random.Gather(WrappedPointSampler, uv).x * (float) 0xFFFFFFFF));


	//prepare to "spawn" our amount of particles
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
		//We can only spawn a integer amount of particles per thread and we can't round up because we might overshoot our buffer, so lets floor our current amount
        const uint NumParticlesPerThread = (uint) floor(particlesToEmitPerThread);
		//now due to the flooring we have lost an amount of particles, so lets calculate how many particles we'd be missing by purely flooring
		//for example, if we'd spawn 1.5 particles per thread, we'd lose half of em by flooring, so 1.5 % 1.0 = 0.5, so we make sure to add those missing few, any smaller deviation can be considered irrelevant
        const uint DivCutoff = fmod(particlesToEmitPerThread, 1.0) * totalThreads;
        //add the missing particles to the first thread
		const uint endEmit = (NumParticlesPerThread + (DTid.x == 0 ? DivCutoff : 0));
        for (uint i = 0; i < endEmit; i++)
        {
            SpawnParticle(rand);
        }
    }
}


