#include "render.h"

#include <max/max.h>

#include "components.h"

struct PosTexCoord0Vertex
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

max::VertexLayout PosTexCoord0Vertex::ms_layout;

void screenSpaceQuad(bool _originBottomLeft, float _width = 1.0f, float _height = 1.0f)
{
	if (3 == max::getAvailTransientVertexBuffer(3, PosTexCoord0Vertex::ms_layout))
	{
		max::TransientVertexBuffer vb;
		max::allocTransientVertexBuffer(&vb, 3, PosTexCoord0Vertex::ms_layout);
		PosTexCoord0Vertex* vertex = (PosTexCoord0Vertex*)vb.data;

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

struct CommonResources
{
	bool m_firstFrame; 

	uint16_t m_width;
	uint16_t m_height;

	float m_view[16];
	float m_proj[16];
};

struct GBuffer
{
	enum TextureType
	{
		Color,
		Normal,
		Depth,

		Count
	};

	void create(max::ViewId _view)
	{
		m_view = _view;

		m_program = max::loadProgram("vs_gbuffer", "fs_gbuffer");
		s_texColor = max::createUniform("s_texColor", max::UniformType::Sampler);
		s_texNormal = max::createUniform("s_texNormal", max::UniformType::Sampler);

		// Don't create framebuffers and textures until first render call.
		m_gbuffer[0].idx  = max::kInvalidHandle;
		m_gbuffer[1].idx  = max::kInvalidHandle;
		m_gbuffer[2].idx  = max::kInvalidHandle;
		m_framebuffer.idx = max::kInvalidHandle;
	}

	void destroy()
	{
		max::destroy(s_texNormal);
		max::destroy(s_texColor);
		max::destroy(m_program);

		destroyFramebuffer();
		destroyTextures();
	}

	void render(CommonResources* _common)
	{
		if (_common->m_firstFrame)
		{
			destroyFramebuffer();
			destroyTextures();

			createTextures(_common->m_width, _common->m_height);
			createFramebuffer();
		}

		max::setViewFrameBuffer(m_view, m_framebuffer);
		max::setViewRect(m_view, 0, 0, _common->m_width, _common->m_height);
		max::setViewClear(m_view, MAX_CLEAR_COLOR | MAX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		max::setViewTransform(m_view, _common->m_view, _common->m_proj);
		max::touch(m_view);

		max::System<TransformComponent> system;
		system.each(10, [](max::EntityHandle _entity, void* _userData)
			{
				GBuffer* gbuffer = (GBuffer*)_userData;

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

						if (isValid(rc->m_material.m_color))
						{
							max::setTexture(0, gbuffer->s_texColor, rc->m_material.m_color);
						}

						if (isValid(rc->m_material.m_normal))
						{
							max::setTexture(1, gbuffer->s_texNormal, rc->m_material.m_normal);
						}

						max::setState(0
							| MAX_STATE_WRITE_RGB
							| MAX_STATE_WRITE_A
							| MAX_STATE_WRITE_Z
							| MAX_STATE_DEPTH_TEST_LESS
							//| MAX_STATE_CULL_CW
							| MAX_STATE_MSAA
						);

						max::submit(gbuffer->m_view, gbuffer->m_program);
					}
				}

			}, this);
	}

	void createTextures(uint16_t _width, uint16_t _height)
	{
		m_gbuffer[TextureType::Color]  = max::createTexture2D(_width, _height, false, 1, max::TextureFormat::RGBA8,   MAX_TEXTURE_RT);
		m_gbuffer[TextureType::Normal] = max::createTexture2D(_width, _height, false, 1, max::TextureFormat::RGBA16F, MAX_TEXTURE_RT);
		m_gbuffer[TextureType::Depth]  = max::createTexture2D(_width, _height, false, 1, max::TextureFormat::D32F, MAX_TEXTURE_RT);
	}

	void destroyTextures()
	{
		for (uint32_t ii = 0; ii < TextureType::Count; ++ii)
		{
			if (isValid(m_gbuffer[ii]))
			{
				max::destroy(m_gbuffer[ii]);
			}
		}
	}

	void createFramebuffer()
	{
		max::Attachment attachments[TextureType::Count];
		attachments[TextureType::Color].init(m_gbuffer[TextureType::Color]);
		attachments[TextureType::Normal].init(m_gbuffer[TextureType::Normal]);
		attachments[TextureType::Depth].init(m_gbuffer[TextureType::Depth]);

		m_framebuffer = max::createFrameBuffer(BX_COUNTOF(attachments), attachments, true);
	}

	void destroyFramebuffer()
	{
		if (isValid(m_framebuffer))
		{
			max::destroy(m_framebuffer);
		}
	}

	max::ViewId m_view;

	max::ProgramHandle m_program;
	max::UniformHandle s_texColor;
	max::UniformHandle s_texNormal;

	max::FrameBufferHandle m_framebuffer;
	max::TextureHandle m_gbuffer[TextureType::Count];
};

struct RayTracedShadows
{

};

struct DeferredShading
{
	void create(max::ViewId _view)
	{
		m_view = _view;

		m_program = max::loadProgram("vs_deferred", "fs_deferred");
		s_albedo = max::createUniform("s_albedo", max::UniformType::Sampler);
	}

	void destroy()
	{
		max::destroy(s_albedo);
		max::destroy(m_program);
	}

	void render(CommonResources* _common, GBuffer* _gbuffer)
	{
		const max::Caps* caps = max::getCaps();

		float proj[16];
		bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, caps->homogeneousDepth);

		max::setViewFrameBuffer(m_view, MAX_INVALID_HANDLE);
		max::setViewRect(m_view, 0, 0, _common->m_width, _common->m_height);
		max::setViewTransform(m_view, NULL, proj);
		max::touch(m_view);

		max::setTexture(0, s_albedo, max::getTexture(_gbuffer->m_framebuffer, 0));
		max::setState(0
			| MAX_STATE_WRITE_RGB
			| MAX_STATE_WRITE_A
		);

		screenSpaceQuad(caps->originBottomLeft);

		max::submit(m_view, m_program);
	}

	max::ViewId m_view;

	max::ProgramHandle m_program;
	max::UniformHandle s_albedo;
};

