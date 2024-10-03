/*
 * Copyright 2014-2015 Daniel Collin. All rights reserved.
 * License: https://github.com/bkaradzic/max/blob/master/LICENSE
 */

#include <max/embedded_shader.h>
#include <bx/allocator.h>
#include <bx/math.h>
#include <bx/timer.h>
#include <dear-imgui/imgui.h>
#include <dear-imgui/imgui_internal.h>

#include "imgui.h"

#ifndef USE_ENTRY
#	define USE_ENTRY 0
#endif // USE_ENTRY

#if USE_ENTRY
#	include "../entry/entry.h"
#	include "../entry/input.h"
#endif // USE_ENTRY

#include "vs_ocornut_imgui.bin.h"
#include "fs_ocornut_imgui.bin.h"
#include "vs_imgui_image.bin.h"
#include "fs_imgui_image.bin.h"

#include "roboto_regular.ttf.h"
#include "robotomono_regular.ttf.h"
#include "icons_kenney.ttf.h"
#include "icons_font_awesome.ttf.h"

static const max::EmbeddedShader s_embeddedShaders[] =
{
	MAX_EMBEDDED_SHADER(vs_ocornut_imgui),
	MAX_EMBEDDED_SHADER(fs_ocornut_imgui),
	MAX_EMBEDDED_SHADER(vs_imgui_image),
	MAX_EMBEDDED_SHADER(fs_imgui_image),

	MAX_EMBEDDED_SHADER_END()
};

struct FontRangeMerge
{
	const void* data;
	size_t      size;
	ImWchar     ranges[3];
};

static FontRangeMerge s_fontRangeMerge[] =
{
	{ s_iconsKenneyTtf,      sizeof(s_iconsKenneyTtf),      { ICON_MIN_KI, ICON_MAX_KI, 0 } },
	{ s_iconsFontAwesomeTtf, sizeof(s_iconsFontAwesomeTtf), { ICON_MIN_FA, ICON_MAX_FA, 0 } },
};

static void* memAlloc(size_t _size, void* _userData);
static void memFree(void* _ptr, void* _userData);

static bool checkAvailTransientBuffers(uint32_t _numVertices, const max::VertexLayout& _layout, uint32_t _numIndices)
{
	return _numVertices == max::getAvailTransientVertexBuffer(_numVertices, _layout)
		&& (0 == _numIndices || _numIndices == max::getAvailTransientIndexBuffer(_numIndices))
		;
}

