#include "bpch.h"
#include "stateStack.h"

namespace bento
{
	void stateStack::push(state* state)
	{
		states.push(state);
	}

	void stateStack::pop()
	{
		states.pop();
	}
}


