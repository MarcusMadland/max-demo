#include "input.h"

#include <max/max.h>

struct InputSystem
{
	void create(InputSettings* _settings, max::MouseState* _mouseState)
	{
		m_settings = _settings;
		m_mouseState = _mouseState;

		m_enable = false;

		m_mouse[0] = 0.0f;
		m_mouse[1] = 0.0f;
		m_mouse[2] = 0.0f;

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
			const float axis = -(float)max::inputGetGamepadAxis({0}, max::GamepadAxis::LeftX) / 32767.0f;
			if (axis < -0.1f || axis > 0.1f)
			{
				return axis;
			}

			if (max::inputGetKeyState(max::Key::KeyD))
			{
				return -1.0f;
			}

			if (max::inputGetKeyState(max::Key::KeyA))
			{
				return 1.0f;
			}

			return 0.0f;
		} };

		m_mapping[Action::MoveUp] = { Action::MoveUp, [](const void* _userData)
		{
			uint8_t modifier = 0 | max::Modifier::LeftCtrl;

			// @todo

			if (max::inputGetKeyState(max::Key::Space) || max::inputGetKeyState(max::Key::GamepadShoulderR))
			{
				//return 1.0f;
			}

			if (max::inputGetKeyState(max::Key::None, &modifier) || max::inputGetKeyState(max::Key::GamepadShoulderL))
			{
				//return -1.0f;
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

			InputSystem* input = (InputSystem*)_userData;

			if (input->m_mouseState->m_buttons[max::MouseButton::Left])
			{
				//return input->m_mouse[1] * 100.0f;
			}

			return 0.0f;

		}, this };

		m_mapping[Action::LookRight] = { Action::LookRight, [](const void* _userData)
		{
			const float axis = -(float)max::inputGetGamepadAxis({0}, max::GamepadAxis::RightX) / 32767.0f;
			if (axis < -0.1f || axis > 0.1f)
			{
				return axis;
			}

			InputSystem* input = (InputSystem*)_userData;

			if (input->m_mouseState->m_buttons[max::MouseButton::Left])
			{
				//return input->m_mouse[0] * -100.0f;
			}

			return 0.0f;

		}, this };

		m_mapping[Action::ToggleFullscreen] = { Action::ToggleFullscreen, [](const void* _userData)
		{
			const bool kb = max::inputGetKeyState(max::Key::F11);
			max::inputSetKeyState(max::Key::F11, NULL, false);

			return kb ? 1.0f : 0.0f;
		} };

		m_mapping[Action::Quit] = { Action::Quit, [](const void* _userData)
		{
			const bool gp = max::inputGetKeyState(max::Key::GamepadStart);
			max::inputSetKeyState(max::Key::GamepadStart, NULL, false);

			const bool kb = max::inputGetKeyState(max::Key::Esc);
			max::inputSetKeyState(max::Key::Esc, NULL, false);

			return gp ? 1.0f : kb ? 1.0f : 0.0f;
		} };
	}

	void destroy()
	{

	}

	void update()
	{
		max::inputGetMouse(m_mouse);

		// Global inputs.
		if (max::inputGetAsBool(0, Action::ToggleFullscreen))
		{
			max::toggleFullscreen({ 0 });
		}
		if (max::inputGetAsBool(0, Action::Quit))
		{
			max::destroyWindow({ 0 });
		}
	}

	void enable()
	{
		max::inputAddMappings(0, m_mapping);
	}

	void disable()
	{
		max::inputRemoveMappings(0);
	}

	InputSettings* m_settings;
	max::MouseState* m_mouseState;

	bool m_enable;

	float m_mouse[3];
	max::InputMapping m_mapping[Action::Count];
};

static InputSystem* s_ctx = NULL;

void inputCreate(InputSettings* _settings, max::MouseState* _mouseState)
{
	s_ctx = BX_NEW(max::getAllocator(), InputSystem);
	s_ctx->create(_settings, _mouseState);
}

void inputDestroy()
{
	s_ctx->destroy();
	bx::deleteObject<InputSystem>(max::getAllocator(), s_ctx);
	s_ctx = NULL;
}

void inputUpdate()
{
	s_ctx->update();
}

void inputEnable()
{
	s_ctx->enable();
}

void inputDisable()
{
	s_ctx->disable();
}

