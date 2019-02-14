#include "ThempSystem.h"
#include "ThempD3D.h"
#include "ThempResources.h"
#include "ParticleManager.h"
#include "ThempGame.h"
#include "ThempCamera.h"
#include "ParticleSystem.h"

ParticleManager::ParticleManager()
{
#ifdef _DEBUG
	m_ParticleInitShader = Themp::Resources::TRes->GetComputeShader(std::string(BASE_SHADER_PATH) + "particleInit_cs_d.cso");
	m_ParticleEmitShader = Themp::Resources::TRes->GetComputeShader(std::string(BASE_SHADER_PATH) + "particleEmit_cs_d.cso");
	m_ParticleUpdateShader = Themp::Resources::TRes->GetComputeShader(std::string(BASE_SHADER_PATH) + "particleSim_cs_d.cso");
#else
	m_ParticleInitShader = Themp::Resources::TRes->GetComputeShader(std::string(BASE_SHADER_PATH) + "particleInit_cs.cso");
	m_ParticleEmitShader = Themp::Resources::TRes->GetComputeShader(std::string(BASE_SHADER_PATH) + "particleEmit_cs.cso");
	m_ParticleUpdateShader = Themp::Resources::TRes->GetComputeShader(std::string(BASE_SHADER_PATH) + "particleSim_cs.cso");
#endif
	m_ParticleMaterial = Themp::Resources::TRes->GetMaterial("Particle", "", "particle", true);
}
ParticleSystem* ParticleManager::CreateParticleSystem()
{
	ParticleSystem* ps = new ParticleSystem(1024*1024*2);
	m_ParticleSystems.push_back(ps);
	return ps;
}
void ParticleManager::Update(double dt)
{
	if (Themp::System::m_WindowIsChanging)return;

	ID3D11DeviceContext* devcon = Themp::D3D::s_D3D->m_DevCon;

	//constant buffers to assign (first the system buffer which has time and all general info I'd potentially want, second is camera info)
	//emitter specific data is set inside the emitter functions
	ID3D11Buffer* csBuffs[2] = { Themp::D3D::s_D3D->m_CBuffer, Themp::D3D::s_D3D->ConstantBuffers[1] };
	devcon->CSSetConstantBuffers(0, 2, csBuffs);
	for (int i = 0; i < m_ParticleSystems.size(); i++)
	{
		ParticleSystem* ps = m_ParticleSystems[i];
		if (!ps->m_IsInit)
		{
			ps->Init(m_ParticleInitShader);
		}
		ps->Update(dt, m_ParticleEmitShader, m_ParticleUpdateShader);
	}

	devcon->CSSetShader(0, 0, 0);
	ID3D11Buffer* nullBuffs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr, };
	devcon->CSSetConstantBuffers(0, 5, nullBuffs);
	ID3D11ShaderResourceView* nullSRVs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr, };
	devcon->CSSetShaderResources(0, 5, nullSRVs);
	ID3D11UnorderedAccessView* nullViews[2] = { nullptr,nullptr };
	devcon->CSSetUnorderedAccessViews(0, 2, nullViews, 0);
}
void ParticleManager::Draw()
{
	ID3D11Buffer* nullBuffs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr, };
	uint32_t nullstrides[1] = { 0 };
	ID3D11DeviceContext* devcon = Themp::D3D::s_D3D->m_DevCon;
	devcon->IASetInputLayout(nullptr);
	devcon->IASetIndexBuffer(nullptr, DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, 0);
	devcon->IASetVertexBuffers(0,1, nullBuffs,nullstrides, nullstrides);
	devcon->VSSetShader(m_ParticleMaterial->m_VertexShader, 0, 0);
	devcon->PSSetShader(m_ParticleMaterial->m_PixelShader, 0, 0);
	if (m_DrawQuads)
	{
		devcon->GSSetShader(m_ParticleMaterial->m_GeometryShader, 0, 0);
	}
	else
	{
		devcon->GSSetShader(nullptr, 0, 0);
	}
	Themp::D3D::s_D3D->VSUploadConstantBuffersToGPU();
	Themp::D3D::s_D3D->PSUploadConstantBuffersToGPU();
	Themp::D3D::s_D3D->GSUploadConstantBuffersToGPU();
	for (int i = 0; i < m_ParticleSystems.size(); i++)
	{
		m_ParticleSystems[i]->Draw();
	}

	devcon->GSSetShader(nullptr, 0, 0);
	devcon->CSSetShader(0, 0, 0);
	devcon->CSSetConstantBuffers(0,5, nullBuffs);
	ID3D11ShaderResourceView* nullSRVs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr, };
	devcon->CSSetShaderResources(0, 5, nullSRVs);
	ID3D11UnorderedAccessView* nullViews[2] = { nullptr,nullptr };
	devcon->CSSetUnorderedAccessViews(0, 2, nullViews, 0);
}
ParticleManager::~ParticleManager()
{
	for (int i = 0; i < m_ParticleSystems.size(); i++)
	{
		delete m_ParticleSystems[i];
	}
	CLEAN(m_ParticleInitShader);
	CLEAN(m_ParticleEmitShader);
	CLEAN(m_ParticleUpdateShader);
}