#pragma once
#include <stack>

#include "state.h"

namespace bento
{
	class stateStack
	{
	public:
		stateStack() = default;
		~stateStack() = default;

		void push(state* state);
		void pop();

		state* top() { return states.top(); }

	private:
		std::stack<state*> states;
	};
}

