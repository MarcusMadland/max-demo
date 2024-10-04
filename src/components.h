#pragma once

#include <max/max.h>
#include <bx/math.h>

struct TransformComponent
{
	bx::Vec3 m_position;
	bx::Quaternion m_rotation;
	bx::Vec3 m_scale;
};

struct MaterialComponent
{
	struct Texture
	{
		max::TextureHandle m_texture; //!< Handle to texture.
		char m_filepath[1024];	      //!< Path to texture.
	};

	Texture m_diffuse;	      //!< Diffuse map.
	float m_diffuseFactor[3]; //!< Diffuse factor.

	Texture m_normal;         //!< Normal map.
	float m_normalFactor[3];  //!< Normal factor.

	Texture m_roughness;      //!< Rougness map.
	float m_roughnessFactor;  //!< Rougness factor.

	Texture m_metallic;       //!< Metallic map.
	float m_metallicFactor;   //!< Metallic factor.
};

struct RenderComponent
{
	max::MeshHandle m_mesh;  //!< Handle to mesh.
	bool m_castShadows;		 //!< Should cast shadows.
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
	float m_aspect;
	bx::Vec3 m_position;
	bx::Vec3 m_direction;
	bx::Vec3 m_up;
};

struct DirectionalLightComponent
{
	bx::Vec3 m_direction;
	bx::Vec3 m_color;
};