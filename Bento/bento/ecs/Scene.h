#pragma once

#include <entt.hpp>

namespace bento
{
	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());

		void OnUpdate();
		void OnRender();

	private:
		entt::registry registry;

		friend class Entity;
	};
}