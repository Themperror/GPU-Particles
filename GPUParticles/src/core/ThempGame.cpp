#include "ThempSystem.h"
#include "ThempGame.h"
#include "ThempD3D.h"
#include "ThempCamera.h"
#include "ParticleManager.h"
#include "ParticleSystem.h"
#include "imgui.h"

float totalMouseX = 0, totalMouseY = 0;
float oldMouseX=0, oldMouseY = 0;
float mouseSensitivity = 0.15f;


void Themp::Game::Start()
{
	m_Camera = new Themp::Camera();
	m_Camera->SetTarget(XMFLOAT3(0,0,0));
	m_Camera->Rotate(0, 0);
	m_Camera->SetAspectRatio(Themp::D3D::s_D3D->m_ScreenWidth / Themp::D3D::s_D3D->m_ScreenHeight);
	m_Camera->SetFoV(75);
	m_Camera->SetProjection(Camera::CameraType::Perspective);
	m_Camera->SetNear(0.1f);
	m_Camera->SetFar(1000.0f);
	m_Camera->SetPosition(0, 0, -400);
	m_Camera->SetTarget(XMFLOAT3(0, 0, -400));

	m_ParticleManager = new ParticleManager();
	ParticleSystem* ps = m_ParticleManager->CreateParticleSystem();
	
}

void Themp::Game::Update(double dt)
{
	m_CursorWindowedX = m_CursorWindowedX < 0 ? 0 : m_CursorWindowedX > D3D::s_D3D->m_ScreenWidth ? D3D::s_D3D->m_ScreenWidth : m_CursorWindowedX;
	m_CursorWindowedY = m_CursorWindowedY < 0 ? 0 : m_CursorWindowedY > D3D::s_D3D->m_ScreenHeight ? D3D::s_D3D->m_ScreenHeight : m_CursorWindowedY;

	m_Camera->SetAspectRatio(Themp::D3D::s_D3D->m_ScreenWidth / Themp::D3D::s_D3D->m_ScreenHeight);
	m_CursorDeltaX = oldMouseX - m_CursorWindowedX;
	m_CursorDeltaY = oldMouseY - m_CursorWindowedY;

	bool changedData = false;
	ParticleSystem::ParticleSystemBuffer& cBufferData = m_ParticleManager->m_ParticleSystems[0]->m_ParticleSystemConstantBufferData;

	ImGui::Text("Particle Count: %i", m_ParticleManager->m_ParticleSystems[0]->m_CurrentParticleCount);
	ImGui::Checkbox("Draw Quads", &m_ParticleManager->m_DrawQuads);
	ImGui::SliderInt("Particles Per Second", &cBufferData.rate, 0, 100000);
	ImGui::SliderFloat("Gravity", &cBufferData.gravity, 0.1f, 100.0f);
	ImGui::SliderFloat("Mass", &cBufferData.mass, 0.1f, 10000.0f);
	ImGui::SliderFloat2("Lifetime (Min/Max)", &cBufferData.LifeTimeMin, 0, 30);
	ImGui::SliderFloat2("Scale (Min/Max)", &cBufferData.ScaleMin, 0, 30);
	ImGui::SliderFloat3("Min Velocity", (float*)&cBufferData.VelocityMin, -100, 100);
	ImGui::SliderFloat3("Max Velocity", (float*)&cBufferData.VelocityMax, -100, 100);
	ImGui::SliderFloat3("Min Position", (float*)&cBufferData.PositionMin, -100, 100);
	ImGui::SliderFloat3("Max Position", (float*)&cBufferData.PositionMax, -100, 100);

	//left mouse button
	if (m_Keys[257])
	{
		totalMouseX += m_CursorDeltaX * mouseSensitivity;
		totalMouseY += m_CursorDeltaY * mouseSensitivity;

		totalMouseY = totalMouseY > 90.0f ? 90.0f : totalMouseY < -90.0f ? -90.0f : totalMouseY;
		m_Camera->Rotate(totalMouseX, totalMouseY);
	}
	float speedMod = 0.05f;
	if (m_Keys[VK_SHIFT])
	{
		speedMod = 0.2f;
	}
	m_Camera->SetSpeed(speedMod);
	if (m_Keys['W'])
	{
		m_Camera->MoveForward();
	}
	if (m_Keys['S'])
	{
		m_Camera->MoveBackward();
	}
	if (m_Keys['A'])
	{
		m_Camera->MoveLeft();
	}
	if (m_Keys['D'])
	{
		m_Camera->MoveRight();
	}
	//spacebar
	if (m_Keys[32] || m_Keys['E'])
	{
		m_Camera->MoveUp();
	}
	if (m_Keys['X'] || m_Keys['Q'])
	{
		m_Camera->MoveDown();
	}

	m_Camera->Update((float)dt);
	m_Camera->UpdateMatrices();
	oldMouseX = m_CursorWindowedX;
	oldMouseY = m_CursorWindowedY;

	m_ParticleManager->Update(dt);
}
void Themp::Game::Stop()
{
	delete m_Camera;
	m_Camera = nullptr;
	delete m_ParticleManager;
	m_ParticleManager = nullptr;
}