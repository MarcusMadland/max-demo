#include "render.h"

#include <max/max.h>
#include <bimg/bimg.h>
#include <bx/file.h>
#include <bx/readerwriter.h>

#include "components.h"

struct ScreenSpaceQuadVertex
{
	float m_x;
	float m_y;
	float m_z;
	float m_u;
	float m_v;

	static void init()
	{
		ms_layout
			.begin()
			.add(max::Attrib::Position, 3, max::AttribType::Float)
			.add(max::Attrib::TexCoord0, 2, max::AttribType::Float)
			.end();
	}

	static max::VertexLayout ms_layout;
};
max::VertexLayout ScreenSpaceQuadVertex::ms_layout;

/// Submit screen sized quad buffer.
///
static void screenSpaceQuad(bool _originBottomLeft, float _width = 1.0f, float _height = 1.0f)
{
	if (3 == max::getAvailTransientVertexBuffer(3, ScreenSpaceQuadVertex::ms_layout))
	{
		max::TransientVertexBuffer vb;
		max::allocTransientVertexBuffer(&vb, 3, ScreenSpaceQuadVertex::ms_layout);
		ScreenSpaceQuadVertex* vertex = (ScreenSpaceQuadVertex*)vb.data;

		const float minx = -_width;
		const float maxx = _width;
		const float miny = 0.0f;
		const float maxy = _height * 2.0f;

		const float minu = -1.0f;
		const float maxu = 1.0f;

		const float zz = 0.0f;

		float minv = 0.0f;
		float maxv = 2.0f;

		if (_originBottomLeft)
		{
			float temp = minv;
			minv = maxv;
			maxv = temp;

			minv -= 1.0f;
			maxv -= 1.0f;
		}

		vertex[0].m_x = minx;
		vertex[0].m_y = miny;
		vertex[0].m_z = zz;
		vertex[0].m_u = minu;
		vertex[0].m_v = minv;

		vertex[1].m_x = maxx;
		vertex[1].m_y = miny;
		vertex[1].m_z = zz;
		vertex[1].m_u = maxu;
		vertex[1].m_v = minv;

		vertex[2].m_x = maxx;
		vertex[2].m_y = maxy;
		vertex[2].m_z = zz;
		vertex[2].m_u = maxu;
		vertex[2].m_v = maxv;

		max::setVertexBuffer(0, &vb);
	}
}

/// Face enum used for cubemaps sides.
///
struct Faces
{
	enum Enum
	{
		Up,
		Down,
		Right,
		Left,
		Front,
		Back,

		Count
	};
};

const bx::Vec3 s_targets[Faces::Count] =
{
	{ 1.0f,  0.0f,  0.0f },  // +X 
	{-1.0f,  0.0f,  0.0f },  // -X
	{ 0.0f,  1.0f,  0.0f },  // +Y
	{ 0.0f, -1.0f,  0.0f },  // -Y
	{ 0.0f,  0.0f,  1.0f },  // +Z
	{ 0.0f,  0.0f, -1.0f },  // -Z
};

const bx::Vec3 s_ups[Faces::Count] =
{
	{ 0.0f,  1.0f,  0.0f },  // +X
	{ 0.0f,  1.0f,  0.0f },  // -X
	{ 0.0f,  0.0f, -1.0f },  // +Y
	{ 0.0f,  0.0f,  1.0f },  // -Y
	{ 0.0f,  1.0f,  0.0f },  // +Z
	{ 0.0f,  1.0f,  0.0f },  // -Z
};

/// Global uniforms.
///
struct Uniforms
{
	void create()
	{
		u_perFrame = max::createUniform("u_perframe", max::UniformType::Vec4, 20);
		u_perDraw = max::createUniform("u_perdraw", max::UniformType::Vec4, 3);
	}

	void destroy()
	{
		max::destroy(u_perFrame);
		max::destroy(u_perDraw);
	}

	void submitPerFrame()
	{
		max::setUniform(u_perFrame, m_perFrame, 20);
	}

	void submitPerDraw()
	{
		max::setUniform(u_perDraw, m_perDraw, 3);
	}

	union
	{
		struct
		{
			union
			{
				float m_invViewProj[16];
				/* 0*/ struct { float m_invViewProj0[4]; };
				/* 1*/ struct { float m_invViewProj1[4]; };
				/* 2*/ struct { float m_invViewProj2[4]; };
				/* 3*/ struct { float m_invViewProj3[4]; };
			};
			union
			{
				float m_cameraMtx[16];
				/* 4*/ struct { float m_cameraMtx0[4]; };
				/* 5*/ struct { float m_cameraMtx1[4]; };
				/* 6*/ struct { float m_cameraMtx2[4]; };
				/* 7*/ struct { float m_cameraMtx3[4]; };
			};
			/*8*/  struct { float m_lightDir[3],     m_width; };
			/*9*/  struct { float m_lightCol[3],     m_height; };
			/*10*/ struct { float m_volumeMin[3],    m_volumeSpacing; };
			/*11*/ struct { float m_volumeMax[3],    m_time; };
			/*12*/ struct { float m_volumeSize[3],   m_exposition; };
			/*13*/ struct { float m_skyLuminance[3], m_sunBloom; };
			/*14*/ struct { float m_sunLuminance[3], m_sunSize; };
			union
			{
				float m_perezCoeff[20];
				/* 15*/ struct { float m_perezCoeff0[4]; };
				/* 16*/ struct { float m_perezCoeff1[4]; };
				/* 17*/ struct { float m_perezCoeff2[4]; };
				/* 18*/ struct { float m_perezCoeff3[4]; };
				/* 19*/ struct { float m_perezCoeff4[4]; };
			};
			/*20 COUNT*/
		};

		float m_perFrame[20 * 4];
	};

	union
	{
		struct
		{
			/*0*/ struct { float m_texDiffuseFactor[3], m_texRoughnessFactor; };
			/*1*/ struct { float m_texNormalFactor[3],  m_texMetallicFactor; };
			/*2*/ struct { float m_probeGridPos[3],     m_unused15; };
			/*3 COUNT*/
		};

		float m_perDraw[3 * 4];
	};

	max::UniformHandle u_perFrame;
	max::UniformHandle u_perDraw;
};

