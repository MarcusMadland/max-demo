#include <max/max.h>

#include "imgui/imgui.h" // 3rdparty

#include "world.h"
#include "entities.h"
#include "input.h"
#include "components.h"
#include "render.h"
#include "camera.h"

#ifndef TG_CONFIG_WITH_EDITOR_TOOLS
#	define TG_CONFIG_WITH_EDITOR_TOOLS 1
#endif // TG_CONFIG_WITH_EDITOR_TOOLS

struct EngineSettings
{
	bool m_vsync;

	bool m_debugGrid;
	bool m_debugText;
	bool m_debugStats;
};

struct TwilightGuardian : public max::AppI
{
	TwilightGuardian(const char* _name)
		: max::AppI(_name)
	{
		// Default.
		m_width = 1280;
		m_height = 720;
		m_debug = MAX_DEBUG_NONE;
		m_reset = MAX_RESET_NONE;

		// @todo Need config file support for settings. This should be serialized on disk so we don't need
		// to fucking recompile the entire game just to change something here when TG_CONFIG_WITH_EDITOR_TOOLS
		// is false. Will also make it easier for settings menu in the future.

		// Engine settings.
		m_engineSettings.m_debugText = false;
		m_engineSettings.m_debugStats = false;
		m_engineSettings.m_debugGrid = false;

		// Camera settings.
		m_cameraSettings.m_viewport.m_width = m_width;
		m_cameraSettings.m_viewport.m_height = m_height;
		m_cameraSettings.m_near = 0.01f;
		m_cameraSettings.m_far = 1000.0f;
		m_cameraSettings.m_moveSpeed = 15.0f;
		m_cameraSettings.m_lookSpeed = 100.0f;

		// Render settings.
		m_renderSettings.m_activeCameraIdx = 1;
		m_renderSettings.m_resolution.m_height = m_width;
		m_renderSettings.m_resolution.m_width = m_height;
		m_renderSettings.m_viewport.m_height = m_width;
		m_renderSettings.m_viewport.m_height = m_height;
		m_renderSettings.m_skybox = "textures/bolonga_lod.dds";
		m_renderSettings.m_radiance = "textures/bolonga_lod.dds";
		m_renderSettings.m_irradiance = "textures/bolonga_irr.dds";
		m_renderSettings.m_debugbuffer = RenderSettings::None;
		m_renderSettings.m_sunDir = { 0.0f, -1.0f, 0.0f };
		m_renderSettings.m_sunCol = { 1.0f, 1.0f, 1.0f };
		m_renderSettings.m_probe = { 1.0f, 1.0f, 1.0f };
		m_renderSettings.m_shadowMap.m_width = 1024;
		m_renderSettings.m_shadowMap.m_height = 1024;

#if TG_CONFIG_WITH_EDITOR_TOOLS
		m_mayaBridge = NULL;
#endif // TG_CONFIG_WITH_EDITOR_TOOLS
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
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
		
		// Load scenes.
		m_world.load("scenes/scene.bin");
		m_entities.load();

		// Enable input.
		m_input.enable();

		// Create systems.
		cameraCreate(&m_cameraSettings);
		renderCreate(&m_renderSettings);

#if TG_CONFIG_WITH_EDITOR_TOOLS
		imguiCreate();
#endif // TG_CONFIG_WITH_EDITOR_TOOLS

	}

