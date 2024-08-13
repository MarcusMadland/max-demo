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

	struct Material
	{
		bx::Vec3 m_color;

	} m_material;
};

struct DynamicRenderComponent
{
	max::DynamicMeshHandle m_mesh;

	struct Material
	{
		bx::Vec3 m_color;

	} m_material;
};

struct CameraComponent
{
	CameraComponent(uint32_t _idx, float _fov)
		: m_idx(_idx)
		, m_fov(_fov)
		, m_numSplinePoints(0)
		, m_splinePoints(0)
		, m_view()
		, m_proj()
	{}

	uint32_t m_idx;
	float m_fov;

	uint32_t m_numSplinePoints;
	uint32_t m_splinePoints;

	float m_view[16];
	float m_proj[16];
};