/// Global samplers.
///
struct Samplers
{
	void create()
	{
		s_materialDiffuse = max::createUniform("s_materialDiffuse", max::UniformType::Sampler);
		s_materialNormal = max::createUniform("s_materialNormal", max::UniformType::Sampler);
		s_materialRoughness = max::createUniform("s_materialRoughness", max::UniformType::Sampler);
		s_materialMetallic = max::createUniform("s_materialMetallic", max::UniformType::Sampler);
		s_cubeDiffuse = max::createUniform("s_cubeDiffuse", max::UniformType::Sampler);
		s_cubeNormal = max::createUniform("s_cubeNormal", max::UniformType::Sampler);
		s_cubePosition = max::createUniform("s_cubePosition", max::UniformType::Sampler);
		s_cubeDepth = max::createUniform("s_cubeDepth", max::UniformType::Sampler);
		s_atlasDiffuse = max::createUniform("s_atlasDiffuse", max::UniformType::Sampler);
		s_atlasNormal = max::createUniform("s_atlasNormal", max::UniformType::Sampler);
		s_atlasPosition = max::createUniform("s_atlasPosition", max::UniformType::Sampler);
		s_atlasDepth = max::createUniform("s_atlasDepth", max::UniformType::Sampler);
		s_atlasRadiance = max::createUniform("s_atlasRadiance", max::UniformType::Sampler);
		s_atlasIrradiance = max::createUniform("s_atlasIrradiance", max::UniformType::Sampler);
		s_gbufferDiffuse = max::createUniform("s_gbufferDiffuse", max::UniformType::Sampler);
		s_gbufferNormal = max::createUniform("s_gbufferNormal", max::UniformType::Sampler);
		s_gbufferSurface = max::createUniform("s_gbufferSurface", max::UniformType::Sampler);
		s_gbufferDepth = max::createUniform("s_gbufferDepth", max::UniformType::Sampler);
	}

	void destroy()
	{
		max::destroy(s_materialDiffuse);
		max::destroy(s_materialNormal);
		max::destroy(s_materialRoughness);
		max::destroy(s_materialMetallic);
		max::destroy(s_cubeDiffuse);
		max::destroy(s_cubeNormal);
		max::destroy(s_cubePosition);
		max::destroy(s_cubeDepth);
		max::destroy(s_atlasDiffuse);
		max::destroy(s_atlasNormal);
		max::destroy(s_atlasPosition);
		max::destroy(s_atlasDepth);
		max::destroy(s_atlasRadiance);
		max::destroy(s_atlasIrradiance);
		max::destroy(s_gbufferDiffuse);
		max::destroy(s_gbufferNormal);
		max::destroy(s_gbufferSurface);
		max::destroy(s_gbufferDepth);
	}


	max::UniformHandle s_materialDiffuse;
	max::UniformHandle s_materialNormal;
	max::UniformHandle s_materialRoughness;
	max::UniformHandle s_materialMetallic;
	max::UniformHandle s_cubeDiffuse;
	max::UniformHandle s_cubeNormal;
	max::UniformHandle s_cubePosition;
	max::UniformHandle s_cubeDepth;
	max::UniformHandle s_atlasDiffuse;
	max::UniformHandle s_atlasNormal;
	max::UniformHandle s_atlasPosition;
	max::UniformHandle s_atlasDepth;
	max::UniformHandle s_atlasRadiance;
	max::UniformHandle s_atlasIrradiance;
	max::UniformHandle s_gbufferDiffuse;
	max::UniformHandle s_gbufferNormal;
	max::UniformHandle s_gbufferSurface;
	max::UniformHandle s_gbufferDepth;
};

/// Create default texture with given color.
/// 
static max::TextureHandle createDefaultTexture(const uint8_t _r, const uint8_t _g, const uint8_t _b, const uint8_t _a)
{
	const uint32_t dim = 8;
	const uint32_t textureSize = dim * dim * 4;
	uint8_t* data = new uint8_t[textureSize];
	for (uint32_t i = 0; i < textureSize; i += 4)
	{
		data[i + 0] = _r;
		data[i + 1] = _g;
		data[i + 2] = _b;
		data[i + 3] = _a;
	}
	const max::Memory* mem = max::copy(data, textureSize);
	max::TextureHandle texture = max::createTexture2D(
		dim, dim, false, 1, max::TextureFormat::RGBA8, 0, mem
	);
	delete[] data;

	return texture;
}

/// Global material.
/// 
struct Material
{
	void create(Samplers* _samplers, Uniforms* _uniforms)
	{
		m_samplers = _samplers;
		m_uniforms = _uniforms;

		m_whiteTexture = createDefaultTexture(255, 255, 255, 255);
		m_normalTexture = createDefaultTexture(128, 128, 255, 255);
	}

	void destroy()
	{
		max::destroy(m_normalTexture);
		max::destroy(m_whiteTexture);
	}

	void submitPerDraw()
	{
		max::setTexture(0, m_samplers->s_materialDiffuse, m_texDiffuse);
		m_uniforms->m_texDiffuseFactor[0] = m_factorDiffuse[0];
		m_uniforms->m_texDiffuseFactor[1] = m_factorDiffuse[1];
		m_uniforms->m_texDiffuseFactor[2] = m_factorDiffuse[2];

		max::setTexture(1,m_samplers->s_materialNormal, m_texNormal);
		m_uniforms->m_texNormalFactor[0] = m_factorNormal[0];
		m_uniforms->m_texNormalFactor[1] = m_factorNormal[1];
		m_uniforms->m_texNormalFactor[2] = m_factorNormal[2];

		max::setTexture(2, m_samplers->s_materialRoughness, m_texRoughness);
		m_uniforms->m_texRoughnessFactor = m_factorRoughness;

		max::setTexture(3, m_samplers->s_materialMetallic, m_texMetallic);
		m_uniforms->m_texMetallicFactor = m_factorMetallic;
	}

	void setDiffuse(max::TextureHandle _texture, float _r, float _g, float _b)
	{
		if (isValid(_texture))
		{
			m_texDiffuse = _texture;
		}
		else
		{
			m_texDiffuse = m_whiteTexture;
		}

		m_factorDiffuse[0] = _r;
		m_factorDiffuse[1] = _g;
		m_factorDiffuse[2] = _b;
		m_factorDiffuse[3] = 1.0f;
	}

	void setNormal(max::TextureHandle _texture, float _r, float _g, float _b)
	{
		if (isValid(_texture))
		{
			m_texNormal = _texture;
		}
		else
		{
			m_texNormal = m_normalTexture;
		}

		m_factorNormal[0] = _r;
		m_factorNormal[1] = _g;
		m_factorNormal[2] = _b;
		m_factorNormal[3] = 1.0f;
	}

	void setRoughness(max::TextureHandle _texture, float _r)
	{
		if (isValid(_texture))
		{
			m_texRoughness = _texture;
		}
		else
		{
			m_texRoughness = m_whiteTexture;
		}

		m_factorRoughness = _r;
	}

	void setMetallic(max::TextureHandle _texture, float _r)
	{
		if (isValid(_texture))
		{
			m_texMetallic = _texture;
		}
		else
		{
			m_texMetallic = m_whiteTexture;
		}

		m_factorMetallic = _r;
	}

	Samplers* m_samplers;
	Uniforms* m_uniforms;

	max::TextureHandle m_texDiffuse;
	max::TextureHandle m_texNormal;
	max::TextureHandle m_texRoughness;
	max::TextureHandle m_texMetallic;

	float m_factorDiffuse[4];
	float m_factorNormal[4];
	float m_factorRoughness;
	float m_factorMetallic;

private:
	max::TextureHandle m_whiteTexture;
	max::TextureHandle m_normalTexture;
};

/// Probe volume.
///
struct Probes
{
	struct Probe 
	{ 
		bx::Vec3 m_pos;	    //!< Position of probe in world space.    
		bx::Vec3 m_gridPos; //!< Position of probe in grid space.
	};

