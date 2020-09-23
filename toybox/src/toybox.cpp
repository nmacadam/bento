#include <bento.h>

class toyboxState : public bento::state
{
public:
	toyboxState()
	{
		scene = std::make_shared<bento::Scene>();

		//IMGUI_CHECKVERSION();
	}

	void start() override
	{
		bento::log::info("state start"); // is this right?

		auto entity = scene->CreateEntity("new entity");

		//entity.AddComponent<bento::TransformComponent>(glm::mat4(1.0f));
		entity.AddComponent<bento::MeshComponent>(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	}

	void update() override
	{
		//bento::log::info("state update");
		scene->OnUpdate();
	}

	void render() override
	{
		//bento::log::info("state render");
		scene->OnRender();
	}

private:
	std::shared_ptr<bento::Scene> scene;
};

class toybox : public bento::application
{
public:
	toybox()
	{
		state = new toyboxState();
		pushState(state);
	}

	~toybox()
	{
		delete state;
	}

private:
	bento::state* state;
};

bento::application* bento::createApplication()
{
	return new toybox();
}
