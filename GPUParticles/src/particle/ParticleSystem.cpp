#include "ThempSystem.h"
#include "ThempD3D.h"
#include "ParticleSystem.h"
#include "ThempFunctions.h"
#include <ppl.h>
#include <imgui.h>
#include <random>
ParticleSystem::ParticleSystem(int maxParticles)
{
	m_ParticleSystemConstantBufferData.maxParticles = maxParticles;
	m_CurrentParticleCount = m_ParticleSystemConstantBufferData.newParticles;

	//create buffer for dispatch indirect and assign data
	m_DispatchBufferData = { 1 ,1, 1};

	D3D11_BUFFER_DESC desc ={0};
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(DispatchBuffer);
	desc.StructureByteStride = 0;

	Themp::D3D::s_D3D->m_Device->CreateBuffer(&desc,nullptr, &m_DispatchBuffer);


	//map and assign info
	D3D11_MAPPED_SUBRESOURCE ms {0};
	Themp::D3D::s_D3D->m_DevCon->Map(m_DispatchBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &ms);
	memcpy(ms.pData, &m_DispatchBufferData, sizeof(DispatchBuffer));
	Themp::D3D::s_D3D->m_DevCon->Unmap(m_DispatchBuffer, 0);

	//Create our particle system constant buffer
	desc = { 0 };
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(ParticleSystemBuffer);

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &m_ParticleSystemConstantBufferData;
	Themp::D3D::s_D3D->m_Device->CreateBuffer(&desc, &data, &m_ParticleSystemConstantBuffer);


	D3D11_DRAW_INSTANCED_INDIRECT_ARGS instancedArgs{ 0 };
	instancedArgs.InstanceCount = 1;
	instancedArgs.VertexCountPerInstance = maxParticles;
	data.pSysMem = &instancedArgs;

	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(D3D11_DRAW_INSTANCED_INDIRECT_ARGS);

	Themp::D3D::s_D3D->m_Device->CreateBuffer(&desc, &data, &m_InstancedDrawBuffer);

	//create our particles buffers,srvs and uavs
	//and create the particle count buffer,srv and uav


	uint32_t count = 0;
	for (int i = 0; i < 2; i++)
	{
		m_Particles[i].InitBuf(maxParticles * sizeof(Particle), sizeof(Particle));
		m_Particles[i].InitUAV();
		m_Particles[i].InitSRV();

		m_ParticleCount[i].InitBuf(sizeof(uint32_t), sizeof(uint32_t), D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ, &count);
		m_ParticleCount[i].InitUAV();
		m_ParticleCount[i].InitSRV();
	}

	//create texture with random noise
	m_RandomTexture = new Themp::Texture();
	char* randompixels = new char[1024 * 1024*4];
	std::default_random_engine generator;
	for (size_t i = 0; i < 1024 * 1024 * 4; i++)
	{
		std::uniform_int_distribution<int> distribution(0, 255);
		randompixels[i] = distribution(generator);
	}
	m_RandomTexture->Create(1024, 1024, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, false, randompixels);
	delete[] randompixels;

}
ParticleSystem::~ParticleSystem()
{
	delete m_RandomTexture;
	CLEAN(m_DispatchBuffer);
	CLEAN(m_ParticleSystemConstantBuffer);
	CLEAN(m_InstancedDrawBuffer);

}
void ParticleSystem::UpdateDispatchBuffer()
{
	//figure out how many dispatch threads we need, and limit it if need ever be..
	m_ParticleSystemConstantBufferData.numDispatch = Themp::Clamp((int)((m_CurrentParticleCount / 512) + (m_CurrentParticleCount % 512)), 1, D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);

	//update our buffers so the cs knows about the new data
	{
		ForceUpdateBuffer();
		m_DispatchBufferData.x = m_ParticleSystemConstantBufferData.numDispatch;
		D3D11_MAPPED_SUBRESOURCE ms{ 0 };
		Themp::D3D::s_D3D->m_DevCon->Map(m_DispatchBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &ms);
		memcpy(ms.pData, &m_DispatchBufferData, sizeof(DispatchBuffer));
		Themp::D3D::s_D3D->m_DevCon->Unmap(m_DispatchBuffer, 0);
	}
}
void ParticleSystem::ForceUpdateBuffer()
{
	D3D11_MAPPED_SUBRESOURCE ms{ 0 };
	Themp::D3D::s_D3D->m_DevCon->Map(m_ParticleSystemConstantBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &ms);
	memcpy(ms.pData, &m_ParticleSystemConstantBufferData, sizeof(ParticleSystemBuffer));
	Themp::D3D::s_D3D->m_DevCon->Unmap(m_ParticleSystemConstantBuffer, 0);
}