	Probes()
		: m_gridSize(0.0f)
		, m_position(0.0f)
		, m_spacing(0.0f)
		, m_num(0)
		, m_probes(NULL)
	{}

	void create(const bx::Vec3& _pos, const uint32_t _numX, const uint32_t _numY, const uint32_t _numZ, const float _spacing)
	{
		m_gridSize = bx::Vec3((float)_numX, (float)_numY, (float)_numZ);
		m_position = _pos;
		m_spacing = _spacing;

		m_num = _numX * _numY * _numZ;
		m_probes = (Probe*)bx::alloc(max::getAllocator(), m_num * sizeof(Probe));

		uint32_t index = 0;
		for (uint32_t x = 0; x < _numX; ++x)
		{
			for (uint32_t y = 0; y < _numY; ++y)
			{
				for (uint32_t z = 0; z < _numZ; ++z)
				{
					m_probes[index].m_gridPos = bx::Vec3((float)x, (float)y, (float)z);
					m_probes[index].m_pos = bx::add(m_position, bx::Vec3(x * _spacing, y * _spacing, z * _spacing));

					++index;
				}
			}
		}
	}

	void destroy()
	{
		bx::free(max::getAllocator(), m_probes);
		m_probes = NULL;
	}

	const bx::Vec3 extents()
	{
		return bx::Vec3((m_gridSize.x - 1) * m_spacing,
			            (m_gridSize.y - 1) * m_spacing,
			            (m_gridSize.z - 1) * m_spacing);
	}

	bx::Vec3 m_gridSize; //!< Number of probes in each direction.
	bx::Vec3 m_position; //!< Grid position in world space.
	float m_spacing;	 //!< World space spacing between probes.

	uint32_t m_num;	 //!< Total number of probes.
	Probe* m_probes; //!< Probes.
};

/// Common data used across render techniques.
/// 
struct CommonResources
{
	RenderSettings* m_settings;

	float m_view[16];
	float m_proj[16];

	float m_viewDir[3];

	float m_sunDir[3];
	float m_sunCol[3];

	Uniforms* m_uniforms;
	Samplers* m_samplers;
	Material* m_material;
	Probes* m_probes;

	bool m_firstFrame;
};

/// Get scaled resolution clamped with window max res.
/// 
static const RenderSettings::Rect getScaledResolution(CommonResources* _common)
{
	if (_common->m_settings->m_resolution.m_width > _common->m_settings->m_viewport.m_width ||
		_common->m_settings->m_resolution.m_height > _common->m_settings->m_viewport.m_height)
	{
		return { _common->m_settings->m_viewport.m_width, _common->m_settings->m_viewport.m_height };
	}
	else
	{
		return { _common->m_settings->m_resolution.m_width, _common->m_settings->m_resolution.m_height };
	}
}

/// Data used to submit scene geometry for rendering.
///
struct RenderData
{
	max::ViewId m_view;
	max::ProgramHandle m_program;

	CommonResources* m_common;

	bool m_material; 
};

constexpr uint32_t kMaxRenderables = 1000;

/// Submit scene geometry for rendering.
///
static void submit(RenderData* _renderData)
{
	max::System<TransformComponent, RenderComponent> renderables;
	renderables.each(kMaxRenderables, [](max::EntityHandle _entity, void* _userData)
	{
		RenderData* data = (RenderData*)_userData;

		TransformComponent* tc = max::getComponent<TransformComponent>(_entity);
		RenderComponent* rc = max::getComponent<RenderComponent>(_entity);

		max::MeshQuery* query = max::queryMesh(rc->m_mesh);
		for (uint32_t ii = 0; ii < query->m_num; ++ii)
		{
			float mtx[16];
			bx::mtxSRT(mtx,
				tc->m_scale.x, tc->m_scale.y, tc->m_scale.z,
				tc->m_rotation.x, tc->m_rotation.y, tc->m_rotation.z, tc->m_rotation.w,
				tc->m_position.x, tc->m_position.y, tc->m_position.z);

			max::setTransform(mtx);

			max::setVertexBuffer(0, query->m_vertices[ii]);
			max::setIndexBuffer(query->m_indices[ii]);

			if (data->m_material)
			{
				MaterialComponent* mc = max::getComponent<MaterialComponent>(_entity);
				if (mc != NULL)
				{
					data->m_common->m_material->setDiffuse(mc->m_diffuse.m_texture, mc->m_diffuseFactor[0], mc->m_diffuseFactor[1], mc->m_diffuseFactor[2]);
					data->m_common->m_material->setNormal(mc->m_normal.m_texture, mc->m_normalFactor[0], mc->m_normalFactor[1], mc->m_normalFactor[2]);
					data->m_common->m_material->setRoughness(mc->m_roughness.m_texture, mc->m_roughnessFactor);
					data->m_common->m_material->setMetallic(mc->m_metallic.m_texture, mc->m_metallicFactor);
				}

				data->m_common->m_material->submitPerDraw();
				data->m_common->m_uniforms->submitPerDraw();
			}

			max::setState(0
				| MAX_STATE_WRITE_RGB
				| MAX_STATE_WRITE_A
				| MAX_STATE_WRITE_Z
				| MAX_STATE_DEPTH_TEST_LESS
				| MAX_STATE_MSAA
			);

			max::submit(data->m_view, data->m_program);
		}

	}, _renderData);
}

/// Shadow Mapping @todo Cascaded shadow mapping when?
///
struct SM
{
	void create(CommonResources* _common, max::ViewId _view)
	{
		m_view = _view;
		m_common = _common;

		//
		m_program = max::loadProgram("vs_shadow", "fs_shadow");

		// Don't create framebuffer until first render call.
		m_framebuffer.idx = max::kInvalidHandle;
	}

	void destroy()
	{
		destroyFramebuffer();

		max::destroy(m_program);
	}

	void render()
	{
		// Recreate shadow map upon reset.
		if (m_common->m_firstFrame)
		{
			destroyFramebuffer();
			createFramebuffer(m_common->m_settings->m_shadowMap.m_width, m_common->m_settings->m_shadowMap.m_height);
		}

		// Calculate view.
		const bx::Vec3 eye = { -m_common->m_sunDir[0], -m_common->m_sunDir[1], -m_common->m_sunDir[2] }; 
		const bx::Vec3 at = { 0.0f,  0.0f, 0.0f };
		float view[16];
		bx::mtxLookAt(view, eye, at); 

		// Calculate proj.
		const float area = 30.0f; 
		float proj[16];
		bx::mtxOrtho(proj, -area, area, -area, area, -100.0f, 100.0f, 0.0f, max::getCaps()->homogeneousDepth);

		// Render scene geometry to shadow map.
		max::setViewRect(m_view, 0, 0, m_common->m_settings->m_shadowMap.m_width, m_common->m_settings->m_shadowMap.m_height);
		max::setViewFrameBuffer(m_view, m_framebuffer);
		max::setViewTransform(m_view, view, proj);
		max::setViewClear(m_view, MAX_CLEAR_COLOR | MAX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);

		m_renderData.m_view = m_view;
		m_renderData.m_program = m_program;
		m_renderData.m_common = m_common;
		m_renderData.m_material = false;
		submit(&m_renderData);
	}

