#pragma once

#include <max/max.h>
#include <bx/math.h>

struct TransformComponent
{
	bx::Vec3 m_position;
	bx::Quaternion m_rotation;
	bx::Vec3 m_scale;
};

struct RenderComponent
{
	max::MeshHandle m_mesh;
	max::MaterialHandle m_material;
};

struct CameraComponent
{
	CameraComponent(uint32_t _idx, float _fov)
		: m_idx(_idx)
		, m_fov(_fov)
		, m_numAnimations(0)
		, m_view()
		, m_proj()
	{}

	uint32_t m_idx;
	float m_fov;

	uint32_t m_numAnimations;

	float m_view[16];
	float m_proj[16];
};