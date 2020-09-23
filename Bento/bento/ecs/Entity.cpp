#include "bpch.h"
#include "Entity.h"

namespace bento
{
	Entity::Entity(entt::entity handle, Scene* scene)
		: entityHandle(handle), scene(scene)
	{ }


}