	void createFramebuffer(uint32_t _width, uint32_t _height)
	{
		BX_ASSERT(0 != (max::getCaps()->supported & MAX_CAPS_TEXTURE_COMPARE_LEQUAL), "Shadow texture mapping not supported on this device.")

		max::TextureHandle fbtextures[] =
		{
			max::createTexture2D(
					_width
				, _height
				, false
				, 1
				, max::TextureFormat::D16
				, MAX_TEXTURE_RT | MAX_SAMPLER_COMPARE_LEQUAL
				),
		};
		m_framebuffer = max::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
	}

	void destroyFramebuffer()
	{
		if (max::isValid(m_framebuffer))
		{
			max::destroy(m_framebuffer); // Texture is destroyed with it.
		}
	}

	max::ViewId m_view;
	CommonResources* m_common;

	RenderData m_renderData;

	max::ProgramHandle m_program;
	max::FrameBufferHandle m_framebuffer;
};
 
/// Deferred GBuffer.
///
struct GBuffer
{
	enum TextureType
	{
		Diffuse, //!< .rgb = Diffuse / Albedo / Base color map.
		Normal,  //!< .rgb = World normals.
		Surface, //!< .r = Roughness, .g = Metallic
		Depth,   //!< .r = Depth

		Count
	};

	void create(CommonResources* _common, max::ViewId _view)
	{
		m_view = _view;
		m_common = _common;

		//
		m_program = max::loadProgram("vs_gbuffer", "fs_gbuffer");

		// Don't create framebuffer until first render call.
		m_framebuffer.idx = max::kInvalidHandle;
	}

	void destroy()
	{
		destroyFramebuffer();
		
		max::destroy(m_program);
	}

	void render()
	{
		// Recreate gbuffer upon reset. 
		if (m_common->m_firstFrame)
		{
			destroyFramebuffer();
			createFramebuffer();
		}

		// Render scene to gbuffer.
		max::setViewFrameBuffer(m_view, m_framebuffer);
		max::setViewRect(m_view, 0, 0, m_common->m_settings->m_viewport.m_width, m_common->m_settings->m_viewport.m_height);
		max::setViewClear(m_view, MAX_CLEAR_COLOR | MAX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
		max::setViewTransform(m_view, m_common->m_view, m_common->m_proj);

		m_renderData.m_view = m_view;
		m_renderData.m_program = m_program;
		m_renderData.m_common = m_common;
		m_renderData.m_material = true;
		submit(&m_renderData);
	}

	void createFramebuffer()
	{
		const RenderSettings::Rect rect = getScaledResolution(m_common);

		max::TextureHandle fbtextures[] =
		{
			max::createTexture2D(rect.m_width, rect.m_height, false, 1, max::TextureFormat::RGBA8, MAX_TEXTURE_RT),   // TextureType::Diffuse
			max::createTexture2D(rect.m_width, rect.m_height, false, 1, max::TextureFormat::RGBA16F, MAX_TEXTURE_RT), // TextureType::Normal
			max::createTexture2D(rect.m_width, rect.m_height, false, 1, max::TextureFormat::RGBA8, MAX_TEXTURE_RT),   // TextureType::Surface
			max::createTexture2D(rect.m_width, rect.m_height, false, 1, max::TextureFormat::D32F, MAX_TEXTURE_RT)     // TextureType::Depth
		};
		m_framebuffer = max::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
	}

	void destroyFramebuffer()
	{
		if (isValid(m_framebuffer))
		{
			max::destroy(m_framebuffer); // Textures are destroyed with it.
		}
	}

	max::ViewId m_view;
	CommonResources* m_common;

	RenderData m_renderData;

	max::ProgramHandle m_program;
	max::FrameBufferHandle m_framebuffer;
};

// @todo Move this.
constexpr uint32_t kCubemapResolution = 1024;
constexpr uint32_t kOctahedralResolution = 48;

/// Global Illumination.
///
struct GI
{
	void create(CommonResources* _common, max::ViewId _view0, max::ViewId _view1)
	{
		// Create data.
		m_viewId0 = _view0;
		m_viewId1 = _view1;
		m_common = _common;

		//
		m_precomputed = false;

		const uint32_t atlasWidth = (m_common->m_probes->m_gridSize.x * m_common->m_probes->m_gridSize.z) * kOctahedralResolution;
		const uint32_t atlasHeight = m_common->m_probes->m_gridSize.y * kOctahedralResolution;

		max::TextureHandle radiance = max::createTexture2D(atlasWidth, atlasHeight, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_RT);
		m_framebufferRadiance = max::createFrameBuffer(1, &radiance, true);

		max::TextureHandle irradiance = max::createTexture2D(atlasWidth, atlasHeight, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_RT);
		m_framebufferIrradiance = max::createFrameBuffer(1, &irradiance, true);

		m_programLightDir = max::loadProgram("vs_screen", "fs_light_dir");
		m_programPrefilter = max::loadProgram("vs_screen", "fs_prefilter");

		//
		m_diffuseAtlas = max::createTexture2D(atlasWidth, atlasHeight, false, 1, max::TextureFormat::RGBA8, MAX_TEXTURE_BLIT_DST);
		m_normalAtlas = max::createTexture2D(atlasWidth, atlasHeight, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_BLIT_DST);
		m_positionAtlas = max::createTexture2D(atlasWidth, atlasHeight, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_BLIT_DST);
		m_depthAtlas = max::createTexture2D(atlasWidth, atlasHeight, false, 1, max::TextureFormat::R32F, MAX_TEXTURE_BLIT_DST);

		m_programCubemap = max::loadProgram("vs_gbuffer", "fs_gbuffer_cubemap");

		m_programOctahedral = max::loadProgram("vs_screen", "fs_octahedral");
	}

	void destroy()
	{
		max::destroy(m_programOctahedral);
		max::destroy(m_programCubemap);

		max::destroy(m_depthAtlas);
		max::destroy(m_positionAtlas);
		max::destroy(m_normalAtlas);
		max::destroy(m_diffuseAtlas);

		max::destroy(m_programPrefilter);
		max::destroy(m_programLightDir);
		max::destroy(m_framebufferIrradiance);
		max::destroy(m_framebufferRadiance);
	}

