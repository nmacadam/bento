#pragma once

#include "Scene.h"

#include <entt.hpp>
#include "bento/core/log.h"

namespace bento
{
	class Entity
	{
	public:
		Entity(entt::entity handle, Scene* scene);
		Entity(const Entity& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			// change to assert
			if (HasComponent<T>())
			{
				bento::log::warn("entity already has component");
			}

			// don't unpack everything here, forward it off to entt
			return scene->registry.emplace<T>(entityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			// change to assert
			if (!HasComponent<T>())
			{
				bento::log::warn("entity does not have component");
			}

			// don't unpack everything here, forward it off to entt
			return scene->registry.get<T>(entityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return scene->registry.has<T>(entityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			// change to assert
			if (!HasComponent<T>())
			{
				bento::log::warn("entity does not have component");
			}

			scene->registry.remove<T>(entityHandle);
		}

		operator bool() const { return entityHandle != 0; }

	private:
		entt::entity entityHandle{ 0 };

		// use a weak reference
		Scene* scene = nullptr;
	};

}
