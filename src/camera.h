#pragma once

#include <bx/uint32_t.h>

struct CameraSettings
{
	uint32_t m_activeCameraIdx; //!< Current active camera that will be used for rendering.
	uint16_t m_width;		    //!< Current resolution.
	uint16_t m_height;		    //!< Current resolution.
	float m_near;				//!< Near clip distance.
	float m_far;				//!< Far clip distance.
	float m_moveSpeed;			//!< Move speed of camera.
	float m_lookSpeed;		    //!< Look speed of camera.
};

void cameraCreate(CameraSettings* _settings);

void cameraDestroy();

void cameraUpdate(CameraSettings* _settings);