	void render()
	{
		float viewProj[16];
		bx::mtxMul(viewProj, m_common->m_view, m_common->m_proj);

		float invViewProj[16];
		bx::mtxInverse(invViewProj, viewProj);

		// We have to transpose it manually since we are using custom vec4 uniforms instead of 'max::setTransform'.
		if (max::getRendererType() != max::RendererType::OpenGL)
		{
			float invViewProjTransposed[16];
			bx::mtxTranspose(invViewProjTransposed, invViewProj);

			bx::memCopy(m_common->m_uniforms->m_invViewProj, invViewProjTransposed, 16 * sizeof(float));
		}
		else // Not for openGL tho..
		{
			bx::memCopy(m_common->m_uniforms->m_invViewProj, invViewProj, 16 * sizeof(float));
		}

		const uint32_t atlasWidth = (m_common->m_probes->m_gridSize.x * m_common->m_probes->m_gridSize.z) * kOctahedralResolution;
		const uint32_t atlasHeight = m_common->m_probes->m_gridSize.y * kOctahedralResolution;

		if (m_precomputed)
		{
			// Directional Light. (Radiance atlas)
			float proj[16];
			bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, max::getCaps()->homogeneousDepth);

			max::setViewFrameBuffer(m_viewId0, m_framebufferRadiance);
			max::setViewRect(m_viewId0, 0, 0, atlasWidth, atlasHeight);
			max::setViewTransform(m_viewId0, NULL, proj);

			m_common->m_uniforms->m_lightDir[0] = m_common->m_sunDir[0];
			m_common->m_uniforms->m_lightDir[1] = m_common->m_sunDir[1];
			m_common->m_uniforms->m_lightDir[2] = m_common->m_sunDir[2];
			m_common->m_uniforms->m_lightCol[0] = m_common->m_sunCol[0];
			m_common->m_uniforms->m_lightCol[1] = m_common->m_sunCol[1];
			m_common->m_uniforms->m_lightCol[2] = m_common->m_sunCol[2];

			max::setTexture(0, m_common->m_samplers->s_atlasDiffuse,  m_diffuseAtlas);
			max::setTexture(1, m_common->m_samplers->s_atlasNormal,   m_normalAtlas);
			max::setState(0
				| MAX_STATE_WRITE_RGB
				| MAX_STATE_WRITE_A
				//| MAX_STATE_BLEND_ADD // @todo For multiple lights, but currently ruins output buffer.
			);
			screenSpaceQuad(max::getCaps()->originBottomLeft);

			max::submit(m_viewId0, m_programLightDir);

			// Convolve radiance atlas into SH coefficients irradiance atlas.
			max::setViewFrameBuffer(m_viewId1, m_framebufferIrradiance);
			max::setViewRect(m_viewId1, 0, 0, atlasWidth, atlasHeight);
			max::setViewTransform(m_viewId1, NULL, proj);
			
			max::setTexture(0, m_common->m_samplers->s_atlasRadiance, max::getTexture(m_framebufferRadiance));
			max::setTexture(1, m_common->m_samplers->s_atlasNormal, m_normalAtlas);
			max::setTexture(2, m_common->m_samplers->s_atlasPosition,  m_positionAtlas);
			max::setTexture(3, m_common->m_samplers->s_atlasDepth, m_depthAtlas);
			max::setState(0
				| MAX_STATE_WRITE_RGB
				| MAX_STATE_WRITE_A
			);
			screenSpaceQuad(max::getCaps()->originBottomLeft);

			max::submit(m_viewId1, m_programPrefilter);
		}
		else
		{
			for (uint32_t ii = 0; ii < m_common->m_probes->m_num; ++ii)
			{
				// Create gbuffer cubemap rendertarget.
				max::FrameBufferHandle cubeFramebuffers[Faces::Count];

				max::TextureHandle diffuseCube = max::createTextureCube(kCubemapResolution, false, 1, max::TextureFormat::RGBA8, MAX_TEXTURE_RT);
				max::TextureHandle normalCube = max::createTextureCube(kCubemapResolution, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_RT);
				max::TextureHandle positionCube = max::createTextureCube(kCubemapResolution, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_RT);
				max::TextureHandle depthCube = max::createTextureCube(kCubemapResolution, false, 1, max::TextureFormat::D32F, MAX_TEXTURE_RT);

				for (uint32_t jj = 0; jj < Faces::Count; ++jj)
				{
					max::Attachment at[4];
					at[0].init(diffuseCube, max::Access::Write, uint16_t(jj));
					at[1].init(normalCube, max::Access::Write, uint16_t(jj));
					at[2].init(positionCube, max::Access::Write, uint16_t(jj));
					at[3].init(depthCube, max::Access::Write, uint16_t(jj));
					cubeFramebuffers[jj] = max::createFrameBuffer(BX_COUNTOF(at), at, true);
				}

				// Submit render all sides of cubemap.
				m_viewIdOffline = 0;
				for (uint8_t jj = 0; jj < Faces::Count; ++jj)
				{
					float view[16];
					bx::mtxLookAt(view, m_common->m_probes->m_probes[ii].m_pos, bx::add(m_common->m_probes->m_probes[ii].m_pos, s_targets[jj]), s_ups[jj]);

					float proj[16];
					bx::mtxProj(proj, 90.0f, 1.0f, 0.001f, 1000.0f, max::getCaps()->homogeneousDepth);

					max::setViewTransform(m_viewIdOffline, view, proj);
					max::setViewFrameBuffer(m_viewIdOffline, cubeFramebuffers[jj]);
					max::setViewRect(m_viewIdOffline, 0, 0, kCubemapResolution, kCubemapResolution);
					max::setViewClear(m_viewIdOffline, MAX_CLEAR_COLOR | MAX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);

					m_renderData.m_view = m_viewIdOffline;
					m_renderData.m_program = m_programCubemap;
					m_renderData.m_common = m_common;
					m_renderData.m_material = true;
					submit(&m_renderData);

					++m_viewIdOffline;
				}

				// Submit cubemap to octahedral map.
				max::TextureHandle diffuseOct = max::createTexture2D(kOctahedralResolution, kOctahedralResolution, false, 1, max::TextureFormat::RGBA8, MAX_TEXTURE_RT);
				max::TextureHandle normalOct = max::createTexture2D(kOctahedralResolution, kOctahedralResolution, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_RT);
				max::TextureHandle positionOct = max::createTexture2D(kOctahedralResolution, kOctahedralResolution, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_RT);
				max::TextureHandle depthOct = max::createTexture2D(kOctahedralResolution, kOctahedralResolution, false, 1, max::TextureFormat::R32F, MAX_TEXTURE_RT);

				max::Attachment at[4];
				at[0].init(diffuseOct, max::Access::Write);
				at[1].init(normalOct, max::Access::Write);
				at[2].init(positionOct, max::Access::Write);
				at[3].init(depthOct, max::Access::Write);
				max::FrameBufferHandle octFramebuffer = max::createFrameBuffer(BX_COUNTOF(at), at, true);

				float proj[16];
				bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, max::getCaps()->homogeneousDepth);

				max::setViewFrameBuffer(m_viewIdOffline, octFramebuffer);
				max::setViewRect(m_viewIdOffline, 0, 0, kOctahedralResolution, kOctahedralResolution);
				max::setViewTransform(m_viewIdOffline, NULL, proj);

				max::setTexture(0, m_common->m_samplers->s_cubeDiffuse,  diffuseCube);
				max::setTexture(1, m_common->m_samplers->s_cubeNormal,   normalCube);
				max::setTexture(2, m_common->m_samplers->s_cubePosition, positionCube);
				max::setTexture(3, m_common->m_samplers->s_cubeDepth,    depthCube);

				max::setState(0 | MAX_STATE_WRITE_RGB | MAX_STATE_WRITE_A);

				screenSpaceQuad(max::getCaps()->originBottomLeft);

				max::submit(m_viewIdOffline, m_programOctahedral);

				m_viewIdOffline++;

				// Blit the octahedral maps into the atlas at the correct position 
				// @todo Is this correct?
				const bx::Vec3 gridPos = m_common->m_probes->m_probes[ii].m_gridPos;
				{
					uint32_t atlasPosX = (gridPos.x + gridPos.z * m_common->m_probes->m_gridSize.z) * kOctahedralResolution;
					uint32_t atlasPosY = gridPos.y * kOctahedralResolution;

					max::blit(m_viewIdOffline, m_diffuseAtlas, atlasPosX, atlasPosY, diffuseOct, 0, 0, kOctahedralResolution, kOctahedralResolution);
					max::blit(m_viewIdOffline, m_normalAtlas, atlasPosX, atlasPosY, normalOct, 0, 0, kOctahedralResolution, kOctahedralResolution);
					max::blit(m_viewIdOffline, m_positionAtlas, atlasPosX, atlasPosY, positionOct, 0, 0, kOctahedralResolution, kOctahedralResolution);
					max::blit(m_viewIdOffline, m_depthAtlas, atlasPosX, atlasPosY, depthOct, 0, 0, kOctahedralResolution, kOctahedralResolution);
				}

				max::frame();

				// Destroy.
				for (uint32_t jj = 0; jj < Faces::Count; ++jj)
				{
					max::destroy(cubeFramebuffers[jj]);
				}

				max::destroy(octFramebuffer);
			}

