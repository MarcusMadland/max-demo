#include "render.h"

#include <max/max.h>

#include "components.h"

void render(RenderSettings* _settings)
{
	max::setViewRect(0, 0, 0, uint16_t(_settings->m_width), uint16_t(_settings->m_height));
	max::setViewClear(0
		, MAX_CLEAR_COLOR | MAX_CLEAR_DEPTH
		, 0x303030ff
		, 1.0f
		, 0
	);

	max::System<CameraComponent> system;
	system.each(10, [](max::EntityHandle _entity, void* _userData)
		{
			RenderSettings* settings = (RenderSettings*)_userData;

			// Update with active camera only.
			CameraComponent* cc = max::getComponent<CameraComponent>(_entity);
			if (cc->m_idx == settings->m_activeCameraIdx)
			{
				max::setViewTransform(0, cc->m_view, cc->m_proj);
			}

		}, _settings);

	max::System<RenderComponent, TransformComponent> render;
	render.each(10, [](max::EntityHandle _entity, void* _userData)
		{
			RenderComponent* rc = max::getComponent<RenderComponent>(_entity);
			TransformComponent* tc = max::getComponent<TransformComponent>(_entity);

			max::MeshQuery* query = max::queryMesh(rc->m_mesh);

			for (uint32_t ii = 0; ii < query->m_num; ++ii)
			{
				float mtx[16];
				bx::mtxSRT(mtx,
					tc->m_scale.x, tc->m_scale.y, tc->m_scale.z,
					tc->m_rotation.x, tc->m_rotation.y, tc->m_rotation.z, tc->m_rotation.w,
					tc->m_position.x, tc->m_position.y, tc->m_position.z);

				max::setTransform(mtx);

				max::MeshQuery::HandleData& handleData = query->m_handleData[ii];
				if (handleData.m_dynamic)
				{
					max::DynamicVertexBufferHandle dvbh = { handleData.m_vertexHandleIdx };
					max::setVertexBuffer(0, dvbh);

					max::DynamicIndexBufferHandle dibh = { handleData.m_indexHandleIdx };
					max::setIndexBuffer(dibh);
				}
				else
				{
					max::VertexBufferHandle vbh = { handleData.m_vertexHandleIdx };
					max::setVertexBuffer(0, vbh);

					max::IndexBufferHandle ibh = { handleData.m_indexHandleIdx };
					max::setIndexBuffer(ibh);
				}

				max::setMaterial(rc->m_material);
				max::setState(0
					| MAX_STATE_WRITE_RGB
					| MAX_STATE_WRITE_A
					| MAX_STATE_WRITE_Z
					| MAX_STATE_DEPTH_TEST_LESS
					| MAX_STATE_MSAA
				);

				max::submit(0, rc->m_material);
			}

		});

	max::frame();
}