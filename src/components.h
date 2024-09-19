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

struct LightComponent
{
	union Position
	{
		struct
		{
			float m_x;
			float m_y;
			float m_z;
			float m_w;
		};

		float m_v[4];
	};

	union LightRgbPower
	{
		struct
		{
			float m_r;
			float m_g;
			float m_b;
			float m_power;
		};

		float m_v[4];
	};

	union SpotDirectionInner
	{
		struct
		{
			float m_x;
			float m_y;
			float m_z;
			float m_inner;
		};

		float m_v[4];
	};

	union AttenuationSpotOuter
	{
		struct
		{
			float m_attnConst;
			float m_attnLinear;
			float m_attnQuadrantic;
			float m_outer;
		};

		float m_v[4];
	};

	Position              m_position;
	float				  m_position_viewSpace[4];
	LightRgbPower         m_ambientPower;
	LightRgbPower         m_diffusePower;
	LightRgbPower         m_specularPower;
	SpotDirectionInner    m_spotDirectionInner;
	float				  m_spotDirectionInner_viewSpace[4];
	AttenuationSpotOuter  m_attenuationSpotOuter;
};