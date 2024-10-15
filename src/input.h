#pragma once

namespace max { struct MouseState; };

struct Action
{
	enum Enum
	{
		MoveForward,
		MoveRight,
		MoveUp,
		LookUp,
		LookRight,

		ToggleFullscreen,
		Quit,

		Count
	};
};

struct InputSettings
{

};

void inputCreate(InputSettings* _settings, max::MouseState* _mouseState);

void inputDestroy();

void inputUpdate();

void inputEnable();

void inputDisable();