struct OcornutImguiContext
{
	void render(ImDrawData* _drawData)
	{
		// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
		int32_t dispWidth  = _drawData->DisplaySize.x * _drawData->FramebufferScale.x;
		int32_t dispHeight = _drawData->DisplaySize.y * _drawData->FramebufferScale.y;
		if (dispWidth  <= 0
		||  dispHeight <= 0)
		{
			return;
		}

		max::setViewName(m_viewId, "ImGui");
		max::setViewMode(m_viewId, max::ViewMode::Sequential);

		const max::Caps* caps = max::getCaps();
		{
			float ortho[16];
			float x = _drawData->DisplayPos.x;
			float y = _drawData->DisplayPos.y;
			float width = _drawData->DisplaySize.x;
			float height = _drawData->DisplaySize.y;

			bx::mtxOrtho(ortho, x, x + width, y + height, y, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
			max::setViewTransform(m_viewId, NULL, ortho);
			max::setViewRect(m_viewId, 0, 0, uint16_t(width), uint16_t(height) );
		}

		const ImVec2 clipPos   = _drawData->DisplayPos;       // (0,0) unless using multi-viewports
		const ImVec2 clipScale = _drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

		// Render command lists
		for (int32_t ii = 0, num = _drawData->CmdListsCount; ii < num; ++ii)
		{
			max::TransientVertexBuffer tvb;
			max::TransientIndexBuffer tib;

			const ImDrawList* drawList = _drawData->CmdLists[ii];
			uint32_t numVertices = (uint32_t)drawList->VtxBuffer.size();
			uint32_t numIndices  = (uint32_t)drawList->IdxBuffer.size();

			if (!checkAvailTransientBuffers(numVertices, m_layout, numIndices) )
			{
				// not enough space in transient buffer just quit drawing the rest...
				break;
			}

			max::allocTransientVertexBuffer(&tvb, numVertices, m_layout);
			max::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

			ImDrawVert* verts = (ImDrawVert*)tvb.data;
			bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert) );

			ImDrawIdx* indices = (ImDrawIdx*)tib.data;
			bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx) );

			max::Encoder* encoder = max::begin();

			for (const ImDrawCmd* cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd)
			{
				if (cmd->UserCallback)
				{
					cmd->UserCallback(drawList, cmd);
				}
				else if (0 != cmd->ElemCount)
				{
					uint64_t state = 0
						| MAX_STATE_WRITE_RGB
						| MAX_STATE_WRITE_A
						| MAX_STATE_MSAA
						;

					max::TextureHandle th = m_texture;
					max::ProgramHandle program = m_program;

					if (NULL != cmd->TextureId)
					{
						union { ImTextureID ptr; struct { max::TextureHandle handle; uint8_t flags; uint8_t mip; } s; } texture = { cmd->TextureId };

						state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
							? MAX_STATE_BLEND_FUNC(MAX_STATE_BLEND_SRC_ALPHA, MAX_STATE_BLEND_INV_SRC_ALPHA)
							: MAX_STATE_NONE
							;
						th = texture.s.handle;

						if (0 != texture.s.mip)
						{
							const float lodEnabled[4] = { float(texture.s.mip), 1.0f, 0.0f, 0.0f };
							max::setUniform(u_imageLodEnabled, lodEnabled);
							program = m_imageProgram;
						}
					}
					else
					{
						state |= MAX_STATE_BLEND_FUNC(MAX_STATE_BLEND_SRC_ALPHA, MAX_STATE_BLEND_INV_SRC_ALPHA);
					}

					// Project scissor/clipping rectangles into framebuffer space
					ImVec4 clipRect;
					clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
					clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
					clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
					clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

					if (clipRect.x <  dispWidth
					&&  clipRect.y <  dispHeight
					&&  clipRect.z >= 0.0f
					&&  clipRect.w >= 0.0f)
					{
						const uint16_t xx = uint16_t(bx::max(clipRect.x, 0.0f) );
						const uint16_t yy = uint16_t(bx::max(clipRect.y, 0.0f) );
						encoder->setScissor(xx, yy
								, uint16_t(bx::min(clipRect.z, 65535.0f)-xx)
								, uint16_t(bx::min(clipRect.w, 65535.0f)-yy)
								);

						encoder->setState(state);
						encoder->setTexture(0, s_tex, th);
						encoder->setVertexBuffer(0, &tvb, cmd->VtxOffset, numVertices);
						encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
						encoder->submit(m_viewId, program);
					}
				}
			}

			max::end(encoder);
		}
	}

	void create(float _fontSize, bx::AllocatorI* _allocator)
	{
		IMGUI_CHECKVERSION();

		m_allocator = _allocator;

		if (NULL == _allocator)
		{
			static bx::DefaultAllocator allocator;
			m_allocator = &allocator;
		}

		m_viewId = 255;
		m_lastScroll = 0;
		m_last = bx::getHPCounter();

		ImGui::SetAllocatorFunctions(memAlloc, memFree, NULL);

		m_imgui = ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();

		io.DisplaySize = ImVec2(1280.0f, 720.0f);
		io.DeltaTime   = 1.0f / 60.0f;
		io.IniFilename = NULL;

		setupStyle(true);

		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

#if USE_ENTRY
		for (int32_t ii = 0; ii < (int32_t)entry::Key::Count; ++ii)
		{
			m_keyMap[ii] = ImGuiKey_None;
		}

		m_keyMap[entry::Key::Esc]          = ImGuiKey_Escape;
		m_keyMap[entry::Key::Return]       = ImGuiKey_Enter;
		m_keyMap[entry::Key::Tab]          = ImGuiKey_Tab;
		m_keyMap[entry::Key::Space]        = ImGuiKey_Space;
		m_keyMap[entry::Key::Backspace]    = ImGuiKey_Backspace;
		m_keyMap[entry::Key::Up]           = ImGuiKey_UpArrow;
		m_keyMap[entry::Key::Down]         = ImGuiKey_DownArrow;
		m_keyMap[entry::Key::Left]         = ImGuiKey_LeftArrow;
		m_keyMap[entry::Key::Right]        = ImGuiKey_RightArrow;
		m_keyMap[entry::Key::Insert]       = ImGuiKey_Insert;
		m_keyMap[entry::Key::Delete]       = ImGuiKey_Delete;
		m_keyMap[entry::Key::Home]         = ImGuiKey_Home;
		m_keyMap[entry::Key::End]          = ImGuiKey_End;
		m_keyMap[entry::Key::PageUp]       = ImGuiKey_PageUp;
		m_keyMap[entry::Key::PageDown]     = ImGuiKey_PageDown;
		m_keyMap[entry::Key::Print]        = ImGuiKey_PrintScreen;
		m_keyMap[entry::Key::Plus]         = ImGuiKey_Equal;
		m_keyMap[entry::Key::Minus]        = ImGuiKey_Minus;
		m_keyMap[entry::Key::LeftBracket]  = ImGuiKey_LeftBracket;
		m_keyMap[entry::Key::RightBracket] = ImGuiKey_RightBracket;
		m_keyMap[entry::Key::Semicolon]    = ImGuiKey_Semicolon;
		m_keyMap[entry::Key::Quote]        = ImGuiKey_Apostrophe;
		m_keyMap[entry::Key::Comma]        = ImGuiKey_Comma;
		m_keyMap[entry::Key::Period]       = ImGuiKey_Period;
		m_keyMap[entry::Key::Slash]        = ImGuiKey_Slash;
		m_keyMap[entry::Key::Backslash]    = ImGuiKey_Backslash;
		m_keyMap[entry::Key::Tilde]        = ImGuiKey_GraveAccent;
		m_keyMap[entry::Key::F1]           = ImGuiKey_F1;
		m_keyMap[entry::Key::F2]           = ImGuiKey_F2;
		m_keyMap[entry::Key::F3]           = ImGuiKey_F3;
		m_keyMap[entry::Key::F4]           = ImGuiKey_F4;
		m_keyMap[entry::Key::F5]           = ImGuiKey_F5;
		m_keyMap[entry::Key::F6]           = ImGuiKey_F6;
		m_keyMap[entry::Key::F7]           = ImGuiKey_F7;
		m_keyMap[entry::Key::F8]           = ImGuiKey_F8;
		m_keyMap[entry::Key::F9]           = ImGuiKey_F9;
		m_keyMap[entry::Key::F10]          = ImGuiKey_F10;
		m_keyMap[entry::Key::F11]          = ImGuiKey_F11;
		m_keyMap[entry::Key::F12]          = ImGuiKey_F12;
		m_keyMap[entry::Key::NumPad0]      = ImGuiKey_Keypad0;
		m_keyMap[entry::Key::NumPad1]      = ImGuiKey_Keypad1;
		m_keyMap[entry::Key::NumPad2]      = ImGuiKey_Keypad2;
		m_keyMap[entry::Key::NumPad3]      = ImGuiKey_Keypad3;
		m_keyMap[entry::Key::NumPad4]      = ImGuiKey_Keypad4;
		m_keyMap[entry::Key::NumPad5]      = ImGuiKey_Keypad5;
		m_keyMap[entry::Key::NumPad6]      = ImGuiKey_Keypad6;
		m_keyMap[entry::Key::NumPad7]      = ImGuiKey_Keypad7;
		m_keyMap[entry::Key::NumPad8]      = ImGuiKey_Keypad8;
		m_keyMap[entry::Key::NumPad9]      = ImGuiKey_Keypad9;
		m_keyMap[entry::Key::Key0]         = ImGuiKey_0;
		m_keyMap[entry::Key::Key1]         = ImGuiKey_1;
		m_keyMap[entry::Key::Key2]         = ImGuiKey_2;
		m_keyMap[entry::Key::Key3]         = ImGuiKey_3;
		m_keyMap[entry::Key::Key4]         = ImGuiKey_4;
		m_keyMap[entry::Key::Key5]         = ImGuiKey_5;
		m_keyMap[entry::Key::Key6]         = ImGuiKey_6;
		m_keyMap[entry::Key::Key7]         = ImGuiKey_7;
		m_keyMap[entry::Key::Key8]         = ImGuiKey_8;
		m_keyMap[entry::Key::Key9]         = ImGuiKey_9;
		m_keyMap[entry::Key::KeyA]         = ImGuiKey_A;
		m_keyMap[entry::Key::KeyB]         = ImGuiKey_B;
		m_keyMap[entry::Key::KeyC]         = ImGuiKey_C;
		m_keyMap[entry::Key::KeyD]         = ImGuiKey_D;
		m_keyMap[entry::Key::KeyE]         = ImGuiKey_E;
		m_keyMap[entry::Key::KeyF]         = ImGuiKey_F;
		m_keyMap[entry::Key::KeyG]         = ImGuiKey_G;
		m_keyMap[entry::Key::KeyH]         = ImGuiKey_H;
		m_keyMap[entry::Key::KeyI]         = ImGuiKey_I;
		m_keyMap[entry::Key::KeyJ]         = ImGuiKey_J;
		m_keyMap[entry::Key::KeyK]         = ImGuiKey_K;
		m_keyMap[entry::Key::KeyL]         = ImGuiKey_L;
		m_keyMap[entry::Key::KeyM]         = ImGuiKey_M;
		m_keyMap[entry::Key::KeyN]         = ImGuiKey_N;
		m_keyMap[entry::Key::KeyO]         = ImGuiKey_O;
		m_keyMap[entry::Key::KeyP]         = ImGuiKey_P;
		m_keyMap[entry::Key::KeyQ]         = ImGuiKey_Q;
		m_keyMap[entry::Key::KeyR]         = ImGuiKey_R;
		m_keyMap[entry::Key::KeyS]         = ImGuiKey_S;
		m_keyMap[entry::Key::KeyT]         = ImGuiKey_T;
		m_keyMap[entry::Key::KeyU]         = ImGuiKey_U;
		m_keyMap[entry::Key::KeyV]         = ImGuiKey_V;
		m_keyMap[entry::Key::KeyW]         = ImGuiKey_W;
		m_keyMap[entry::Key::KeyX]         = ImGuiKey_X;
		m_keyMap[entry::Key::KeyY]         = ImGuiKey_Y;
		m_keyMap[entry::Key::KeyZ]         = ImGuiKey_Z;

		io.ConfigFlags |= 0
			| ImGuiConfigFlags_NavEnableGamepad
			| ImGuiConfigFlags_NavEnableKeyboard
			;

		m_keyMap[entry::Key::GamepadStart]     = ImGuiKey_GamepadStart;
		m_keyMap[entry::Key::GamepadBack]      = ImGuiKey_GamepadBack;
		m_keyMap[entry::Key::GamepadY]         = ImGuiKey_GamepadFaceUp;
		m_keyMap[entry::Key::GamepadA]         = ImGuiKey_GamepadFaceDown;
		m_keyMap[entry::Key::GamepadX]         = ImGuiKey_GamepadFaceLeft;
		m_keyMap[entry::Key::GamepadB]         = ImGuiKey_GamepadFaceRight;
		m_keyMap[entry::Key::GamepadUp]        = ImGuiKey_GamepadDpadUp;
		m_keyMap[entry::Key::GamepadDown]      = ImGuiKey_GamepadDpadDown;
		m_keyMap[entry::Key::GamepadLeft]      = ImGuiKey_GamepadDpadLeft;
		m_keyMap[entry::Key::GamepadRight]     = ImGuiKey_GamepadDpadRight;
		m_keyMap[entry::Key::GamepadShoulderL] = ImGuiKey_GamepadL1;
		m_keyMap[entry::Key::GamepadShoulderR] = ImGuiKey_GamepadR1;
		m_keyMap[entry::Key::GamepadThumbL]    = ImGuiKey_GamepadL3;
		m_keyMap[entry::Key::GamepadThumbR]    = ImGuiKey_GamepadR3;
#endif // USE_ENTRY

		max::RendererType::Enum type = max::getRendererType();
		m_program = max::createProgram(
			  max::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui")
			, max::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui")
			, true
			);

		u_imageLodEnabled = max::createUniform("u_imageLodEnabled", max::UniformType::Vec4);
		m_imageProgram = max::createProgram(
			  max::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image")
			, max::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image")
			, true
			);

		m_layout
			.begin()
			.add(max::Attrib::Position,  2, max::AttribType::Float)
			.add(max::Attrib::TexCoord0, 2, max::AttribType::Float)
			.add(max::Attrib::Color0,    4, max::AttribType::Uint8, true)
			.end();

		s_tex = max::createUniform("s_tex", max::UniformType::Sampler);

		uint8_t* data;
		int32_t width;
		int32_t height;
		{
			ImFontConfig config;
			config.FontDataOwnedByAtlas = false;
			config.MergeMode = false;
//			config.MergeGlyphCenterV = true;

			const ImWchar* ranges = io.Fonts->GetGlyphRangesCyrillic();
			m_font[ImGui::Font::Regular] = io.Fonts->AddFontFromMemoryTTF( (void*)s_robotoRegularTtf,     sizeof(s_robotoRegularTtf),     _fontSize,      &config, ranges);
			m_font[ImGui::Font::Mono   ] = io.Fonts->AddFontFromMemoryTTF( (void*)s_robotoMonoRegularTtf, sizeof(s_robotoMonoRegularTtf), _fontSize-3.0f, &config, ranges);

			config.MergeMode = true;
			config.DstFont   = m_font[ImGui::Font::Regular];

			for (uint32_t ii = 0; ii < BX_COUNTOF(s_fontRangeMerge); ++ii)
			{
				const FontRangeMerge& frm = s_fontRangeMerge[ii];

				io.Fonts->AddFontFromMemoryTTF( (void*)frm.data
						, (int)frm.size
						, _fontSize-3.0f
						, &config
						, frm.ranges
						);
			}
		}

		io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);

		m_texture = max::createTexture2D(
			  (uint16_t)width
			, (uint16_t)height
			, false
			, 1
			, max::TextureFormat::BGRA8
			, 0
			, max::copy(data, width*height*4)
			);
	}

	void destroy()
	{
		ImGui::DestroyContext(m_imgui);

		max::destroy(s_tex);
		max::destroy(m_texture);

		max::destroy(u_imageLodEnabled);
		max::destroy(m_imageProgram);
		max::destroy(m_program);

		m_allocator = NULL;
	}

	void setupStyle(bool _dark)
	{
		ImGuiStyle& style = ImGui::GetStyle();

		style.WindowRounding = 0.0f;
		style.ChildRounding = 0.0f;
		style.FrameRounding = 0.0f;
		style.GrabRounding = 0.0f;
		style.PopupRounding = 0.0f;
		style.ScrollbarRounding = 0.0f;
		style.TabRounding = 0.0f;

		constexpr auto ColorFromBytes = [](uint8_t _r, uint8_t _g, uint8_t _b)
		{
			return ImVec4((float)_r / 255.0f, (float)_g / 255.0f, (float)_b / 255.0f, 1.0f);
		};

		ImVec4* colors = style.Colors;
		const ImVec4 bgColor		   = ColorFromBytes(37, 37, 38);
		const ImVec4 lightBgColor	   = ColorFromBytes(82, 82, 85);
		const ImVec4 veryLightBgColor  = ColorFromBytes(90, 90, 95);
		const ImVec4 panelColor		   = ColorFromBytes(51, 51, 55);
		const ImVec4 panelHoverColor   = ColorFromBytes(29, 151, 236);
		const ImVec4 panelActiveColor  = ColorFromBytes(0, 119, 200);
		const ImVec4 textColor         = ColorFromBytes(255, 255, 255);
		const ImVec4 textDisabledColor = ColorFromBytes(151, 151, 151);
		const ImVec4 borderColor	   = ColorFromBytes(78, 78, 78);
		
		colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.95f);
		colors[ImGuiCol_Text] = textColor;
		colors[ImGuiCol_TextDisabled] = textDisabledColor;
		colors[ImGuiCol_TextSelectedBg] = panelActiveColor;
		colors[ImGuiCol_ChildBg] = bgColor;
		colors[ImGuiCol_PopupBg] = bgColor;
		colors[ImGuiCol_Border] = borderColor;
		colors[ImGuiCol_BorderShadow] = borderColor;
		colors[ImGuiCol_FrameBg] = panelColor;
		colors[ImGuiCol_FrameBgHovered] = panelHoverColor;
		colors[ImGuiCol_FrameBgActive] = panelActiveColor;
		colors[ImGuiCol_TitleBg] = bgColor;
		colors[ImGuiCol_TitleBgActive] = bgColor;
		colors[ImGuiCol_TitleBgCollapsed] = bgColor;
		colors[ImGuiCol_MenuBarBg] = panelColor;
		colors[ImGuiCol_ScrollbarBg] = panelColor;
		colors[ImGuiCol_ScrollbarGrab] = lightBgColor;
		colors[ImGuiCol_ScrollbarGrabHovered] = veryLightBgColor;
		colors[ImGuiCol_ScrollbarGrabActive] = veryLightBgColor;
		colors[ImGuiCol_CheckMark] = panelActiveColor;
		colors[ImGuiCol_SliderGrab] = panelHoverColor;
		colors[ImGuiCol_SliderGrabActive] = panelActiveColor;
		colors[ImGuiCol_Button] = panelColor;
		colors[ImGuiCol_ButtonHovered] = panelHoverColor;
		colors[ImGuiCol_ButtonActive] = panelHoverColor;
		colors[ImGuiCol_Header] = panelColor;
		colors[ImGuiCol_HeaderHovered] = panelHoverColor;
		colors[ImGuiCol_HeaderActive] = panelActiveColor;
		colors[ImGuiCol_Separator] = borderColor;
		colors[ImGuiCol_SeparatorHovered] = borderColor;
		colors[ImGuiCol_SeparatorActive] = borderColor;
		colors[ImGuiCol_ResizeGrip] = bgColor;
		colors[ImGuiCol_ResizeGripHovered] = panelColor;
		colors[ImGuiCol_ResizeGripActive] = lightBgColor;
		colors[ImGuiCol_PlotLines] = panelActiveColor;
		colors[ImGuiCol_PlotLinesHovered] = panelHoverColor;
		colors[ImGuiCol_PlotHistogram] = panelActiveColor;
		colors[ImGuiCol_PlotHistogramHovered] = panelHoverColor;
		colors[ImGuiCol_DragDropTarget] = bgColor;
		colors[ImGuiCol_NavHighlight] = bgColor;
		colors[ImGuiCol_Tab] = bgColor;
		colors[ImGuiCol_TabActive] = panelActiveColor;
		colors[ImGuiCol_TabUnfocused] = bgColor;
		colors[ImGuiCol_TabUnfocusedActive] = panelActiveColor;
		colors[ImGuiCol_TabHovered] = panelHoverColor;

		// Doug Binks' darl color scheme
		// https://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9
		//ImGuiStyle& style = ImGui::GetStyle();
		//if (_dark)
		//{
		//	ImGui::StyleColorsDark(&style);
		//}
		//else
		//{
		//	ImGui::StyleColorsLight(&style);
		//}
		//
		//style.FrameRounding    = 4.0f;
		//style.WindowBorderSize = 0.0f;
	}

	void beginFrame(
		  int32_t _mx
		, int32_t _my
		, uint8_t _button
		, int32_t _scroll
		, int _width
		, int _height
		, int _inputChar
		, max::ViewId _viewId
		)
	{
		m_viewId = _viewId;

		ImGuiIO& io = ImGui::GetIO();
		if (_inputChar >= 0)
		{
			io.AddInputCharacter(_inputChar);
		}

		io.DisplaySize = ImVec2( (float)_width, (float)_height);

		const int64_t now = bx::getHPCounter();
		const int64_t frameTime = now - m_last;
		m_last = now;
		const double freq = double(bx::getHPFrequency() );
		io.DeltaTime = float(frameTime/freq);

		io.AddMousePosEvent( (float)_mx, (float)_my);
		io.AddMouseButtonEvent(ImGuiMouseButton_Left,   0 != (_button & IMGUI_MBUT_LEFT  ) );
		io.AddMouseButtonEvent(ImGuiMouseButton_Right,  0 != (_button & IMGUI_MBUT_RIGHT ) );
		io.AddMouseButtonEvent(ImGuiMouseButton_Middle, 0 != (_button & IMGUI_MBUT_MIDDLE) );
		io.AddMouseWheelEvent(0.0f, (float)(_scroll - m_lastScroll) );
		m_lastScroll = _scroll;

#if USE_ENTRY
		uint8_t modifiers = inputGetModifiersState();
		io.AddKeyEvent(ImGuiMod_Shift, 0 != (modifiers & (entry::Modifier::LeftShift | entry::Modifier::RightShift) ) );
		io.AddKeyEvent(ImGuiMod_Ctrl,  0 != (modifiers & (entry::Modifier::LeftCtrl  | entry::Modifier::RightCtrl ) ) );
		io.AddKeyEvent(ImGuiMod_Alt,   0 != (modifiers & (entry::Modifier::LeftAlt   | entry::Modifier::RightAlt  ) ) );
		io.AddKeyEvent(ImGuiMod_Super, 0 != (modifiers & (entry::Modifier::LeftMeta  | entry::Modifier::RightMeta ) ) );
		for (int32_t ii = 0; ii < (int32_t)entry::Key::Count; ++ii)
		{
			io.AddKeyEvent(m_keyMap[ii], inputGetKeyState(entry::Key::Enum(ii) ) );
			io.SetKeyEventNativeData(m_keyMap[ii], 0, 0, ii);
		}
#endif // USE_ENTRY

		ImGui::NewFrame();
	}

	void endFrame()
	{
		ImGui::Render();
		render(ImGui::GetDrawData() );
	}

	ImGuiContext*       m_imgui;
	bx::AllocatorI*     m_allocator;
	max::VertexLayout  m_layout;
	max::ProgramHandle m_program;
	max::ProgramHandle m_imageProgram;
	max::TextureHandle m_texture;
	max::UniformHandle s_tex;
	max::UniformHandle u_imageLodEnabled;
	ImFont* m_font[ImGui::Font::Count];
	int64_t m_last;
	int32_t m_lastScroll;
	max::ViewId m_viewId;
