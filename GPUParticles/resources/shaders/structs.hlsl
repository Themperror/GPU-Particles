#include "defines.hlsl"
#pragma pack_matrix( row_major )

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};
struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
    float2 uv : UV;
};
cbuffer ObjectBuffer : register(b0)
{
	float4x4 _modelMatrix; //64
    int _misc0, _misc1, _misc2, _misc3;
};
cbuffer CameraBuffer : register(b1)
{
	float4x4 _viewMatrix;
	float4x4 _projectionMatrix;
	float4x4 _invProjectionMatrix;
    float4x4 _invViewMatrix;
    float4 _cameraPosition;
    float4 _cameraDir;
};
cbuffer ConstantBuffer : register(b2)
{
    float _screenWidth;
    float _screenHeight;
    float _visualType;
    float _time;
    float _fps;
    float3 _dummyCB;
};