	virtual int shutdown() override
	{

#if TG_CONFIG_WITH_EDITOR_TOOLS
		imguiDestroy();
#endif // TG_CONFIG_WITH_EDITOR_TOOLS

		// Destroy systems.
		renderDestroy();
		cameraDestroy();

		// Disable input.
		m_input.disable();

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
		if (!max::processEvents(m_width, m_height, m_debug, m_reset, &m_input.m_mouseState) )
		{
#if TG_CONFIG_WITH_EDITOR_TOOLS
			// Imgui.
			imguiBeginFrame(m_input.m_mouseState.m_mx
				, m_input.m_mouseState.m_my
				, (m_input.m_mouseState.m_buttons[max::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
				| (m_input.m_mouseState.m_buttons[max::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
				| (m_input.m_mouseState.m_buttons[max::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				, m_input.m_mouseState.m_mz
				, uint16_t(m_width)
				, uint16_t(m_height)
			);

			struct Resolution
			{
				const char* m_label;
				int m_width;
				int m_height;
			};

			const Resolution s_resolutions[] =
			{
				{ "1920x1080", 1920, 1080 },
				{ "1280x720", 1280, 720 },
				{ "720x480", 720, 480 },
				{ "480x360", 480, 360 },
			};

			const Resolution s_shadowmap[] =
			{
				{ "4096x4096", 4096, 4096 },
				{ "2048x2048", 2048, 2048 },
				{ "1024x1024", 1024, 1024 },
				{ "512x512", 512, 512 },
			};

			struct DebugBuffer
			{
				const char* m_label;
				RenderSettings::DebugBuffer m_enum;
			};

			const DebugBuffer s_debugbuffers[] =
			{
				{ "None", RenderSettings::DebugBuffer::None },
				{ "Diffuse", RenderSettings::DebugBuffer::Diffuse },
				{ "Normal", RenderSettings::DebugBuffer::Normal },
				{ "Surface", RenderSettings::DebugBuffer::Surface },
				{ "Depth", RenderSettings::DebugBuffer::Depth },
				{ "Irradiance", RenderSettings::DebugBuffer::Irradiance },
				{ "Specular", RenderSettings::DebugBuffer::Specular },
			};

			ImGui::SetNextWindowPos(ImVec2(10, 10));
			ImGui::SetNextWindowSizeConstraints({ 200, 100 }, { 1920, 1920 });
			ImGui::SetNextWindowCollapsed(true, ImGuiCond_Appearing);
			if (ImGui::Begin("MAX Engine | Dev Menu", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove))
			{
				if (ImGui::CollapsingHeader("Engine Settings", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::SeparatorText("Graphics API");

					ImGui::Checkbox("VSync", &m_engineSettings.m_vsync);

					uint32_t reset = m_reset;
					m_reset = (m_engineSettings.m_vsync ? (m_reset | MAX_RESET_VSYNC) : (m_reset & ~MAX_RESET_VSYNC));
					if (m_reset != reset)
					{
						max::reset(m_width, m_height, m_reset);
					}

					ImGui::SeparatorText("Debug");

					ImGui::Checkbox("Debug Text", &m_engineSettings.m_debugText);
					ImGui::Checkbox("Debug Grid", &m_engineSettings.m_debugGrid);
					ImGui::Checkbox("Debug Stats", &m_engineSettings.m_debugStats);

					m_debug = (m_engineSettings.m_debugText ? (m_debug | MAX_DEBUG_TEXT) : (m_debug & ~MAX_DEBUG_TEXT));
					m_debug = (m_engineSettings.m_debugStats ? (m_debug | MAX_DEBUG_STATS) : (m_debug & ~MAX_DEBUG_STATS));
				}

				if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::SeparatorText("Renderer");

					ImGui::SliderInt("Active Render Camera", (int*)&m_renderSettings.m_activeCameraIdx, 0, 1);

					static int currentResolutionIndex = 0;
					if (ImGui::BeginCombo("Display Resolution", s_resolutions[currentResolutionIndex].m_label)) 
					{
						for (int i = 0; i < IM_ARRAYSIZE(s_resolutions); i++)
						{
							bool isSelected = (currentResolutionIndex == i);
							if (ImGui::Selectable(s_resolutions[i].m_label, isSelected))
							{
								currentResolutionIndex = i;

								m_renderSettings.m_resolution.m_width = s_resolutions[i].m_width;
								m_renderSettings.m_resolution.m_height = s_resolutions[i].m_height;

								renderReset();
							}
							if (isSelected) 
							{
								ImGui::SetItemDefaultFocus(); 
							}
						}
						ImGui::EndCombo();
					}

					ImGui::SeparatorText("Shadows");

					static int currentShadowmapIndex = 0;
					if (ImGui::BeginCombo("Shadow Resolution", s_shadowmap[currentShadowmapIndex].m_label))
					{
						for (int i = 0; i < IM_ARRAYSIZE(s_shadowmap); i++)
						{
							bool isSelected = (currentShadowmapIndex == i);
							if (ImGui::Selectable(s_shadowmap[i].m_label, isSelected))
							{
								currentShadowmapIndex = i;

								m_renderSettings.m_shadowMap.m_width = s_shadowmap[i].m_width;
								m_renderSettings.m_shadowMap.m_height = s_shadowmap[i].m_height;

								renderReset();
							}
							if (isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

					ImGui::SeparatorText("Debug");

					static int currentDebugbufferIndex = 0;
					if (ImGui::BeginCombo("Debug Buffer", s_debugbuffers[currentDebugbufferIndex].m_label))
					{
						for (int i = 0; i < IM_ARRAYSIZE(s_debugbuffers); i++)
						{
							bool isSelected = (currentDebugbufferIndex == i);
							if (ImGui::Selectable(s_debugbuffers[i].m_label, isSelected))
							{
								currentDebugbufferIndex = i;

								m_renderSettings.m_debugbuffer = s_debugbuffers[i].m_enum;
							}
							if (isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
				}

				if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::SeparatorText("Camera");

					ImGui::SliderFloat("Near", &m_cameraSettings.m_near, 0.0f, 1.0f);
					ImGui::SliderFloat("Far", &m_cameraSettings.m_far, 1.0f, 10000.0f);

					ImGui::SeparatorText("Controller");

					ImGui::SliderFloat("Sensitivity", &m_cameraSettings.m_lookSpeed, 0.1f, 200.0f);
					ImGui::SliderFloat("Speed", &m_cameraSettings.m_moveSpeed, 0.1f, 50.0f);
				}

				if (ImGui::CollapsingHeader("Other", ImGuiTreeNodeFlags_DefaultOpen))
				{
					static bool s_connectToMaya;
					if (ImGui::Checkbox("Edit in Autodesk Maya", &s_connectToMaya))
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
				}
			}
			ImGui::End();

			imguiEndFrame();

			// Set debug mode.
			max::setDebug(m_debug);
			max::dbgTextClear();
			if (m_engineSettings.m_debugGrid)
			{
				max::dbgDrawBegin(1); // Draw in gbuffer view.
				max::dbgDrawGrid(max::Axis::Y, { 0.0f, 0.0f, 0.0f });
				//max::dbgDrawAxis(0.0f, -2.0f, 0.0f, 1.0f, max::Axis::X, 2.0f);
				max::dbgDrawEnd();
			}

			if (true)
			{
				uint32_t idx = 1;

				max::dbgTextPrintf(0, ++idx, 0xf, "World:");
				for (auto it = m_world.m_entities.begin(); it != m_world.m_entities.end(); ++it)
				{
					max::dbgTextPrintf(0, ++idx, 0xf, "[%u]: %s", idx, it->first.c_str());
				}

				++idx;

				max::dbgTextPrintf(0, ++idx, 0xf, "Entities:");
				for (auto it = m_entities.m_entities.begin(); it != m_entities.m_entities.end(); ++it)
				{
					max::dbgTextPrintf(0, ++idx, 0xf, "[%u]: %s", idx, it->first.c_str());
				}
			}

			// Update maya bridge.
			if (m_mayaBridge != NULL)
			{
				m_mayaBridge->read(&m_world, &m_entities);

				max::dbgTextPrintf(0, 0, 0xf, "Connected to maya...");
			}
#endif  // TG_CONFIG_WITH_EDITOR_TOOLS

			// Resize.
			if (m_width  != m_renderSettings.m_viewport.m_width ||
				m_height != m_renderSettings.m_viewport.m_height)
			{
				m_renderSettings.m_viewport.m_width = m_width;
				m_renderSettings.m_viewport.m_height = m_height;
				
				renderReset();

				m_cameraSettings.m_viewport.m_width = m_width;
				m_cameraSettings.m_viewport.m_height = m_height;
			}

			// Update scene.
			m_world.update();
			m_entities.update();

			// Update systems.
			cameraUpdate();
			renderUpdate();

			// Update input.
			m_input.update();

			// Some global input stuff @todo Dude seriously put this fucking elsewhere...
			if (max::inputGetAsBool(0, Action::ToggleFullscreen))
			{
				max::toggleFullscreen({ 0 });
			}
			if (max::inputGetAsBool(0, Action::Quit))
			{
				max::destroyWindow({ 0 });
			}

			return true;
		}

		return false;
	}

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;

	EngineSettings m_engineSettings;
	CameraSettings m_cameraSettings;
	RenderSettings m_renderSettings;

	World m_world;
	Entities m_entities;
	Input m_input;

#if TG_CONFIG_WITH_EDITOR_TOOLS
	MayaBridge* m_mayaBridge;
#endif // TG_CONFIG_WITH_EDITOR_TOOLS
};

MAX_IMPLEMENT_MAIN(TwilightGuardian, "TWILIGHT GUARDIAN");