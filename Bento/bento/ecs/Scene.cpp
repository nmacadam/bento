#include "bpch.h"
#include "Scene.h"
#include "Components.h"

#include <glm/mat4x4.hpp>
#include "bento/core/log.h"
#include "Entity.h"

namespace bento
{
	Scene::Scene()
	{
		//struct MeshComponent
		//{
		//	bool data;
		//};

		//struct TransformComponent
		//{
		//	glm::mat4 transform;

		//	TransformComponent() = default;
		//	TransformComponent(const TransformComponent&) = default;
		//	TransformComponent(const glm::mat4& transform)
		//		: transform(transform) {}

		//	//operator glm::mat4&() { return transform; }
		//	//operator const glm::mat4&() const { return transform; }
		//};

		//// this is basically an integer
		//entt::entity entity = registry.create();

		//// adding
		//registry.emplace<TransformComponent>(entity, glm::mat4(1.0f));

		//// add and retrieve reference
		//auto& transformReference = registry.emplace<TransformComponent>(entity, glm::mat4(1.0f));

		//// removing
		//registry.remove<TransformComponent>(entity);

		//// get
		//registry.get<TransformComponent>(entity);

		//// has?
		//registry.has<TransformComponent>(entity);

		//// iterate through all with transform
		//auto view = registry.view<TransformComponent>();
		//for (auto entity : view)
		//{
		//	auto& transformReference = view.get<TransformComponent>(entity);

		//	// do stuff
		//}

		//// use groups for multiple components that need each other
		//auto group = registry.group<TransformComponent>(entt::get<MeshComponent>);
		//for (auto entity : group)
		//{
		//	auto&[transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

		//	// renderer.submit(mesh, transform)

		//	// do stuff
		//}
	}

	Scene::~Scene()
	{
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity { registry.create(), this };

		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.tag = name.empty() ? "entity" : name;

		return entity;
	}

	void Scene::OnUpdate()
	{
		//auto group = registry.group<TransformComponent>(entt::get<MeshComponent>);
		//for (auto entity : group)
		//{
		//	auto&[transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

		//	// renderer.submit(mesh, transform)

		//	// do stuff
		//}
	}

	void Scene::OnRender()
	{
		auto group = registry.group<TransformComponent>(entt::get<MeshComponent>);
		for (auto entity : group)
		{
			auto[transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);
			// draw!

			//log::warn("color is r:{0} g:{1} b:{2}", mesh.color.r, mesh.color.g, mesh.color.b);
		}
	}
}
