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

/// Global uniforms.
///
struct Uniforms
{
	void create()
	{
		u_perFrame = max::createUniform("u_perframe", max::UniformType::Vec4, 9);
		u_perDraw = max::createUniform("u_perdraw", max::UniformType::Vec4, 3);
	}

	void destroy()
	{
		max::destroy(u_perFrame);
		max::destroy(u_perDraw);
	}

	void submitPerFrame()
	{
		max::setUniform(u_perFrame, m_perFrame, 9);
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
			/*4*/ struct { float m_lightDir[3],   m_unused10; };
			/*5*/ struct { float m_lightCol[3],   m_unused11; };
			/*6*/ struct { float m_volumeMin[3],  m_volumeSpacing; };
			/*7*/ struct { float m_volumeMax[3],  m_unused12; };
			/*8*/ struct { float m_volumeSize[3], m_unused13; };
			/*9 COUNT*/
		};

		float m_perFrame[9 * 4];
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
	void create()
	{
		m_whiteTexture = createDefaultTexture(255, 255, 255, 255);
		m_normalTexture = createDefaultTexture(128, 128, 255, 255);

		s_texDiffuse = max::createUniform("s_texDiffuse", max::UniformType::Sampler);
		s_texNormal = max::createUniform("s_texNormal", max::UniformType::Sampler);
		s_texRoughness = max::createUniform("s_texRoughness", max::UniformType::Sampler);
		s_texMetallic = max::createUniform("s_texMetallic", max::UniformType::Sampler);
	}

	void destroy()
	{
		max::destroy(s_texMetallic);
		max::destroy(s_texRoughness);
		max::destroy(s_texNormal);
		max::destroy(s_texDiffuse);

		max::destroy(m_normalTexture);
		max::destroy(m_whiteTexture);
	}

	void submitPerDraw(Uniforms* _uniforms)
	{
		max::setTexture(0, s_texDiffuse, m_texDiffuse);
		_uniforms->m_texDiffuseFactor[0] = m_factorDiffuse[0];
		_uniforms->m_texDiffuseFactor[1] = m_factorDiffuse[1];
		_uniforms->m_texDiffuseFactor[2] = m_factorDiffuse[2];

		max::setTexture(1, s_texNormal, m_texNormal);
		_uniforms->m_texNormalFactor[0] = m_factorNormal[0];
		_uniforms->m_texNormalFactor[1] = m_factorNormal[1];
		_uniforms->m_texNormalFactor[2] = m_factorNormal[2];

		max::setTexture(2, s_texRoughness, m_texRoughness);
		_uniforms->m_texRoughnessFactor = m_factorRoughness;

		max::setTexture(3, s_texMetallic, m_texMetallic);
		_uniforms->m_texMetallicFactor = m_factorMetallic;
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

	max::UniformHandle s_texDiffuse;
	max::UniformHandle s_texNormal;
	max::UniformHandle s_texRoughness;
	max::UniformHandle s_texMetallic;

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

	Uniforms* m_uniforms;
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

// @todo Move this.
constexpr uint32_t kMaxRenderables = 1000;

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
		float view[16];

		max::System<DirectionalLightComponent> dirLights;
		dirLights.each(1, [](max::EntityHandle _entity, void* _userData)
		{
			float* view = (float*)_userData;

			DirectionalLightComponent* dlc = max::getComponent<DirectionalLightComponent>(_entity);
				
			const bx::Vec3 eye = bx::mul(dlc->m_direction, -100.0f); // @todo What is 100 here? Does this matter?
			const bx::Vec3 at = { 0.0f,  0.0f,   0.0f };

			bx::mtxLookAt(view, bx::mul(eye, 100), at);

		}, view);

		// Calculate proj.
		const float area = 30.0f; 
		
		float proj[16];
		bx::mtxOrtho(proj, -area, area, -area, area, -100.0f, 100.0f, 0.0f, max::getCaps()->homogeneousDepth);

		// Render scene geometry to shadow map.
		max::setViewRect(m_view, 0, 0, m_common->m_settings->m_shadowMap.m_width, m_common->m_settings->m_shadowMap.m_height);
		max::setViewFrameBuffer(m_view, m_framebuffer);
		max::setViewTransform(m_view, view, proj);
		max::setViewClear(m_view, MAX_CLEAR_COLOR | MAX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);

		max::System<TransformComponent, RenderComponent> renderables;
		renderables.each(kMaxRenderables, [](max::EntityHandle _entity, void* _userData)
		{
			SM* sm = (SM*)_userData;

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

				max::setState(0
					| MAX_STATE_WRITE_RGB
					| MAX_STATE_WRITE_A
					| MAX_STATE_WRITE_Z
					| MAX_STATE_DEPTH_TEST_LESS
					| MAX_STATE_MSAA
				);

				max::submit(sm->m_view, sm->m_program);
			}

		}, this);
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