constexpr max::ViewId kRenderPassGBuffer = 0;
constexpr max::ViewId kRenderPassDeferred = 1;

struct RenderSystem
{
	void create(RenderSettings* _settings)
	{
		m_common.m_firstFrame = true;

		PosTexCoord0Vertex::init();

		m_gbuffer.create(kRenderPassGBuffer);
		m_deferredShading.create(kRenderPassDeferred);
	}

	void destroy()
	{
		m_deferredShading.destroy();
		m_gbuffer.destroy();
	}

	void update(RenderSettings* _settings)
	{
		m_settings = _settings;

		m_common.m_width = _settings->m_width;
		m_common.m_height = _settings->m_height;

		max::System<CameraComponent> camera;
		camera.each(10, [](max::EntityHandle _entity, void* _userData)
			{
				RenderSystem* system = (RenderSystem*)_userData;

				CameraComponent* cc = max::getComponent<CameraComponent>(_entity);
				if (cc->m_idx == system->m_settings->m_activeCameraIdx)
				{
					bx::memCopy(system->m_common.m_view, cc->m_view, sizeof(float) * 16);
					bx::memCopy(system->m_common.m_proj, cc->m_proj, sizeof(float) * 16);
				}

			}, this);

		m_gbuffer.render(&m_common);
		m_deferredShading.render(&m_common, &m_gbuffer);

		max::frame();

		m_common.m_firstFrame = false;
	}

	RenderSettings* m_settings;

	CommonResources m_common;

	GBuffer m_gbuffer;
	DeferredShading m_deferredShading;
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

void renderUpdate(RenderSettings* _settings)
{
	s_ctx->update(_settings);
}
