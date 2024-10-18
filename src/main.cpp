#include <max/max.h>

#include "imgui/imgui.h" // 3rdparty

#include "world.h"
#include "entities.h"
#include "input.h"
#include "components.h"
#include "render.h"
#include "camera.h"

#ifndef TG_CONFIG_WITH_IMGUI
#	define TG_CONFIG_WITH_IMGUI 1
#endif // TG_CONFIG_WITH_IMGUI

#ifndef TG_CONFIG_WITH_MAYA
#	define TG_CONFIG_WITH_MAYA 1
#endif // TG_CONFIG_WITH_MAYA

static const uint32_t kAppWidth = 1280;
static const uint32_t kAppHeight = 720;

/// Callback implementation.
/// 
struct Callback : public max::CallbackI
{
	virtual ~Callback()
	{
	}

	virtual void fatal(const char* _filePath, uint16_t _line, max::Fatal::Enum _code, const char* _str) override
	{
		BX_UNUSED(_filePath, _line);

		bx::debugPrintf("Fatal error: 0x%08x: %s", _code, _str);
		abort();
	}

	virtual void traceVargs(const char* _filePath, uint16_t _line, const char* _format, va_list _argList) override
	{
		bx::debugPrintf("%s (%d): ", _filePath, _line);
		bx::debugPrintfVargs(_format, _argList);

		// Console output
		char format[1024];
		vsprintf(format, _format, _argList);
		printf(format);
	}

	virtual void profilerBegin(const char* /*_name*/, uint32_t /*_abgr*/, const char* /*_filePath*/, uint16_t /*_line*/) override
	{
	}

	virtual void profilerBeginLiteral(const char* /*_name*/, uint32_t /*_abgr*/, const char* /*_filePath*/, uint16_t /*_line*/) override
	{
	}

	virtual void profilerEnd() override
	{
	}

	virtual uint32_t cacheReadSize(uint64_t _id) override
	{
		return 0;
	}

	virtual bool cacheRead(uint64_t _id, void* _data, uint32_t _size) override
	{
		return false;
	}

	virtual void cacheWrite(uint64_t _id, const void* _data, uint32_t _size) override
	{
	}

	virtual void screenShot(const char* _filePath, uint32_t _width, uint32_t _height, uint32_t _pitch, const void* _data, uint32_t /*_size*/, bool _yflip) override
	{
	}

	virtual void captureBegin(uint32_t _width, uint32_t _height, uint32_t /*_pitch*/, max::TextureFormat::Enum /*_format*/, bool _yflip) override
	{
	}

	virtual void captureEnd() override
	{
	}

	virtual void captureFrame(const void* _data, uint32_t /*_size*/) override
	{
	}
};

/// Engine settings.
///
struct Engine
{
	max::MouseState m_mouseState; //!< State of mouse input.
	uint32_t m_width;  //!< Desired width of the viewport/res/window
	uint32_t m_height; //!< Desired height of the viewport/res/window
	uint32_t m_debug;  //!< Debug mode. See `MAX_DEBUG_` flags.
	uint32_t m_reset;  //!< Reset mode. See `MAX_RESET_` flags.
};

static bool processEvents(Engine* _engine)
{
	return max::processEvents(
			_engine->m_width,
			_engine->m_height,
			_engine->m_debug,
			_engine->m_reset,
			&_engine->m_mouseState
		);
}

