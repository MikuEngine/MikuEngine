#pragma once

namespace engine
{
	class GameObject;
	class Scene;
	class Transform;

	// 프리팹 생성, 인스턴스화 헬퍼 스태틱 클래스

	class Prefab
	{
	public:
		static void Create(GameObject* target, const std::string& name);
		static GameObject* Instantiate(const std::string& name);
	};
}