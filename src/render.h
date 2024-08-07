#pragma once

#include <bx/uint32_t.h>

struct RenderSettings
{
	RenderSettings()
		: m_activeCameraIdx(0)
		, m_width(1280)
		, m_height(720)
	{}

	uint32_t m_activeCameraIdx;
	uint16_t m_width;
	uint16_t m_height;
};

/// For each renderable draw them to the screen using different rendering techniques.
///
void render(RenderSettings* _settings);