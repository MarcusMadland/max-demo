#pragma once

#include <bx/uint32_t.h>

struct RenderSettings
{
	uint32_t m_activeCameraIdx; //!< Current active camera that will be used for rendering.
	uint16_t m_width;			//!< Current resolution.
	uint16_t m_height;			//!< Current resolution.
};

void renderCreate(RenderSettings* _settings);

void renderDestroy();

void renderUpdate(RenderSettings* _settings);