			m_precomputed = true;
		}
	}

	max::ViewId m_viewId0;
	max::ViewId m_viewId1;
	CommonResources* m_common;
	
	RenderData m_renderData;
	
	max::FrameBufferHandle m_framebufferRadiance;   //!< Radiance atlas framebuffer
	max::FrameBufferHandle m_framebufferIrradiance; //!< Irradiance atlas framebuffer
	max::ProgramHandle m_programLightDir;			//!< Program for directional light radiance
	max::ProgramHandle m_programPrefilter;			//!< Program for prefilter SH irradiance

	// Precomputed
	bool m_precomputed; //!< Are all probes precomputed.
	
	max::TextureHandle m_diffuseAtlas;  //!< Atlas texture for diffuse gbuffer data
	max::TextureHandle m_normalAtlas;   //!< Atlas texture for normal gbuffer data
	max::TextureHandle m_positionAtlas; //!< Atlas texture for position gbuffer data
	max::TextureHandle m_depthAtlas;    //!< Atlas texture for depth gbuffer data
	
	max::ProgramHandle m_programCubemap;    //!< Program thats used to render scene objects to gbuffer cubemaps
	max::ProgramHandle m_programOctahedral; //!< Program thats used to convert cubemap to octahedral

	max::ViewId m_viewIdOffline; //!< ViewID used for precomputing probe gbuffer data.
};

/// Accumulation.
///
struct Accumulation
{
	enum TextureType
	{
		Irradiance, //!< Irradiance light accumulation.
		Specular,   //!< Specular light accumulation.

		Count
	};

	void create(CommonResources* _common, max::ViewId _view)
	{
		m_view = _view;
		m_common = _common;

		//
		m_program = max::loadProgram("vs_screen", "fs_accumulation");

		// Don't create framebuffers and textures until first render call.
		m_framebuffer.idx = max::kInvalidHandle;
	}

	void destroy()
	{
		max::destroy(m_program);

		destroyFramebuffer();
	}

	void render(GBuffer* _gbuffer, GI* _gi)
	{
		// Recreate framebuffer upon reset.
		if (m_common->m_firstFrame)
		{
			destroyFramebuffer();
			createFramebuffer();
		}

		float proj[16];
		bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, max::getCaps()->homogeneousDepth);

		max::setViewFrameBuffer(m_view, m_framebuffer);
		max::setViewRect(m_view, 0, 0, m_common->m_settings->m_viewport.m_width, m_common->m_settings->m_viewport.m_height);
		max::setViewTransform(m_view, NULL, proj);

		max::setTexture(0, m_common->m_samplers->s_gbufferNormal, max::getTexture(_gbuffer->m_framebuffer, GBuffer::TextureType::Normal));
		max::setTexture(1, m_common->m_samplers->s_gbufferSurface, max::getTexture(_gbuffer->m_framebuffer, GBuffer::TextureType::Surface));
		max::setTexture(2, m_common->m_samplers->s_gbufferDepth, max::getTexture(_gbuffer->m_framebuffer, GBuffer::TextureType::Depth));
		max::setTexture(3, m_common->m_samplers->s_atlasIrradiance, max::getTexture(_gi->m_framebufferIrradiance));

		max::setState(0
			| MAX_STATE_WRITE_RGB
			| MAX_STATE_WRITE_A
		);
		screenSpaceQuad(max::getCaps()->originBottomLeft);

		max::submit(m_view, m_program);
	}

	void createFramebuffer()
	{
		const RenderSettings::Rect rect = getScaledResolution(m_common);

		max::TextureHandle fbtextures[] =
		{
			max::createTexture2D(rect.m_width, rect.m_height, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_RT), // TextureType::Irradiance
			max::createTexture2D(rect.m_width, rect.m_height, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_RT)  // TextureType::Specular
		};	
		m_framebuffer = max::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
	}

	void destroyFramebuffer()
	{
		if (isValid(m_framebuffer))
		{
			max::destroy(m_framebuffer); // Textures are destroyed with it.
		}
	}

	max::ViewId m_view;
	CommonResources* m_common;

	max::ProgramHandle m_program;
	max::FrameBufferHandle m_framebuffer;
};

/// Combine.
///
struct Combine
{
	void create(CommonResources* _common, max::ViewId _view)
	{
		m_view = _view;
		m_common = _common;

		//
		m_program = max::loadProgram("vs_screen", "fs_combine");
	}

	void destroy()
	{
		max::destroy(m_program);
	}

	void render(GBuffer* _gbuffer, Accumulation* _acc)
	{
		// @todo Currently just submits buffer to show on screen. Default diffuse / or debug buffer.
		float proj[16];
		bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, max::getCaps()->homogeneousDepth);

		max::setViewFrameBuffer(m_view, MAX_INVALID_HANDLE);
		max::setViewRect(m_view, 0, 0, m_common->m_settings->m_viewport.m_width, m_common->m_settings->m_viewport.m_height);
		max::setViewTransform(m_view, NULL, proj);

