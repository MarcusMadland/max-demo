#include "maya_bridge.h"

#include "components.h"

bool MayaBridge::begin()
{
	// Init shared buffer.
	if (m_buffer.init("maya-bridge", sizeof(SharedData)))
	{
		m_shared = BX_NEW(max::getAllocator(), SharedData);

		if (m_readbuffer.init("maya-bridge-read", sizeof(uint32_t)))
		{
			// Make sure maya ticks first.
			uint32_t status = SHARED_DATA_MESSAGE_RECEIVED | SHARED_DATA_MESSAGE_RELOAD_SCENE;
			m_readbuffer.write(&status, sizeof(uint32_t));

			BX_TRACE("Shared memory initialized successfully.");
			return true;
		}
	}

	BX_TRACE("Failed to initialize shared memory.");
	return false;
}

bool MayaBridge::end()
{
	// Shutdown shared buffer.
	bx::deleteObject(max::getAllocator(), m_shared);
	m_shared = NULL;

	m_buffer.shutdown();
	m_readbuffer.shutdown();
	return true;
}

void MayaBridge::update(std::unordered_map<std::string, EntityHandle>& _entities)
{
	// Will always read this data. Maya writes this.
	m_buffer.read(m_shared, sizeof(SharedData));

	uint32_t status = UINT32_MAX;
	m_readbuffer.read(&status, sizeof(uint32_t));

	// Update camera.
	SharedData::CameraUpdate& cameraUpdate = m_shared->m_camera;
	CameraComponent* cc = max::getComponent<CameraComponent>(_entities["DebugPlayer"].m_handle);
	bx::memCopy(cc->m_view, cameraUpdate.m_view, sizeof(float) * 16);
	bx::memCopy(cc->m_proj, cameraUpdate.m_proj, sizeof(float) * 16);

	// Update events.
	if (status == SHARED_DATA_MESSAGE_NONE)
	{
		// Update mesh.
		SharedData::MeshEvent& meshEvent = m_shared->m_meshChanged;
		if (meshEvent.m_changed)
		{
			max::VertexLayout layout;
			layout.begin()
				.add(max::Attrib::Position, 3, max::AttribType::Float)
				.add(max::Attrib::Normal, 3, max::AttribType::Float)
				.add(max::Attrib::TexCoord0, 2, max::AttribType::Float)
				.end();

			max::EntityHandle& entity = _entities[meshEvent.m_name].m_handle;
			if (isValid(entity))
			{
				if (meshEvent.m_numVertices == 0 && meshEvent.m_numIndices == 0)
				{
					// Destroy entity.
					if (RenderComponent* drc = max::getComponent<RenderComponent>(entity))
					{
						max::destroy(drc->m_mesh);
					}
					max::destroy(entity);
					entity = MAX_INVALID_HANDLE;

					_entities.erase(meshEvent.m_name);
				}
				else
				{
					// Update entity.
					RenderComponent* rc = max::getComponent<RenderComponent>(entity);
					if (rc != NULL)
					{
						const max::Memory* vertices = max::copy(meshEvent.m_vertices, layout.getSize(meshEvent.m_numVertices));
						const max::Memory* indices = max::copy(meshEvent.m_indices, meshEvent.m_numIndices * sizeof(uint32_t));

						max::update(rc->m_mesh, vertices, indices);
					}
				}
			}
			else if (meshEvent.m_numVertices != 0 && meshEvent.m_numIndices != 0)
			{
				// Create entity.
				entity = max::createEntity();

				TransformComponent tc = {
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 1.0f },
					{ 1.0f, 1.0f, 1.0f },
					// @todo Will it always be this? Even when importing meshes?
				};
				max::addComponent<TransformComponent>(entity, max::createComponent<TransformComponent>(tc));

				const max::Memory* vertices = max::makeRef(&meshEvent.m_vertices[0][0], layout.getSize(meshEvent.m_numVertices));
				const max::Memory* indices = max::makeRef(&meshEvent.m_indices[0], meshEvent.m_numIndices * sizeof(uint32_t));

				Material material;
				material.m_color = MAX_INVALID_HANDLE;
				bx::strCopy(material.m_colorPath, 1024, "");
				material.m_normal = MAX_INVALID_HANDLE;
				bx::strCopy(material.m_normalPath, 1024, "");

				RenderComponent drc = {
					max::createDynamicMesh(vertices, indices, layout), material,
				};
				max::addComponent<RenderComponent>(entity, max::createComponent<RenderComponent>(drc));
			}
		}

		// Update transform.
		SharedData::TransformEvent transformEvent = m_shared->m_transformChanged;
		if (transformEvent.m_changed)
		{
			if (!(_entities.find(transformEvent.m_name) == _entities.end()))
			{
				max::EntityHandle& entity = _entities[transformEvent.m_name].m_handle;
				if (isValid(entity))
				{
					// Update entity.
					TransformComponent* tc = max::getComponent<TransformComponent>(entity);
					if (tc != NULL)
					{
						tc->m_position = {
							transformEvent.m_pos[0],
							transformEvent.m_pos[1],
							transformEvent.m_pos[2]
						};
						tc->m_rotation = {
							transformEvent.m_rotation[0],
							transformEvent.m_rotation[1],
							transformEvent.m_rotation[2],
							transformEvent.m_rotation[3]
						};
						tc->m_scale = {
							transformEvent.m_scale[0],
							transformEvent.m_scale[1],
							transformEvent.m_scale[2]
						};
					}
				}
			}
		}

		// Update material.
		SharedData::MaterialEvent materialEvent = m_shared->m_materialChanged;
		if (materialEvent.m_changed)
		{
			if (!(_entities.find(materialEvent.m_name) == _entities.end()))
			{
				max::EntityHandle& entity = _entities[materialEvent.m_name].m_handle;
				if (isValid(entity))
				{
					// Update entity.
					RenderComponent* drc = max::getComponent<RenderComponent>(entity);
					if (drc != NULL)
					{
						if (bx::strCmp(drc->m_material.m_colorPath, materialEvent.m_colorPath) != bx::kExitSuccess)
						{
							// Set new paths.
							bx::strCopy(drc->m_material.m_colorPath, 1024, materialEvent.m_colorPath);
							bx::strCopy(drc->m_material.m_normalPath, 1024, materialEvent.m_colorPath);

							// Destroy current textures if they exist.
							if (isValid(drc->m_material.m_color))
							{
								max::destroy(drc->m_material.m_color);
							}
							if (isValid(drc->m_material.m_normal))
							{
								max::destroy(drc->m_material.m_normal);
							}

							// Load new textures with new paths.
							drc->m_material.m_color = max::loadTexture(drc->m_material.m_colorPath);
							drc->m_material.m_color = max::loadTexture(drc->m_material.m_normalPath);
						}
					}
				}
			}
		}

		// Update status.
		status = SHARED_DATA_MESSAGE_RECEIVED;
		m_readbuffer.write(&status, sizeof(uint32_t));
	}
}