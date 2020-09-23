#pragma once
#include "Window.h"
#include "stateStack.h"
#include "bento/renderer/Renderer.h"

int main(int argc, char** argv);

namespace bento
{
	class application
	{
	public:
		application();
		~application();

		void initialize(const char* title, int screenWidth, int screenHeight);
		void run();
		void clean();

		void pushState(state* state);
		void popState();

	private:
		Window window;
		Renderer renderer;
		stateStack stack;

		void start();
		void update();
		void render();

		friend int ::main(int argc, char** argv);
	};

	// to be defined in client
	application* createApplication();
}

