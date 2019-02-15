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

	///////////////////////////////////
	//Create and set up all our buffers

	//create buffer for dispatch indirect and assign data
	m_ParticleSystemConstantBufferData.numDispatch = Themp::Clamp((int)((m_CurrentParticleCount / 512) + (m_CurrentParticleCount % 512)), 1, D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);
	m_DispatchBufferData={ 1 ,1, 1};
	m_DispatchBufferData.x = m_ParticleSystemConstantBufferData.numDispatch;

	D3D11_BUFFER_DESC desc ={0};
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(DispatchBuffer);
	desc.StructureByteStride = 0;

	Themp::D3D::s_D3D->m_Device->CreateBuffer(&desc,nullptr, &m_DispatchBuffer);

	//create our buffer for reading particle count on CPU
	desc = { 0 };
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.ByteWidth = sizeof(uint32_t);

	Themp::D3D::s_D3D->m_Device->CreateBuffer(&desc, nullptr, &m_CPUParticleCountReadBuffer);

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

	///////////////////////////////////

	///////////////////////////////////
	//create our particles buffers,srvs and uavs
	//and create the particle count buffer,srv and uav


	uint32_t count = m_CurrentParticleCount;
	for (int i = 0; i < 2; i++)
	{
		m_Particles[i].InitBuf(maxParticles * sizeof(Particle), sizeof(Particle));
		m_Particles[i].InitUAV();
		m_Particles[i].InitSRV();

		m_ParticleCount[i].InitBuf(sizeof(uint32_t), sizeof(uint32_t),(D3D11_CPU_ACCESS_FLAG)0, &count);
		m_ParticleCount[i].InitUAV();
		m_ParticleCount[i].InitSRV();
	}

	///////////////////////////////////

	///////////////////////////////////
	//create texture with random noise
	m_RandomTexture = new Themp::Texture();
	float* randompixels = new float[2048 * 2048];
	std::default_random_engine generator;
	for (size_t i = 0; i < 2048 * 2048; i++)
	{
		std::uniform_real_distribution<float> distribution(0, 1);
		randompixels[i] = distribution(generator);
	}
	m_RandomTexture->Create(2048, 2048, DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT, false, randompixels);

	//no longer need to store the data on CPU so we delete the input
	delete[] randompixels;
	///////////////////////////////////
}
ParticleSystem::~ParticleSystem()
{
	delete m_RandomTexture;
	CLEAN(m_DispatchBuffer);
	CLEAN(m_ParticleSystemConstantBuffer);
	CLEAN(m_InstancedDrawBuffer);
	CLEAN(m_CPUParticleCountReadBuffer);

}
void ParticleSystem::UpdateDispatchBuffer()
{
	//figure out how many dispatch threads we need, and limit it if need ever be..
	m_ParticleSystemConstantBufferData.numDispatch = Themp::Clamp((int)((m_CurrentParticleCount / 512) + (m_CurrentParticleCount % 512)), 1, D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);

	//update our buffers so the compute shader knows about the new data
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
	//finally call the shader 
	devcon->DispatchIndirect(m_DispatchBuffer, 0);

	//set everything to null (the texture can stay)
	ID3D11ShaderResourceView* nullSRVs[3] = { nullptr,nullptr,m_RandomTexture->m_View };
	ID3D11UnorderedAccessView* nullUAVs[2] = { nullptr,nullptr };
	devcon->CSSetShaderResources(0, 3, nullSRVs);
	devcon->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);

	//and swap the particle buffers
	m_IsBackBuffer = !m_IsBackBuffer;
}
void ParticleSystem::Update(double dt, ID3D11ComputeShader* particleEmitShader, ID3D11ComputeShader* particleUpdateShader)
{
	ID3D11DeviceContext* devcon = Themp::D3D::s_D3D->m_DevCon;

	ID3D11ShaderResourceView* nullSRVs[2] = { nullptr,nullptr };
	ID3D11UnorderedAccessView* nullUAVs[2] = { nullptr,nullptr };
	
	//make sure we have the latest data
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
	//////////////////////
	//read our particle count and prepare the InstancedDrawBuffer for drawing
	D3D11_MAPPED_SUBRESOURCE msDraw{ 0 };
	D3D11_MAPPED_SUBRESOURCE msCount{ 0 };
	D3D11_DRAW_INSTANCED_INDIRECT_ARGS instancedArgs{ 0 };

	devcon->CopyResource(m_CPUParticleCountReadBuffer, m_ParticleCount[m_IsBackBuffer].buf);
	devcon->Map(m_InstancedDrawBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &msDraw);
	devcon->Map(m_CPUParticleCountReadBuffer, 0, D3D11_MAP::D3D11_MAP_READ, 0, &msCount);

	instancedArgs.InstanceCount = 1;
	//copy the count of particles we have, we'll use this as our vertex count
	instancedArgs.VertexCountPerInstance = *(int*)msCount.pData;
	memcpy(msDraw.pData, &instancedArgs, sizeof(D3D11_DRAW_INSTANCED_INDIRECT_ARGS));

	devcon->Unmap(m_CPUParticleCountReadBuffer, 0);
	devcon->Unmap(m_InstancedDrawBuffer, 0);
	//////////////////////

	m_CurrentParticleCount = instancedArgs.VertexCountPerInstance;
	
	//////////////////////
	//set our particle buffer as a srv in the vertex buffer, at this point we have no vertex buffer bound and proceed to create points from the particle list
	//then we transform those points to quads
	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	devcon->VSSetShaderResources(0, 1, &m_Particles[!m_IsBackBuffer].srv);
	devcon->DrawInstancedIndirect(m_InstancedDrawBuffer, 0);
	//////////////////////

	//don't forget to set our SRV back to null as we have to use it again next frame
	ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
	devcon->VSSetShaderResources(0, 1, nullSRVs);
}
void ParticleSystem::Spawn(int numParticles)
{

}