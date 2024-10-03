#pragma once

#include <bx/uint32_t.h>

#include <bx/math.h>

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

	Rect m_shadowMap; //!< Shadowmap resolution.

	const char* m_skybox;     //!< @todo	
	const char* m_irradiance; //!< Irradiance map used for image based lighting.
	const char* m_radiance; //!< Radiance map used for image based lighting.
	bx::Vec3 m_sunDir = { 0.0f, -1.0f, 0.0f }; //!< Directional light direction.
	bx::Vec3 m_sunCol = { 1.0f, 1.0f, 1.0f };  //!< Directional light color.

	bx::Vec3 m_probe = { 0.0f, 0.0f, 0.0f };

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

	bool m_debugbufferR;
	bool m_debugbufferG;
	bool m_debugbufferB;
};

void renderCreate(RenderSettings* _settings);

void renderDestroy();

void renderUpdate();

void renderReset();

