#pragma once
#include "ThempSystem.h"
#include <DirectXMath.h>
#include "ThempMaterial.h"

using namespace DirectX;
class ParticleSystem
{
	struct Particle
	{
		float age, scale, dummy1, dummy2;
		XMFLOAT4 velocity;
		XMFLOAT4 color;
		XMFLOAT4 position;
		XMFLOAT4 prevPosition;
	};
	class ComputeData
	{
	public:
		void InitBuf(int data_size, int structSize, D3D11_CPU_ACCESS_FLAG cpuType = (D3D11_CPU_ACCESS_FLAG)0, void* initData = nullptr)
		{
			D3D11_BUFFER_DESC desc{ 0 };
			desc.ByteWidth = data_size;
			desc.StructureByteStride = structSize;
			desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			desc.CPUAccessFlags = cpuType;
			if (initData != nullptr)
			{
				D3D11_SUBRESOURCE_DATA data;
				data.pSysMem = initData;
				Themp::D3D::s_D3D->m_Device->CreateBuffer(&desc, &data, &buf);
			}
			else
			{
				Themp::D3D::s_D3D->m_Device->CreateBuffer(&desc, nullptr, &buf);
			}

		}
		void InitUAV()
		{
			if (!buf) 
			{
				Themp::System::Print("Can't Init UAV, Buf wasn't initialized yet!");
				return;
			}

			D3D11_BUFFER_DESC bufDesc{ 0 };
			buf->GetDesc(&bufDesc);
		
			D3D11_UNORDERED_ACCESS_VIEW_DESC UAVdesc;
			ZeroMemory(&UAVdesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
			UAVdesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			UAVdesc.Buffer.FirstElement = 0;
			UAVdesc.Format = DXGI_FORMAT_UNKNOWN;
			UAVdesc.Buffer.NumElements = bufDesc.ByteWidth / bufDesc.StructureByteStride;
			Themp::D3D::s_D3D->m_Device->CreateUnorderedAccessView(buf, &UAVdesc, &uav);
		}
		void InitSRV()
		{
			if (!buf)
			{
				Themp::System::Print("Can't Init SRV, Buf wasn't initialized yet!");
				return;
			}

			D3D11_BUFFER_DESC bufDesc{ 0 };
			buf->GetDesc(&bufDesc);

			D3D11_SHADER_RESOURCE_VIEW_DESC SRVdesc;
			ZeroMemory(&SRVdesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

			SRVdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
			SRVdesc.Format = DXGI_FORMAT_UNKNOWN;
			SRVdesc.BufferEx.FirstElement = 0;
			SRVdesc.BufferEx.NumElements = bufDesc.ByteWidth / bufDesc.StructureByteStride;

			Themp::D3D::s_D3D->m_Device->CreateShaderResourceView(buf, &SRVdesc, &srv);
		}
		~ComputeData()
		{
			CLEAN(buf);
			CLEAN(srv);
			CLEAN(uav);
		}
		ID3D11Buffer* buf = nullptr;
		ID3D11ShaderResourceView* srv = nullptr;
		ID3D11UnorderedAccessView* uav = nullptr;
	};
	struct DispatchBuffer
	{
		uint32_t x;
		uint32_t y;
		uint32_t z;
	};
public:
	struct ParticleSystemBuffer
	{
		float LifeTimeMin = 10.0f, LifeTimeMax = 20.0f;
		float ScaleMin = 1.0f, ScaleMax = 2.0f;
		uint32_t newParticles = 1024*256, maxParticles = 1024*1024*2;
		float gravity = 100.0f, mass = 5000.0f; //8
		int rate = 50000, numDispatch = 1, d2, d3; // 8
		XMFLOAT4 VelocityMin = XMFLOAT4(-30,-30,-30, 0), VelocityMax = XMFLOAT4(30, 30, 30, 0);
		XMFLOAT4 PositionMin = XMFLOAT4(-100, -100, -100, 0), PositionMax = XMFLOAT4(100, 100, 100, 0);
		XMFLOAT4 Position = XMFLOAT4(0, 0, 0, 1);
	};
	ParticleSystem(int maxParticles);
	~ParticleSystem();
	void Init(ID3D11ComputeShader* particleInitShader);
	void Update(double dt, ID3D11ComputeShader * particleEmitShader, ID3D11ComputeShader * particleUpdateShader);
	void Draw();
	void Spawn(int numParticles);
	void ForceUpdateBuffer();
	void UpdateDispatchBuffer();

	ParticleSystemBuffer m_ParticleSystemConstantBufferData;
	ID3D11Buffer* m_ParticleSystemConstantBuffer = nullptr;

	DispatchBuffer m_DispatchBufferData;
	ID3D11Buffer* m_DispatchBuffer = nullptr;
	ID3D11Buffer* m_InstancedDrawBuffer = nullptr;

	ComputeData m_ParticleCount[2];
	ComputeData m_Particles[2];

	Themp::Texture* m_RandomTexture = nullptr;
	bool m_IsInit = false;
	bool m_IsBackBuffer = false;
	unsigned int m_CurrentParticleCount = 0;
};