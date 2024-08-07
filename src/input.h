#pragma once

#include <max/max.h>

struct Action
{
	enum Enum
	{
		MoveForward,
		MoveRight,
		LookUp,
		LookRight,

		ToggleMayaBridge,

		Count
	};
};

struct Input
{
	Input();

	void enable();
	void disable();

	max::InputMapping m_mapping[Action::Count];
};