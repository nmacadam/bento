#include <bento.h>

class toyboxState : public bento::state
{
	void start() override
	{
		bento::log::info("state start");
	}

	void update() override
	{
		bento::log::info("state update");
	}

	void render() override
	{
		bento::log::info("state render");
	}
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