#if USE_ENTRY
	ImGuiKey m_keyMap[(int)entry::Key::Count];
#endif // USE_ENTRY
};

static OcornutImguiContext s_ctx;

static void* memAlloc(size_t _size, void* _userData)
{
	BX_UNUSED(_userData);
	return bx::alloc(s_ctx.m_allocator, _size);
}

static void memFree(void* _ptr, void* _userData)
{
	BX_UNUSED(_userData);
	bx::free(s_ctx.m_allocator, _ptr);
}

void imguiCreate(float _fontSize, bx::AllocatorI* _allocator)
{
	s_ctx.create(_fontSize, _allocator);
}

void imguiDestroy()
{
	s_ctx.destroy();
}

void imguiBeginFrame(int32_t _mx, int32_t _my, uint8_t _button, int32_t _scroll, uint16_t _width, uint16_t _height, int _inputChar, max::ViewId _viewId)
{
	s_ctx.beginFrame(_mx, _my, _button, _scroll, _width, _height, _inputChar, _viewId);
}

void imguiEndFrame()
{
	s_ctx.endFrame();
}

namespace ImGui
{
	void PushFont(Font::Enum _font)
	{
		PushFont(s_ctx.m_font[_font]);
	}

	void PushEnabled(bool _enabled)
	{
		extern void PushItemFlag(int option, bool enabled);
		PushItemFlag(ImGuiItemFlags_Disabled, !_enabled);
		PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * (_enabled ? 1.0f : 0.5f) );
	}

	void PopEnabled()
	{
		extern void PopItemFlag();
		PopItemFlag();
		PopStyleVar();
	}

} // namespace ImGui

BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4505); // error C4505: '' : unreferenced local function has been removed
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wunused-function"); // warning: 'int rect_width_compare(const void*, const void*)' defined but not used
BX_PRAGMA_DIAGNOSTIC_PUSH();
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG("-Wunknown-pragmas")
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wtype-limits"); // warning: comparison is always true due to limited range of data type

#define STBTT_ifloor(_a)   int32_t(bx::floor(_a) )
#define STBTT_iceil(_a)    int32_t(bx::ceil(_a) )
#define STBTT_sqrt(_a)     bx::sqrt(_a)
#define STBTT_pow(_a, _b)  bx::pow(_a, _b)
#define STBTT_fmod(_a, _b) bx::mod(_a, _b)
#define STBTT_cos(_a)      bx::cos(_a)
#define STBTT_acos(_a)     bx::acos(_a)
#define STBTT_fabs(_a)     bx::abs(_a)
#define STBTT_strlen(_str) bx::strLen(_str)

#define STBTT_memcpy(_dst, _src, _numBytes) bx::memCopy(_dst, _src, _numBytes)
#define STBTT_memset(_dst, _ch, _numBytes)  bx::memSet(_dst, _ch, _numBytes)
#define STBTT_malloc(_size, _userData)      memAlloc(_size, _userData)
#define STBTT_free(_ptr, _userData)         memFree(_ptr, _userData)

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>
BX_PRAGMA_DIAGNOSTIC_POP();