		if (m_common->m_settings->m_debugbuffer == RenderSettings::None)
		{
			max::setTexture(0, m_common->m_samplers->s_gbufferDiffuse, max::getTexture(_gbuffer->m_framebuffer, 0));
		}
		else if (m_common->m_settings->m_debugbuffer <= RenderSettings::DebugBuffer::Depth)
		{
			max::setTexture(0, m_common->m_samplers->s_gbufferDiffuse, max::getTexture(_gbuffer->m_framebuffer, (uint8_t)m_common->m_settings->m_debugbuffer - 1));
		}
		else  if (m_common->m_settings->m_debugbuffer <= RenderSettings::DebugBuffer::Irradiance)
		{
			max::setTexture(0, m_common->m_samplers->s_gbufferDiffuse, max::getTexture(_acc->m_framebuffer, 0));
		}
		else  if (m_common->m_settings->m_debugbuffer <= RenderSettings::DebugBuffer::Specular)
		{
			max::setTexture(0, m_common->m_samplers->s_gbufferDiffuse, max::getTexture(_acc->m_framebuffer, 1));
		}

		max::setState(0
			| MAX_STATE_WRITE_RGB
			| MAX_STATE_WRITE_A
		);

		screenSpaceQuad(max::getCaps()->originBottomLeft);

		max::submit(m_view, m_program);
	}

	max::ViewId m_view;
	CommonResources* m_common;

	max::ProgramHandle m_program;
};

/// Forward pass.
/// 
struct Forward
{
	enum TextureType
	{
		Render, //!< .rgb = Rendered.
		Depth,  //!< .r = Depth

		Count
	};

	void create(CommonResources* _common, max::ViewId _view)
	{
		m_view = _view;
		m_common = _common;

		//
		m_meshProbe = max::loadMesh("meshes/sphere.bin");
		m_programProbe = max::loadProgram("vs_probe", "fs_probe");
	}

	void destroy()
	{
		max::destroy(m_programProbe);
		max::destroy(m_meshProbe);
	}

	void render(GBuffer* _gbuffer, GI* _gi)
	{
		max::setViewFrameBuffer(m_view, MAX_INVALID_HANDLE);
		max::setViewRect(m_view, 0, 0, m_common->m_settings->m_viewport.m_width, m_common->m_settings->m_viewport.m_height);
		max::setViewTransform(m_view, m_common->m_view, m_common->m_proj);

		// Debug GI probes.
		max::MeshQuery* probeQuery = max::queryMesh(m_meshProbe);
		if (probeQuery->m_num > 0 && m_common->m_settings->m_debugProbes)
		{
			for (uint32_t ii = 0; ii < m_common->m_probes->m_num; ++ii)
			{
				Probes::Probe& probe = m_common->m_probes->m_probes[ii];

				float mtx[16];
				bx::mtxSRT(mtx, 
					0.25f, 0.25f, 0.25f,
					0.0f, 0.0f, 0.0f, 
					probe.m_pos.x, probe.m_pos.y, probe.m_pos.z
				);
				max::setTransform(mtx);

				max::setVertexBuffer(0, probeQuery->m_vertices[0]);
				max::setIndexBuffer(probeQuery->m_indices[0]);

				m_common->m_uniforms->m_probeGridPos[0] = probe.m_gridPos.x;
				m_common->m_uniforms->m_probeGridPos[1] = probe.m_gridPos.y;
				m_common->m_uniforms->m_probeGridPos[2] = probe.m_gridPos.z;
				m_common->m_uniforms->submitPerDraw();

				max::setTexture(0, m_common->m_samplers->s_atlasIrradiance, max::getTexture(_gi->m_framebufferIrradiance));
				max::setTexture(1, m_common->m_samplers->s_gbufferDepth, max::getTexture(_gbuffer->m_framebuffer, GBuffer::Depth));
				max::setState(0
					| MAX_STATE_WRITE_RGB
					| MAX_STATE_WRITE_A
					| MAX_STATE_WRITE_Z
					| MAX_STATE_DEPTH_TEST_LESS
					| MAX_STATE_MSAA
				);

				max::submit(m_view, m_programProbe);
			}
			
		}
	}

	max::ViewId m_view;
	CommonResources* m_common;

	max::MeshHandle m_meshProbe;
	max::ProgramHandle m_programProbe;
};

/// Skybox.
/// 
/// 
/*
struct Skybox
{
	void create(CommonResources* _common, max::ViewId _view)
	{
		m_view = _view;
		m_common = _common;

		//
		m_program = max::loadProgram("vs_skybox", "fs_skybox");
	}

	void destroy()
	{
		max::destroy(m_program);
	}

	void render(GBuffer* _gbuffer)
	{
		float proj[16];
		bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, max::getCaps()->homogeneousDepth);

		max::setViewFrameBuffer(m_view, MAX_INVALID_HANDLE);
		max::setViewRect(m_view, 0, 0, m_common->m_settings->m_viewport.m_width, m_common->m_settings->m_viewport.m_height);
		max::setViewTransform(m_view, NULL, proj);

		bx::Vec3 toTargetNorm = { -m_common->m_viewDir[0], m_common->m_viewDir[1], m_common->m_viewDir[2] };
		const bx::Vec3 right = bx::normalize(bx::cross({ 0.0f, 1.0f, 0.0f }, toTargetNorm));
		const bx::Vec3 up = bx::normalize(bx::cross(toTargetNorm, right));
		m_common->m_uniforms->m_cameraMtx[0]  = right.x;
		m_common->m_uniforms->m_cameraMtx[1]  = right.y;
		m_common->m_uniforms->m_cameraMtx[2]  = right.z;
		m_common->m_uniforms->m_cameraMtx[3]  = 0.0f;
		m_common->m_uniforms->m_cameraMtx[4]  = up.x;
		m_common->m_uniforms->m_cameraMtx[5]  = up.y;
		m_common->m_uniforms->m_cameraMtx[6]  = up.z;
		m_common->m_uniforms->m_cameraMtx[7]  = 0.0f;
		m_common->m_uniforms->m_cameraMtx[8]  = toTargetNorm.x;
		m_common->m_uniforms->m_cameraMtx[9]  = toTargetNorm.y;
		m_common->m_uniforms->m_cameraMtx[10] = toTargetNorm.z;
		m_common->m_uniforms->m_cameraMtx[11] = 0.0f;
		m_common->m_uniforms->m_cameraMtx[12] = 0.0f;
		m_common->m_uniforms->m_cameraMtx[13] = 0.0f;
		m_common->m_uniforms->m_cameraMtx[14] = 0.0f;
		m_common->m_uniforms->m_cameraMtx[15] = 1.0f;

		max::setTexture(0, m_common->m_samplers->s_skybox, m_common->m_skybox);
		max::setTexture(1, m_common->m_samplers->s_gbufferDepth, max::getTexture(_gbuffer->m_framebuffer, GBuffer::Depth));
		max::setState(0
			| MAX_STATE_WRITE_RGB
			| MAX_STATE_DEPTH_TEST_LESS
		);

		screenSpaceQuad(max::getCaps()->originBottomLeft);

		max::submit(m_view, m_program);
	}

	max::ViewId m_view;
	CommonResources* m_common;

	max::ProgramHandle m_program;
};
*/

