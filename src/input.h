#pragma once

#include <max/max.h>

struct Action
{
	enum Enum
	{
		MoveForward,
		MoveRight,
		MoveUp,
		LookUp,
		LookRight,

		ToggleMayaBridge,
		PlayerCamera,
		DebugPlayerCamera,
		Quit,

		Count
	};
};

struct Input
{
	Input();

	void update();

	void enable();
	void disable();

	bool m_enable;

	float m_mouse[3];
	max::InputMapping m_mapping[Action::Count];
};