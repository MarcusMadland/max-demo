#include "scene.h"
#include "components.h"

bool MayaBridge::begin()
{
	m_isSynced = false;

	// Init shared buffer.
	if (m_buffer.init("maya-bridge", sizeof(SharedData)))
	{
		m_shared = BX_NEW(max::getAllocator(), SharedData);
		m_shared->m_processed = false;

		m_shared->m_meshChanged.m_changed = false;
		m_shared->m_transformChanged.m_changed = false;

		BX_TRACE("Shared memory initialized successfully.");
		return true;
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
	return true;
}

void MayaBridge::update(std::unordered_map<std::string, EntityHandle>& _entities)
{
	if (!m_buffer.read(m_shared, sizeof(SharedData)))
	{
		m_isSynced = false;
	}

	if (!m_shared->m_processed)
	{
		m_isSynced = false;
	}

	// Update synced.
	SharedData::SyncEvent& syncEvent = m_shared->m_sync;
	m_isSynced = syncEvent.m_isSynced;
	if (m_isSynced)
	{
		max::dbgTextPrintf(0, 0, 0xf, "Connected to Autodesk Maya");
	}
	else
	{
		max::dbgTextPrintf(0, 0, 0xf, "Connecting to Autodesk Maya...");
		return;
	}

	// Update camera.
	SharedData::CameraEvent& cameraEvent = m_shared->m_camera;
	max::setViewTransform(0, cameraEvent.m_view, cameraEvent.m_proj); // @todo

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
				if (RenderComponent* rc = max::getComponent<RenderComponent>(entity))
				{
					max::destroy(rc->m_mesh);
					max::destroy(rc->m_material);
				}
				max::destroy(entity);
				entity = MAX_INVALID_HANDLE;
			}
			else
			{
				// Update entity.
				RenderComponent* rc = max::getComponent<RenderComponent>(entity);
				if (rc != NULL)
				{
					const max::Memory* vertices = max::copy(meshEvent.m_vertices, layout.getSize(meshEvent.m_numVertices));
					const max::Memory* indices = max::copy(meshEvent.m_indices, meshEvent.m_numIndices * sizeof(uint16_t));

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

			const max::Memory* vertices = max::copy(meshEvent.m_vertices, layout.getSize(meshEvent.m_numVertices));
			const max::Memory* indices = max::copy(meshEvent.m_indices, meshEvent.m_numIndices * sizeof(uint16_t));

			max::MaterialHandle whiteMaterial = max::createMaterial(max::loadProgram("vs_cube", "fs_cube"));
			float white[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
			max::addParameter(whiteMaterial, "u_color", white);

			RenderComponent rc;
			rc.m_mesh = max::createMesh(vertices, indices, layout, true);
			rc.m_material = whiteMaterial;
			max::addComponent<RenderComponent>(entity, max::createComponent<RenderComponent>(rc));
		}
	}

	// Update transform.
	SharedData::TransformEvent transformEvent = m_shared->m_transformChanged;
	if (transformEvent.m_changed)
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

void Scene::load()
{
	if (deserialize("scenes/scene.bin") || true)
	{
		// Resources
		max::MeshHandle bunnyMesh = max::loadMesh("meshes/bunny.bin");

		max::ProgramHandle cubeProgram = max::loadProgram("vs_cube", "fs_cube");

		max::MaterialHandle whiteMaterial = max::createMaterial(cubeProgram);
		float white[4] = { 0.9f, 0.9f, 0.9f, 1.0f };
		max::addParameter(whiteMaterial, "u_color", white);

		// Player
#ifdef BX_CONFIG_DEBUG
		m_entities["DebugPlayer"].m_handle = max::createEntity();
		max::addComponent<CameraComponent>(m_entities["DebugPlayer"].m_handle, 
			max::createComponent<CameraComponent>(CameraComponent(0, 45.0f))
		);
#endif
		// Player
		m_entities["Player"].m_handle = max::createEntity();
		max::addComponent<CameraComponent>(m_entities["Player"].m_handle, 
			max::createComponent<CameraComponent>(CameraComponent(1, 45.0f))
		);
		max::addComponent<TransformComponent>(m_entities["Player"].m_handle, 
			max::createComponent<TransformComponent>({ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } })
		);
		max::addComponent<RenderComponent>(m_entities["Player"].m_handle, 
			max::createComponent<RenderComponent>({ bunnyMesh, whiteMaterial })
		);
	}
	else
	{
		BX_TRACE("Failed to load scene binary, doesn't exist.")
	}
}

void Scene::unload()
{
	// Destroy all entities in scene.
	for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
	{
		if (isValid(it->second.m_handle))
		{
			if (RenderComponent* rc = max::getComponent<RenderComponent>(it->second.m_handle))
			{
				max::destroy(rc->m_mesh);
				rc->m_mesh = MAX_INVALID_HANDLE;

				max::destroy(rc->m_material);
				rc->m_material = MAX_INVALID_HANDLE;
			}
			max::destroy(it->second.m_handle);
			it->second.m_handle = MAX_INVALID_HANDLE;
		}
	}
}

void Scene::update()
{
	if (m_mayaBridge != NULL)
	{
		m_mayaBridge->update(m_entities);
	}
}

void Scene::beginMayaBridge()
{
	if (m_entities.size() > 0)
	{
		unload();
	}

	m_mayaBridge = BX_NEW(max::getAllocator(), MayaBridge);
	BX_ASSERT(m_mayaBridge->begin(), "Failed to begin maya bridge, not enough memory.")
}

void Scene::endMayaBridge()
{
	if (m_entities.size() > 0)
	{
		serialize("scenes/scene.bin");
		unload();
		load();
	}

	BX_ASSERT(m_mayaBridge->end(), "Failed to end maya bridge.")
	bx::deleteObject(max::getAllocator(), m_mayaBridge);
	m_mayaBridge = NULL;
}

bool Scene::serialize(const char* _filepath)
{
	/*
	max::System<RenderComponent> render;
	render.each(10, [](max::EntityHandle _entity, void* _userData)
		{
			RenderComponent* comp = max::getComponent<RenderComponent>(_entity);

		});

	max::System<TransformComponent> transform;
	transform.each(10, [](max::EntityHandle _entity, void* _userData)
		{
			TransformComponent* comp = max::getComponent<TransformComponent>(_entity);


		});

	max::System<CameraComponent> camera;
	camera.each(10, [](max::EntityHandle _entity, void* _userData)
		{
			CameraComponent* comp = max::getComponent<CameraComponent>(_entity);

		});
		*/
	return false;
}

bool Scene::deserialize(const char* _filepath)
{
	return false;
}