/// Perez Sky.
/// 
struct Sky
{
	void create(CommonResources* _common, max::ViewId _view)
	{
		m_view = _view;
		m_common = _common;

		//
		m_program = max::loadProgram("vs_sky", "fs_sky");
	}

	void destroy()
	{
		max::destroy(m_program);
	}

	void render(GBuffer* _gbuffer)
	{
		float proj[16];
		bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, max::getCaps()->homogeneousDepth);

		max::setViewFrameBuffer(m_view, MAX_INVALID_HANDLE);
		max::setViewRect(m_view, 0, 0, m_common->m_settings->m_viewport.m_width, m_common->m_settings->m_viewport.m_height);
		max::setViewTransform(m_view, NULL, proj);

		max::setTexture(0, m_common->m_samplers->s_gbufferDepth, max::getTexture(_gbuffer->m_framebuffer, GBuffer::Depth));
		max::setState(0
			| MAX_STATE_WRITE_RGB
			| MAX_STATE_DEPTH_TEST_LESS
		);

		screenSpaceQuad(max::getCaps()->originBottomLeft);

		max::submit(m_view, m_program);
	}

	max::ViewId m_view;
	CommonResources* m_common;

	max::ProgramHandle m_program;
};

/// Render system.
///
struct RenderSystem
{
	void create(RenderSettings* _settings)
	{
		// Init statics.
		ScreenSpaceQuadVertex::init();

		// Create uniforms params.
		m_uniforms.create();
		m_samplers.create();
		m_material.create(&m_samplers, &m_uniforms);
		m_probes.create({ -4.5f, 0.5f, -4.5f }, 3, 3, 3, 4.5f);

		// Set common resources.
		m_common.m_settings   = _settings;
		m_common.m_uniforms   = &m_uniforms;
		m_common.m_samplers   = &m_samplers;
		m_common.m_material   = &m_material;
		m_common.m_probes     = &m_probes;
		m_common.m_firstFrame = true;

		// Create all render techniques.
		m_sm.create(&m_common, 0);
		max::setViewName(0, "Direct Light SM");

		m_gbuffer.create(&m_common, 1);
		max::setViewName(1, "Color Passes [GBuffer/Deferred]");

		m_gi.create(&m_common, 2, 3);
		max::setViewName(2, "Global Illumination #1");
		max::setViewName(3, "Global Illumination #2");

		m_accumulation.create(&m_common, 4);
		max::setViewName(4, "Irradiance & Specular Accumulation");

		m_combine.create(&m_common, 5);
		max::setViewName(5, "Combine");

		m_sky.create(&m_common, 6);
		max::setViewName(6, "Perez Sky");

		m_forward.create(&m_common, 7);
		max::setViewName(7, "Forward");
	}

	void destroy()
	{
		// Destroy all render techniques.
		m_forward.destroy();
		m_sky.destroy();
		m_combine.destroy();
		m_accumulation.destroy();
		m_gbuffer.destroy();
		m_sm.destroy();
		m_gi.destroy();

		// Destroy uniforms params.
		m_probes.destroy();
		m_material.destroy();
		m_samplers.destroy();
		m_uniforms.destroy();
	}

	void update()
	{
		// Update common resources and uniforms.
		max::System<CameraComponent> camera;
		camera.each(10, [](max::EntityHandle _entity, void* _userData)
		{
			RenderSystem* system = (RenderSystem*)_userData;

			CameraComponent* cc = max::getComponent<CameraComponent>(_entity);
			if (cc->m_idx == system->m_common.m_settings->m_activeCameraIdx)
			{
				bx::memCopy(system->m_common.m_view, cc->m_view, sizeof(float) * 16);
				bx::memCopy(system->m_common.m_proj, cc->m_proj, sizeof(float) * 16);
				bx::memCopy(system->m_common.m_viewDir, &cc->m_direction.x, sizeof(float) * 3);
			}

		}, this);

		max::System<SkyComponent> sun;
		sun.each(1, [](max::EntityHandle _entity, void* _userData)
		{
			RenderSystem* system = (RenderSystem*)_userData;

			SkyComponent* sc = max::getComponent<SkyComponent>(_entity);
			system->m_common.m_sunDir[0] = sc->m_direction.x;
			system->m_common.m_sunDir[1] = sc->m_direction.y;
			system->m_common.m_sunDir[2] = sc->m_direction.z;
			system->m_common.m_sunCol[0] = sc->m_color.x;
			system->m_common.m_sunCol[1] = sc->m_color.y;
			system->m_common.m_sunCol[2] = sc->m_color.z;

		}, this);
		
		// Uniforms. @todo EWWWWWW, you fucking disgusting lil gnome. You know what to do.
		m_uniforms.m_volumeSpacing = m_probes.m_spacing;

		const bx::Vec3 min = m_probes.m_position;
		m_uniforms.m_volumeMin[0] = min.x;
		m_uniforms.m_volumeMin[1] = min.y;
		m_uniforms.m_volumeMin[2] = min.z;

		const bx::Vec3 max = bx::add(m_probes.m_position, m_probes.extents());
		m_uniforms.m_volumeMax[0] = max.x;
		m_uniforms.m_volumeMax[1] = max.y;
		m_uniforms.m_volumeMax[2] = max.z;

		const bx::Vec3 size = m_probes.m_gridSize;
		m_uniforms.m_volumeSize[0] = size.x;
		m_uniforms.m_volumeSize[1] = size.y;
		m_uniforms.m_volumeSize[2] = size.z;

		const RenderSettings::Rect rect = getScaledResolution(&m_common);
		m_uniforms.m_width = rect.m_width;
		m_uniforms.m_height = rect.m_height;

		// Submit all uniforms params.
		m_uniforms.submitPerFrame();

		// Render all render techniques.
		m_sm.render();
		m_gbuffer.render();
		m_gi.render();
		m_accumulation.render(&m_gbuffer, &m_gi);
		m_combine.render(&m_gbuffer, &m_accumulation);
		m_sky.render(&m_gbuffer);
		m_forward.render(&m_gbuffer, &m_gi);

		// Swap buffers.
		max::frame();

		// End frame.
		m_common.m_firstFrame = false;
	}

	void reset()
	{
		// Reset frame.
		m_common.m_firstFrame = true;
	}

	CommonResources m_common;

	Uniforms m_uniforms;
	Samplers m_samplers;
	Material m_material;
	Probes m_probes;

	SM m_sm;
	GBuffer m_gbuffer;
	GI m_gi;
	Accumulation m_accumulation;
	Combine m_combine;
	Sky m_sky;
	Forward m_forward;
};

static RenderSystem* s_ctx = NULL;

void renderCreate(RenderSettings* _settings)
{
	s_ctx = BX_NEW(max::getAllocator(), RenderSystem);
	s_ctx->create(_settings);
}

void renderDestroy()
{
	s_ctx->destroy();
	bx::deleteObject<RenderSystem>(max::getAllocator(), s_ctx);
	s_ctx = NULL;
}

void renderUpdate()
{
	s_ctx->update();
}

void renderReset()
{
	s_ctx->reset();
}