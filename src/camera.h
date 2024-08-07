#pragma once

#include <bx/uint32_t.h>

struct CameraSettings
{
	uint32_t m_activeCameraIdx;
	uint16_t m_width;
	uint16_t m_height;
	float m_near;
	float m_far;
};

/// For each camera calculate view and projection matrices.
///
void camera(CameraSettings* _settings);