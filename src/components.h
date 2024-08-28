#pragma once

#include <max/max.h>
#include <bx/math.h>

struct TransformComponent
{
	bx::Vec3 m_position;
	bx::Quaternion m_rotation;
	bx::Vec3 m_scale;
};

// @todo Its a little strange to have the path in here but I don't know where else to put it.
// Resource class that holds path and some data. With load() method?
// Should have similar thing for mesh, because right now if you have 4 cubes, it will serialize a cube 4 times. Lots of wasted memory...
struct Material
{
	max::TextureHandle m_color; 
	char m_colorPath[1024];

	max::TextureHandle m_normal;
	char m_normalPath[1024];
};

// @todo For optimzation have support for non dynamic mesh as the final runtime wont need dynamic meshes.
struct RenderComponent
{
	max::MeshHandle m_mesh; 
	Material m_material;
};

struct CameraComponent
{
	CameraComponent(uint32_t _idx, float _fov)
		: m_idx(_idx)
		, m_numSplinePoints(0)
		, m_view()
		, m_proj()
		, m_fov(_fov)
		, m_position({0.0f, 1.0f, -1.0f})
		, m_direction({0.0f, 0.0f, 1.0f})
		, m_up(0.0f, 1.0f, 0.0f)
	{}

	uint32_t m_idx;

	uint32_t m_numSplinePoints;

	float m_view[16];
	float m_proj[16];
	float m_fov;
	bx::Vec3 m_position;
	bx::Vec3 m_direction;
	bx::Vec3 m_up;
};