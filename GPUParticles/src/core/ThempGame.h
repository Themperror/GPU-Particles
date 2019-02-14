#pragma once
#include <vector>

class ParticleManager;
namespace Themp
{
	class Object3D;
	class Camera;
	//class Resources;
	//struct Texture;
	class Game
	{
	public:
		Game() {};
		void Start();
		void Update(double dt);
		void Stop();
		void AddObject3D(Object3D* obj);
		std::vector<Themp::Object3D*> m_Objects3D;
		Camera* m_Camera = nullptr;

		signed char m_Keys[258];//Keeping track of input;
		POINT m_CursorPos;
		float m_CursorWindowedX, m_CursorWindowedY;
		float m_CursorDeltaX, m_CursorDeltaY;

		ParticleManager* m_ParticleManager = nullptr;
	};
};
