#include <max/max.h>

#include "scene.h"
#include "input.h"
#include "components.h"
#include "render.h"
#include "camera.h"

constexpr uint32_t kDebugCameraIdx = 0;
constexpr uint32_t kDefaultCameraIdx = 1;

class TwilightGuardian : public max::AppI
{
public:
	TwilightGuardian(const char* _name)
		: max::AppI(_name)
	{}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		m_width  = _width;
		m_height = _height;
		m_debug  = MAX_DEBUG_TEXT;
		m_reset  = MAX_RESET_VSYNC;

		// Initialize engine.
		max::Init init;
		init.rendererType = max::RendererType::OpenGL;
		init.physicsType = max::PhysicsType::Count;
		init.vendorId = MAX_PCI_ID_NONE;
		init.platformData.nwh  = max::getNativeWindowHandle({0});
		init.platformData.type = max::getNativeWindowHandleType();
		init.platformData.ndt  = max::getNativeDisplayHandle();
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		max::init(init);

		// Load scene.
		m_scene.load();

		// Enable input.
		m_input.enable();

		// System settings.
		m_cameraSettings.m_activeCameraIdx = kDefaultCameraIdx;
		m_cameraSettings.m_width = m_width;
		m_cameraSettings.m_height = m_height;
		m_cameraSettings.m_near = 0.01f;
		m_cameraSettings.m_far = 1000.0f;
		m_cameraSettings.m_moveSpeed = 2.0f;
		m_cameraSettings.m_lookSpeed = 100.0f;

		m_renderSettings.m_activeCameraIdx = kDefaultCameraIdx;
		m_renderSettings.m_width = m_width;
		m_renderSettings.m_height = m_height;

		// Systems.
		cameraCreate(&m_cameraSettings);
		renderCreate(&m_renderSettings);
	}

	virtual int shutdown() override
	{
		// Systems.
		renderDestroy();
		cameraDestroy();

		// Disable input.
		m_input.disable();

		// Unload scene.
		m_scene.unload();

		// Shutdown engine.
		max::shutdown();

		return 0;
	}

	bool update() override
	{
		max::MouseState mouseState;

		// Process events.
		if (!max::processEvents(m_width, m_height, m_debug, m_reset, &mouseState) )
		{
			// Set debug mode.
			max::setDebug(m_debug);
			// Debug drawing.  (@todo Put elsewhere?)
			max::dbgTextClear();

			max::dbgDrawBegin(0);
			max::dbgDrawGrid(max::Axis::Y, { 0.0f, 0.0f, 0.0f });
			//max::dbgDrawAxis(5.0f, 0.0f, 5.0f, 1.0f, max::Axis::Count, 0.01f);
			max::dbgDrawEnd();

			// Update input.
			m_input.update();

			// Update scene.
			m_scene.update();

			// Update settings.
			m_cameraSettings.m_width  = m_width;
			m_cameraSettings.m_height = m_height;
			m_renderSettings.m_width  = m_width;
			m_renderSettings.m_height = m_height;

			// Some global input stuff (@todo Put this elsewhere?)
			if (max::inputGetAsBool(0, Action::ToggleMayaBridge))
			{
				if (m_scene.m_mayaBridge == NULL)
				{
					m_scene.unload();
					m_scene.beginMayaBridge();
					m_renderSettings.m_activeCameraIdx = kDebugCameraIdx;
				}
				else
				{
					m_renderSettings.m_activeCameraIdx = kDefaultCameraIdx;
					m_scene.endMayaBridge();
					m_scene.unload();

					m_scene.load();
				}
			}

			if (max::inputGetAsBool(0, Action::Quit))
			{
				max::destroyWindow({ 0 });
			}

			// Game Systems.
			cameraUpdate(&m_cameraSettings);
			renderUpdate(&m_renderSettings);

			return true;
		}

		return false;
	}

	Scene m_scene;
	Input m_input;

	CameraSettings m_cameraSettings;
	RenderSettings m_renderSettings;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
};

MAX_IMPLEMENT_MAIN(TwilightGuardian, "Twilight Guardian");
