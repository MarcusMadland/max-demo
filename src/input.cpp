#include "input.h"

Input::Input()
{
	m_mapping[Action::MoveForward] = { Action::MoveForward, [](const void* _userData)
		{
			const float axis = (float)max::inputGetGamepadAxis({0}, max::GamepadAxis::LeftY) / 32767.0f;
			if (axis < -0.1f || axis > 0.1f)
			{
				return -axis;
			}

			if (max::inputGetKeyState(max::Key::KeyW))
			{
				return 1.0f;
			}

			if (max::inputGetKeyState(max::Key::KeyS))
			{
				return -1.0f;
			}

			return 0.0f;
		} };

	m_mapping[Action::MoveRight] = { Action::MoveRight, [](const void* _userData)
		{
			const float axis = (float)max::inputGetGamepadAxis({0}, max::GamepadAxis::LeftX) / 32767.0f;
			if (axis < -0.1f || axis > 0.1f)
			{
				return axis;
			}

			if (max::inputGetKeyState(max::Key::KeyD))
			{
				return 1.0f;
			}

			if (max::inputGetKeyState(max::Key::KeyA))
			{
				return -1.0f;
			}

			return 0.0f;
		} };

	m_mapping[Action::LookUp] = { Action::LookUp, [](const void* _userData)
		{
			const float axis = (float)max::inputGetGamepadAxis({0}, max::GamepadAxis::RightY) / 32767.0f;
			if (axis < -0.1f || axis > 0.1f)
			{
				return axis;
			}

			// @todo Add mouse support

			return 0.0f;
		} };

	m_mapping[Action::LookRight] = { Action::LookRight, [](const void* _userData)
		{
			const float axis = (float)max::inputGetGamepadAxis({0}, max::GamepadAxis::RightX) / 32767.0f;
			if (axis < -0.1f || axis > 0.1f)
			{
				return axis;
			}

			// @todo Add mouse support

			return 0.0f;
		} };

	m_mapping[Action::ToggleMayaBridge] = { Action::ToggleMayaBridge, [](const void* _userData)
		{
			const bool gp = max::inputGetKeyState(max::Key::GamepadGuide);
			max::inputSetKeyState(max::Key::GamepadGuide, NULL, false);

			const bool kb = max::inputGetKeyState(max::Key::F1);
			max::inputSetKeyState(max::Key::F1, NULL, false);

			return gp ? 1.0f : kb ? 1.0f : 0.0f;
		} };
}

void Input::enable()
{
	max::inputAddMappings(0, m_mapping);
}

void Input::disable()
{
	max::inputRemoveMappings(0);
}