		max::System<TransformComponent, RenderComponent> renderables;
		renderables.each(kMaxRenderables, [](max::EntityHandle _entity, void* _userData)
		{
			GBuffer* gbuffer = (GBuffer*)_userData;

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

				MaterialComponent* mc = max::getComponent<MaterialComponent>(_entity);
				if (mc != NULL)
				{
					gbuffer->m_common->m_material->setDiffuse(mc->m_diffuse.m_texture, mc->m_diffuseFactor[0], mc->m_diffuseFactor[1], mc->m_diffuseFactor[2]);
					gbuffer->m_common->m_material->setNormal(mc->m_normal.m_texture, mc->m_normalFactor[0], mc->m_normalFactor[1], mc->m_normalFactor[2]);
					gbuffer->m_common->m_material->setRoughness(mc->m_roughness.m_texture, mc->m_roughnessFactor);
					gbuffer->m_common->m_material->setMetallic(mc->m_metallic.m_texture, mc->m_metallicFactor);
				}

				gbuffer->m_common->m_material->submitPerDraw(gbuffer->m_common->m_uniforms);
				gbuffer->m_common->m_uniforms->submitPerDraw();

				max::setState(0
					| MAX_STATE_WRITE_RGB
					| MAX_STATE_WRITE_A
					| MAX_STATE_WRITE_Z
					| MAX_STATE_DEPTH_TEST_LESS
					| MAX_STATE_MSAA
				);

				max::submit(gbuffer->m_view, gbuffer->m_program);
			}

		}, this);
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
	void create(CommonResources* _common, max::ViewId _view)
	{
		// Create data.
		m_viewId = _view;
		m_common = _common;

		// 
		m_precomputed = false;

		m_programProbe = max::loadProgram("vs_gbuffer", "fs_gbuffer_cubemap");
		s_texDiffuse = max::createUniform("s_texDiffuse", max::UniformType::Sampler);
		s_texNormal = max::createUniform("s_texNormal", max::UniformType::Sampler);

		m_programOctahedral = max::loadProgram("vs_screen", "fs_octahedral");
		s_diffuse = max::createUniform("s_diffuse", max::UniformType::Sampler);
		s_normal = max::createUniform("s_normal", max::UniformType::Sampler);
		s_position = max::createUniform("s_position", max::UniformType::Sampler);
		s_depth = max::createUniform("s_depth", max::UniformType::Sampler);

		// Create 3D textures (atlas) for each channel (diffuse, normals, position)
		m_atlasWidth = (m_common->m_probes->m_gridSize.x * m_common->m_probes->m_gridSize.z) * kOctahedralResolution;
		m_atlasHeight = m_common->m_probes->m_gridSize.y * kOctahedralResolution;

		m_diffuseAtlas = max::createTexture2D(m_atlasWidth, m_atlasHeight, false, 1, max::TextureFormat::RGBA8, MAX_TEXTURE_BLIT_DST);
		m_normalAtlas = max::createTexture2D(m_atlasWidth, m_atlasHeight, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_BLIT_DST);
		m_positionAtlas = max::createTexture2D(m_atlasWidth, m_atlasHeight, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_BLIT_DST);
		m_depthAtlas = max::createTexture2D(m_atlasWidth, m_atlasHeight, false, 1, max::TextureFormat::D32F, MAX_TEXTURE_BLIT_DST);

		m_radianceTexture = max::createTexture2D(m_atlasWidth, m_atlasHeight, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_RT);
		m_radianceFramebuffer = max::createFrameBuffer(1, &m_radianceTexture);

		m_programLightDir = max::loadProgram("vs_screen", "fs_light_dir");
	}

	void destroy()
	{
		max::destroy(m_programLightDir);

		max::destroy(m_radianceFramebuffer);
		max::destroy(m_radianceTexture);

		max::destroy(m_depthAtlas);
		max::destroy(m_positionAtlas);
		max::destroy(m_normalAtlas);
		max::destroy(m_diffuseAtlas);

		max::destroy(s_depth);
		max::destroy(s_position);
		max::destroy(s_normal);
		max::destroy(s_diffuse);
		max::destroy(m_programOctahedral);

		max::destroy(s_texNormal);
		max::destroy(s_texDiffuse);
		max::destroy(m_programProbe);
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

		// Checks if probe data is baked if not, bake it.
		if (m_precomputed)
		{
			// Directional Light.
			float proj[16];
			bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, max::getCaps()->homogeneousDepth);

			max::setViewFrameBuffer(m_viewId, m_radianceFramebuffer);
			max::setViewRect(m_viewId, 0, 0, m_atlasWidth, m_atlasHeight);
			max::setViewTransform(m_viewId, NULL, proj);

			// Position not needed for dir light
			max::setTexture(3, s_diffuse, m_diffuseAtlas);
			max::setTexture(4, s_normal, m_normalAtlas);
			max::setTexture(5, s_position, m_positionAtlas);

			max::setState(0
				| MAX_STATE_WRITE_RGB
				| MAX_STATE_WRITE_A
				//| MAX_STATE_BLEND_ADD // @todo For multiple lights, but currently ruins output buffer.
			);

			screenSpaceQuad(max::getCaps()->originBottomLeft);

			max::submit(m_viewId, m_programLightDir);

			// Point lights. @todo
			// for (uint32_t ii = 0; ii < m_numPointLights; ++ii)
			// {
			//		max::setTexture(6, s_diffuse,  m_diffuseAtlas);
			//      max::setTexture(7, s_normal,   m_normalAtlas);
			//		max::setTexture(8, s_position, m_positionAtlas);
			// 
			//		max::submit(m_view, m_programLightPoint);
			// }
		}
		else
		{
			// Precompute
			const bx::Vec3 targets[Faces::Count] =
			{
				{ 1.0f,  0.0f,  0.0f },  // +X 
				{-1.0f,  0.0f,  0.0f },  // -X
				{ 0.0f,  1.0f,  0.0f },  // +Y
				{ 0.0f, -1.0f,  0.0f },  // -Y
				{ 0.0f,  0.0f,  1.0f },  // +Z
				{ 0.0f,  0.0f, -1.0f },  // -Z
			};

			const bx::Vec3 ups[Faces::Count] =
			{
				{ 0.0f,  1.0f,  0.0f },  // +X
				{ 0.0f,  1.0f,  0.0f },  // -X
				{ 0.0f,  0.0f, -1.0f },  // +Y
				{ 0.0f,  0.0f,  1.0f },  // -Y
				{ 0.0f,  1.0f,  0.0f },  // +Z
				{ 0.0f,  1.0f,  0.0f },  // -Z
			};

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
					cubeFramebuffers[jj] = max::createFrameBuffer(BX_COUNTOF(at), at);
				}

				// Submit render all sides of cubemap.
				m_viewIdOffline = 0;
				for (uint8_t jj = 0; jj < Faces::Count; ++jj)
				{
					float view[16];
					bx::mtxLookAt(view, m_common->m_probes->m_probes[ii].m_pos, bx::add(m_common->m_probes->m_probes[ii].m_pos, targets[jj]), ups[jj]);

					float proj[16];
					bx::mtxProj(proj, 90.0f, 1.0f, 0.001f, 1000.0f, max::getCaps()->homogeneousDepth);

					max::setViewTransform(m_viewIdOffline, view, proj);
					max::setViewFrameBuffer(m_viewIdOffline, cubeFramebuffers[jj]);
					max::setViewRect(m_viewIdOffline, 0, 0, kCubemapResolution, kCubemapResolution);
					max::setViewClear(m_viewIdOffline, MAX_CLEAR_COLOR | MAX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);

					max::System<TransformComponent> system;
					system.each(kMaxRenderables, [](max::EntityHandle _entity, void* _userData)
						{
							GI* gi = (GI*)_userData;

							TransformComponent* tc = max::getComponent<TransformComponent>(_entity);

							RenderComponent* rc = max::getComponent<RenderComponent>(_entity);
							if (rc != NULL)
							{
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

									MaterialComponent* mc = max::getComponent<MaterialComponent>(_entity);
									if (mc != NULL)
									{
										gi->m_common->m_material->setDiffuse(mc->m_diffuse.m_texture, mc->m_diffuseFactor[0], mc->m_diffuseFactor[1], mc->m_diffuseFactor[2]);
										gi->m_common->m_material->setNormal(mc->m_normal.m_texture, mc->m_normalFactor[0], mc->m_normalFactor[1], mc->m_normalFactor[2]);
										gi->m_common->m_material->setRoughness(mc->m_roughness.m_texture, mc->m_roughnessFactor);
										gi->m_common->m_material->setMetallic(mc->m_metallic.m_texture, mc->m_metallicFactor);
									}

									gi->m_common->m_material->submitPerDraw(gi->m_common->m_uniforms);
									gi->m_common->m_uniforms->submitPerDraw();

									max::setState(0
										| MAX_STATE_WRITE_RGB
										| MAX_STATE_WRITE_A
										| MAX_STATE_WRITE_Z
										| MAX_STATE_DEPTH_TEST_LESS
										| MAX_STATE_MSAA
									);

									max::submit(gi->m_viewIdOffline, gi->m_programProbe);
								}
							}
						}, this);

					++m_viewIdOffline;
				}

				// Submit cubemap to octahedral map.
				max::TextureHandle diffuseOct = max::createTexture2D(kOctahedralResolution, kOctahedralResolution, false, 1, max::TextureFormat::RGBA8, MAX_TEXTURE_RT);
				max::TextureHandle normalOct = max::createTexture2D(kOctahedralResolution, kOctahedralResolution, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_RT);
				max::TextureHandle positionOct = max::createTexture2D(kOctahedralResolution, kOctahedralResolution, false, 1, max::TextureFormat::RG11B10F, MAX_TEXTURE_RT);
				max::TextureHandle depthOct = max::createTexture2D(kOctahedralResolution, kOctahedralResolution, false, 1, max::TextureFormat::D32F, MAX_TEXTURE_RT);

				max::Attachment at[4];
				at[0].init(diffuseOct, max::Access::Write);
				at[1].init(normalOct, max::Access::Write);
				at[2].init(positionOct, max::Access::Write);
				at[3].init(depthOct, max::Access::Write);
				max::FrameBufferHandle octFramebuffer = max::createFrameBuffer(BX_COUNTOF(at), at);

				float proj[16];
				bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, max::getCaps()->homogeneousDepth);

				max::setViewFrameBuffer(m_viewIdOffline, octFramebuffer);
				max::setViewRect(m_viewIdOffline, 0, 0, kOctahedralResolution, kOctahedralResolution);
				max::setViewTransform(m_viewIdOffline, NULL, proj);

				max::setTexture(0, s_diffuse, diffuseCube);
				max::setTexture(1, s_normal, normalCube);
				max::setTexture(2, s_position, positionCube);
				max::setTexture(3, s_depth, depthCube);

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
				max::destroy(depthCube);
				max::destroy(positionCube);
				max::destroy(normalCube);
				max::destroy(diffuseCube);

				max::destroy(octFramebuffer);
				max::destroy(positionOct);
				max::destroy(normalOct);
				max::destroy(diffuseOct);
			}

			m_precomputed = true;
		}
	}

	max::ViewId m_viewId;
	CommonResources* m_common;

	bool m_precomputed;

	max::ProgramHandle m_programProbe; //!< Program thats used to render scene objects to light probe cubemaps.
	max::UniformHandle s_texDiffuse; //!< Diffuse uniform
	max::UniformHandle s_texNormal;  //!< Normal uniform

	max::ProgramHandle m_programOctahedral; //!< Program thats used to convert cubemap to octahedral.
	max::UniformHandle s_diffuse; //!< Diffuse uniform
	max::UniformHandle s_normal; //!< Normal uniform
	max::UniformHandle s_position; //!< Position uniform
	max::UniformHandle s_depth; //!< Depth uniform

	max::ViewId m_viewIdOffline;

	uint32_t m_atlasWidth;
	uint32_t m_atlasHeight;

	max::TextureHandle m_diffuseAtlas;
	max::TextureHandle m_normalAtlas;
	max::TextureHandle m_positionAtlas;
	max::TextureHandle m_depthAtlas;

	max::TextureHandle m_radianceTexture;
	max::FrameBufferHandle m_radianceFramebuffer;

	max::ProgramHandle m_programLightDir;
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
		s_surface		  = max::createUniform("s_surface",			max::UniformType::Sampler);
		s_normal		  = max::createUniform("s_normal",			max::UniformType::Sampler);
		s_depth			  = max::createUniform("s_depth",	        max::UniformType::Sampler);
		s_radianceAtlas   = max::createUniform("s_radianceAtlas",   max::UniformType::Sampler);
		s_irradianceAtlas = max::createUniform("s_irradianceAtlas", max::UniformType::Sampler);
		s_depthAtlas	  = max::createUniform("s_depthAtlas",		max::UniformType::Sampler);

		// Don't create framebuffers and textures until first render call.
		m_framebuffer.idx = max::kInvalidHandle;
	}

	void destroy()
	{
		max::destroy(s_depthAtlas);
		max::destroy(s_irradianceAtlas);
		max::destroy(s_radianceAtlas);
		max::destroy(s_depth);
		max::destroy(s_normal);
		max::destroy(s_surface);
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

		max::setTexture(0, s_surface, max::getTexture(_gbuffer->m_framebuffer, GBuffer::TextureType::Surface));
		max::setTexture(1, s_normal, max::getTexture(_gbuffer->m_framebuffer, GBuffer::TextureType::Normal));
		max::setTexture(2, s_depth, max::getTexture(_gbuffer->m_framebuffer, GBuffer::TextureType::Depth));
		max::setTexture(3, s_radianceAtlas, _gi->m_diffuseAtlas);
		max::setTexture(4, s_irradianceAtlas, _gi->m_diffuseAtlas);
		max::setTexture(5, s_depthAtlas, _gi->m_diffuseAtlas);

		max::setState(0
			| MAX_STATE_WRITE_RGB
			| MAX_STATE_WRITE_A
			//| MAX_STATE_BLEND_ADD // @todo For multiple lights, but currently ruins output buffers.
		);
		screenSpaceQuad(max::getCaps()->originBottomLeft);

		// Directional light.
		max::submit(m_view, m_program);

		// Point lights. @todo
		// for (uint32_t ii = 0; ii < m_numPointLights; ++ii)
		// {
		//		max::submit(m_view, m_lightPointProgram);
		// }
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

	max::UniformHandle s_surface;
	max::UniformHandle s_normal;
	max::UniformHandle s_depth;
	max::UniformHandle s_radianceAtlas;
	max::UniformHandle s_irradianceAtlas;
	max::UniformHandle s_depthAtlas;
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
		s_buffer = max::createUniform("s_buffer", max::UniformType::Sampler);
	}

	void destroy()
	{
		max::destroy(s_buffer);
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
			max::setTexture(0, s_buffer, max::getTexture(_gbuffer->m_framebuffer, 0));
		}
		else if (m_common->m_settings->m_debugbuffer <= RenderSettings::DebugBuffer::Depth)
		{
			max::setTexture(0, s_buffer, max::getTexture(_gbuffer->m_framebuffer, (uint8_t)m_common->m_settings->m_debugbuffer - 1));
		}
		else  if (m_common->m_settings->m_debugbuffer <= RenderSettings::DebugBuffer::Irradiance)
		{
			max::setTexture(0, s_buffer, max::getTexture(_acc->m_framebuffer, 0));
		}
		else  if (m_common->m_settings->m_debugbuffer <= RenderSettings::DebugBuffer::Specular)
		{
			max::setTexture(0, s_buffer, max::getTexture(_acc->m_framebuffer, 1));
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
	max::UniformHandle s_buffer;
};

