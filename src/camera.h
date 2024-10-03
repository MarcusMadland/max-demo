#pragma once

#include <bx/uint32_t.h>

struct CameraSettings
{
	struct Rect
	{
		uint16_t m_width;
		uint16_t m_height;
	};
	Rect m_viewport;			//!< Current viewport.
	
	float m_near;				//!< Near clip distance.
	float m_far;				//!< Far clip distance.
	float m_moveSpeed;			//!< Move speed of camera.
	float m_lookSpeed;		    //!< Look speed of camera.
};

void cameraCreate(CameraSettings* _settings);

void cameraDestroy();

void cameraUpdate();