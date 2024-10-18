#pragma once

#include <bx/uint32_t.h>
#include <bx/math.h>

/// Render settings.
/// 
struct RenderSettings
{
	uint32_t m_activeCameraIdx; //!< Current active camera that will be used for rendering.

	struct Rect
	{
		uint16_t m_width;
		uint16_t m_height;
	};

	Rect m_resolution; //!< Current resolution.
	Rect m_viewport;   //!< Current viewport.

	// Shadow
	Rect m_shadowMap; //!< Shadowmap resolution.

	// Debug
	enum DebugBuffer
	{
		None,
		Diffuse,
		Normal,
		Surface,
		Depth,

		Irradiance,
		Specular,

		Count
	};
	DebugBuffer m_debugbuffer; //!< Show debug buffer.

	bool m_debugProbes;

	bool m_debugbufferR;
	bool m_debugbufferG;
	bool m_debugbufferB;
};

/// Create render system context.
/// 
/// @param[in] _settings Render settings.
/// 
void renderCreate(RenderSettings* _settings);

/// Destroy render system context.
/// 
void renderDestroy();

/// Update render system. 
///
void renderUpdate();

/// Reset render system and all render techniques.
/// 
void renderReset();