static void imguiBeginFrame(Engine* _engine)
{
	imguiBeginFrame(_engine->m_mouseState.m_mx
		, _engine->m_mouseState.m_my
		, (_engine->m_mouseState.m_buttons[max::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
		| (_engine->m_mouseState.m_buttons[max::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
		| (_engine->m_mouseState.m_buttons[max::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
		, _engine->m_mouseState.m_mz
		, uint16_t(_engine->m_width)
		, uint16_t(_engine->m_height)
	);
}

/// Application implementation with max.
///
struct TwilightGuardian : public max::AppI
{
	TwilightGuardian(const char* _name)
		: max::AppI(_name)
	{
		// @todo Need config file support for settings. This should be serialized on disk so we don't need
		// to fucking recompile the entire game just to change something here when TG_CONFIG_WITH_EDITOR_TOOLS
		// is false. Will also make it easier for settings menu in the future.

		// Engine.
		m_engine.m_width = kAppWidth;
		m_engine.m_height = kAppHeight;
		m_engine.m_debug = MAX_DEBUG_NONE;
		m_engine.m_reset = MAX_RESET_NONE;

		// Camera settings.
		m_cameraSettings.m_viewport.m_width = kAppWidth;
		m_cameraSettings.m_viewport.m_height = kAppHeight;
		m_cameraSettings.m_near = 0.01f;
		m_cameraSettings.m_far = 1000.0f;
		m_cameraSettings.m_moveSpeed = 15.0f;
		m_cameraSettings.m_lookSpeed = 100.0f;

		// Render settings.
		m_renderSettings.m_resolution.m_height = kAppWidth;
		m_renderSettings.m_resolution.m_width = kAppHeight;
		m_renderSettings.m_viewport.m_height = kAppWidth;
		m_renderSettings.m_viewport.m_height = kAppHeight;
		m_renderSettings.m_activeCameraIdx = 1;
		m_renderSettings.m_debugbuffer = RenderSettings::None;
		m_renderSettings.m_debugProbes = false;
		m_renderSettings.m_shadowMap.m_width = 1024;
		m_renderSettings.m_shadowMap.m_height = 1024;

#if TG_CONFIG_WITH_MAYA
		m_mayaBridge = NULL;
#endif // TG_CONFIG_WITH_MAYA
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		// Initialize engine.
		max::Init init;
		init.rendererType = max::RendererType::Vulkan;
		init.physicsType = max::PhysicsType::Count;
		init.vendorId = MAX_PCI_ID_NONE;
		init.platformData.nwh  = max::getNativeWindowHandle({0});
		init.platformData.type = max::getNativeWindowHandleType();
		init.platformData.ndt  = max::getNativeDisplayHandle();
		init.resolution.width  = kAppWidth;
		init.resolution.height = kAppHeight;
		init.resolution.reset  = m_engine.m_reset;
		init.callback = &m_callback;
		max::init(init);
		
		// Load scenes.
		m_world.load("scenes/scene.bin");
		m_entities.load();

		// Create systems.
		cameraCreate(&m_cameraSettings);
		renderCreate(&m_renderSettings);
		inputCreate(&m_inputSettings, &m_engine.m_mouseState);

		// Enable input.
		inputEnable();

#if TG_CONFIG_WITH_IMGUI
		imguiCreate();
#endif // TG_CONFIG_WITH_IMGUI
	}

	virtual int shutdown() override
	{
#if TG_CONFIG_WITH_IMGUI
		imguiDestroy();
#endif // TG_CONFIG_WITH_IMGUI

		// Destroy systems.
		renderDestroy();
		cameraDestroy();
		inputDestroy();

		// Unload scenes.
		m_world.unload();
		m_entities.unload();

		// Shutdown engine.
		max::shutdown();

		return 0;
	}

	bool update() override
	{
		// Process events.
		if (!processEvents(&m_engine))
		{
#if TG_CONFIG_WITH_IMGUI
			// Imgui.
			imguiBeginFrame(&m_engine);

			ImGui::SetNextWindowPos(ImVec2(10, 10));
			ImGui::SetNextWindowSizeConstraints({ 200, 100 }, { 1920, 1920 });
			ImGui::SetNextWindowCollapsed(true, ImGuiCond_Appearing);
			if (ImGui::Begin("MAX Engine | Dev Menu", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove))
			{
				if (ImGui::CollapsingHeader("Scene"))
				{

				}

				if (ImGui::CollapsingHeader("Camera"))
				{

				}

				if (ImGui::CollapsingHeader("Render"))
				{

				}

#ifdef TG_CONFIG_WITH_MAYA
				static bool s_connectToMaya;
				if (ImGui::Checkbox("Connect to Autodesk Maya", &s_connectToMaya))
				{
					if (s_connectToMaya && m_mayaBridge == NULL)
					{
						// Unload current world and begin syncing with Maya.
						m_world.unload();

						m_mayaBridge = BX_NEW(max::getAllocator(), MayaBridge);
						BX_ASSERT(m_mayaBridge->begin(), "Failed to begin maya bridge, not enough memory.")
					}
					else if (m_mayaBridge != NULL)
					{
						// Save current world and end syncing with Maya. 
						m_world.serialize();

						BX_ASSERT(m_mayaBridge->end(), "Failed to end maya bridge.")
							bx::deleteObject(max::getAllocator(), m_mayaBridge);
						m_mayaBridge = NULL;

						m_world.unload();

						// Load serialized scene from disk.
						m_world.deserialize();
					}
				}
#endif // TG_CONFIG_WITH_MAYA
			}
			ImGui::End();

			imguiEndFrame();

			// Set debug mode.
			max::setDebug(m_engine.m_debug);
			max::dbgTextClear();

#endif  // TG_CONFIG_WITH_IMGUI

#if TG_CONFIG_WITH_MAYA
			// Update maya bridge.
			if (m_mayaBridge != NULL)
			{
				m_mayaBridge->read(&m_world, &m_entities);

				max::dbgTextPrintf(0, 0, 0xf, "Connected to maya...");
			}
#endif  // TG_CONFIG_WITH_MAYA

			// Resize.
			if (m_engine.m_width  != m_renderSettings.m_viewport.m_width ||
				m_engine.m_height != m_renderSettings.m_viewport.m_height)
			{
				m_renderSettings.m_viewport.m_width  = m_engine.m_width;
				m_renderSettings.m_viewport.m_height = m_engine.m_height;
				m_cameraSettings.m_viewport.m_width  = m_engine.m_width;
				m_cameraSettings.m_viewport.m_height = m_engine.m_height;

				renderReset();
			}

			// @todo You gotta decide dude...
			// systemUpdate or system.update(). Private or public data.

			// Update scene.
			m_world.update();
			m_entities.update();  

			// Update systems.
			cameraUpdate();
			renderUpdate();
			inputUpdate();

			return true;
		}

		return false;
	}

	Engine   m_engine;
	Callback m_callback;

	CameraSettings m_cameraSettings;
	RenderSettings m_renderSettings;
	InputSettings  m_inputSettings;

	World m_world;
	Entities m_entities;

#if TG_CONFIG_WITH_MAYA
	MayaBridge* m_mayaBridge;
#endif // TG_CONFIG_WITH_MAYA
};

MAX_IMPLEMENT_MAIN(TwilightGuardian, "TWILIGHT GUARDIAN");