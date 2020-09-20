#pragma once

namespace bento
{
	class state
	{
	public:
		state() = default;
		virtual ~state() = default;

		virtual void start();
		virtual void update();
		virtual void render();
	};
}