/// Forward pass.
/// 
struct Forward
{

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
		m_material.create();
		m_probes.create({ -4.5f, 0.5f, -4.5f }, 3, 3, 3, 4.5f);

		// Set common resources.
		m_common.m_settings = _settings;
		m_common.m_uniforms = &m_uniforms;
		m_common.m_material = &m_material;
		m_common.m_probes = &m_probes;
		m_common.m_firstFrame = true;

		// Create all render techniques.
		m_sm.create(&m_common, 0);
		max::setViewName(0, "Direct Light SM");

		m_gbuffer.create(&m_common, 1);
		max::setViewName(1, "Color Passes [GBuffer/Deferred]");

		m_gi.create(&m_common, 2);
		max::setViewName(2, "Global Illumination");

		m_accumulation.create(&m_common, 3);
		max::setViewName(3, "Irradiance & Specular Accumulation");

		m_combine.create(&m_common, 4);
		max::setViewName(4, "Combine");
	}

	void destroy()
	{
		// Destroy all render techniques.
		m_combine.destroy();
		m_accumulation.destroy();
		m_gbuffer.destroy();
		m_sm.destroy();
		m_gi.destroy();

		// Destroy uniforms params.
		m_probes.destroy();
		m_material.destroy();
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
			}

		}, this);
		
		// Uniforms. @todo EWWWWWW, you fucking disgusting lil gnome
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

		// Submit all uniforms params.
		m_uniforms.submitPerFrame();

		// Render all render techniques.
		m_sm.render();
		m_gbuffer.render();
		m_gi.render();
		m_accumulation.render(&m_gbuffer, &m_gi);
		m_combine.render(&m_gbuffer, &m_accumulation);

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
	Material m_material;
	Probes m_probes;

	SM m_sm;
	GBuffer m_gbuffer;
	GI m_gi;
	Accumulation m_accumulation;
	Combine m_combine;
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