void ParticleSystem::Init(ID3D11ComputeShader* particleInitShader)
{
	m_IsInit = true;
	ID3D11DeviceContext* devcon = Themp::D3D::s_D3D->m_DevCon;
	m_CurrentParticleCount = m_ParticleSystemConstantBufferData.newParticles;
	UpdateDispatchBuffer();
	//compute shader to use
	devcon->CSSetShader(particleInitShader, nullptr, 0);
	//set constant buffers
	devcon->CSSetConstantBuffers(2, 1, &m_ParticleSystemConstantBuffer);
	//set SRV's
	ID3D11ShaderResourceView* srvs[3] = { m_Particles[!m_IsBackBuffer].srv,m_ParticleCount[!m_IsBackBuffer].srv, m_RandomTexture->m_View };
	devcon->CSSetShaderResources(0, 3, srvs);
	//set UAV's
	ID3D11UnorderedAccessView* uavs[2] = { m_Particles[m_IsBackBuffer].uav, m_ParticleCount[m_IsBackBuffer].uav };
	devcon->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
	//finally call the shader and swap the particle buffers
	devcon->DispatchIndirect(m_DispatchBuffer, 0);


	ID3D11ShaderResourceView* nullSRVs[3] = { nullptr,nullptr,m_RandomTexture->m_View };
	ID3D11UnorderedAccessView* nullUAVs[2] = { nullptr,nullptr };
	devcon->CSSetShaderResources(0, 3, nullSRVs);
	devcon->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);

	m_IsBackBuffer = !m_IsBackBuffer;
}
void ParticleSystem::Update(double dt, ID3D11ComputeShader* particleEmitShader, ID3D11ComputeShader* particleUpdateShader)
{
	ID3D11DeviceContext* devcon = Themp::D3D::s_D3D->m_DevCon;

	ID3D11ShaderResourceView* nullSRVs[2] = { nullptr,nullptr };
	ID3D11UnorderedAccessView* nullUAVs[2] = { nullptr,nullptr };

	UpdateDispatchBuffer();

	//Emit any new particles
	{
		//compute shader to use
		devcon->CSSetShader(particleEmitShader, nullptr, 0);
		//set constant buffers
		devcon->CSSetConstantBuffers(2, 1, &m_ParticleSystemConstantBuffer);
		//set SRV's
		ID3D11ShaderResourceView* srvs[3] = { m_Particles[!m_IsBackBuffer].srv, m_ParticleCount[!m_IsBackBuffer].srv , m_RandomTexture->m_View};
		devcon->CSSetShaderResources(0, 3, srvs);
		//set UAV's
		ID3D11UnorderedAccessView* uavs[2] = { m_Particles[m_IsBackBuffer].uav, m_ParticleCount[m_IsBackBuffer].uav };
		devcon->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
		//finally call the shader
		devcon->DispatchIndirect(m_DispatchBuffer, 0);
	
		devcon->CSSetShaderResources(0, 2, nullSRVs);
		devcon->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	
		//and swap the particle buffers
		m_IsBackBuffer = !m_IsBackBuffer;
	}

	//Simulate particles //same procedure as above
	{
		devcon->CSSetShader(particleUpdateShader, nullptr, 0);
		devcon->CSSetConstantBuffers(2, 1, &m_ParticleSystemConstantBuffer);
		ID3D11ShaderResourceView* srvs[3] = { m_Particles[!m_IsBackBuffer].srv, m_ParticleCount[!m_IsBackBuffer].srv , m_RandomTexture->m_View };
		devcon->CSSetShaderResources(0, 3, srvs);
		ID3D11UnorderedAccessView* uavs[2] = { m_Particles[m_IsBackBuffer].uav, m_ParticleCount[m_IsBackBuffer].uav };
		devcon->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
		devcon->DispatchIndirect(m_DispatchBuffer, 0);

		//and swap the particle buffers
		m_IsBackBuffer = !m_IsBackBuffer;
	}
	devcon->CSSetShaderResources(0, 2, nullSRVs);
	devcon->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	
}
void ParticleSystem::Draw()
{
	ID3D11DeviceContext* devcon = Themp::D3D::s_D3D->m_DevCon;
	//read our particle count and prepare the InstancedDrawBuffer for drawing
	D3D11_MAPPED_SUBRESOURCE msDraw{ 0 };
	D3D11_MAPPED_SUBRESOURCE msCount{ 0 };
	D3D11_DRAW_INSTANCED_INDIRECT_ARGS instancedArgs{ 0 };

	devcon->Map(m_InstancedDrawBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &msDraw);
	devcon->Map(m_ParticleCount[m_IsBackBuffer].buf, 0, D3D11_MAP::D3D11_MAP_READ, 0, &msCount);

	instancedArgs.InstanceCount = 1;
	instancedArgs.VertexCountPerInstance = *(int*)msCount.pData;
	msDraw.pData = memcpy(msDraw.pData, &instancedArgs, sizeof(D3D11_DRAW_INSTANCED_INDIRECT_ARGS));

	devcon->Unmap(m_ParticleCount[m_IsBackBuffer].buf, 0);
	devcon->Unmap(m_InstancedDrawBuffer, 0);

	m_CurrentParticleCount = instancedArgs.VertexCountPerInstance;
	
	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	devcon->VSSetShaderResources(0, 1, &m_Particles[!m_IsBackBuffer].srv);
	devcon->DrawInstancedIndirect(m_InstancedDrawBuffer, 0);


	ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
	devcon->VSSetShaderResources(0, 1, nullSRVs);
}
void ParticleSystem::Spawn(int numParticles)
{

}