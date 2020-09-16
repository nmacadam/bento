#pragma once
#include "Window.h"
#include "Renderer.h"

class Engine
{
public:
	Engine();
	~Engine();

	void initialize(const char* title, int screenWidth, int screenHeight);
	void run();
	void clean();

private:
	Window window;
	Renderer renderer;

	void update();
	void render();

	//void eventLoop();
};

