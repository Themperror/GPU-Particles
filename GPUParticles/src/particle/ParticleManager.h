#pragma once
#include <vector>

class ParticleSystem;
class ParticleManager
{
public:
	ParticleManager();
	ParticleSystem* CreateParticleSystem();
	void Update(double dt);
	void Draw();
	~ParticleManager();
	bool m_DrawQuads = false;

	std::vector<ParticleSystem*> m_ParticleSystems;
	ID3D11ComputeShader* m_ParticleInitShader = nullptr;
	ID3D11ComputeShader* m_ParticleEmitShader = nullptr;
	ID3D11ComputeShader* m_ParticleUpdateShader = nullptr;
	Themp::Material* m_ParticleMaterial = nullptr;
};