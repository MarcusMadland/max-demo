#include "scene.h"
#include "components.h"

#include <bx/readerwriter.h>
#include <bx/file.h>

bool MayaBridge::begin()
{
	// Init shared buffer.
	if (m_buffer.init("maya-bridge", sizeof(SharedData)))
	{
		m_shared = BX_NEW(max::getAllocator(), SharedData);

		if (m_readbuffer.init("maya-bridge-read", sizeof(uint32_t)))
		{
			// Make sure maya ticks first.
			uint32_t status = 1;
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

	// 
	uint32_t status = UINT32_MAX;
	m_readbuffer.read(&status, sizeof(uint32_t)); 
	
	// Update camera.
	SharedData::CameraUpdate& cameraUpdate = m_shared->m_camera;
	max::setViewTransform(0, cameraUpdate.m_view, cameraUpdate.m_proj); // @todo

	// Update events.
	if (status == 0)
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
					if (RenderComponent* rc = max::getComponent<RenderComponent>(entity))
					{
						max::destroy(rc->m_mesh);
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
						const max::Memory* indices = max::copy(meshEvent.m_indices, meshEvent.m_numIndices * sizeof(uint16_t));

						//max::update(rc->m_mesh, vertices, indices);
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

				RenderComponent rc = {
					max::createMesh(vertices, indices, layout),
					{ { 0.8f, 0.8f, 0.8f } },
				};
				max::addComponent<RenderComponent>(entity, max::createComponent<RenderComponent>(rc));
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

		// Update status.
		status = 1;
		m_readbuffer.write(&status, sizeof(uint32_t));
	}
}

void Scene::load()
{
	if (deserialize("scenes/scene.bin") || true)
	{
		// Resources
		max::MeshHandle bunnyMesh = max::loadMesh("meshes/bunny.bin");

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
			max::createComponent<RenderComponent>({ bunnyMesh, {{0.9f, 0.9f, 0.9f}} })
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
			}
			max::destroy(it->second.m_handle);
			it->second.m_handle = MAX_INVALID_HANDLE;
		}
	}
	m_entities.clear();
}

void Scene::update()
{
	if (m_mayaBridge != NULL)
	{
		m_mayaBridge->update(m_entities);
	}

	// Debug print.
	uint32_t idx = 0;
	for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
	{
		max::dbgTextPrintf(0, 1 + idx, 0xf, "[%u]: %s", idx, it->first.c_str());
		++idx;
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
	else
	{
		load();
	}

	BX_ASSERT(m_mayaBridge->end(), "Failed to end maya bridge.")
	bx::deleteObject(max::getAllocator(), m_mayaBridge);
	m_mayaBridge = NULL;
}

bool Scene::serialize(const char* _filepath)
{
	bx::Error err;
	bx::FileWriter writer;

	bx::FilePath filepath = bx::FilePath(bx::Dir::Executable).getPath();
	filepath.join("runtime");
	filepath.join(_filepath);

	if (bx::open(&writer, filepath, &err))
	{
		// 
		uint32_t numEntities = m_entities.size();
		bx::write(&writer, &numEntities, sizeof(uint32_t), &err);

		for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
		{
			// Render Component
			RenderComponent* rc = max::getComponent<RenderComponent>(it->second.m_handle);
			bool hasRenderComponent = rc != NULL;
			bx::write(&writer, &hasRenderComponent, sizeof(bool), &err);
			if (hasRenderComponent)
			{
				//const max::Memory* mem = max::getMeshData(rc->m_mesh);
				//bx::write(&writer, &mem->size, sizeof(uint32_t), &err);
				//bx::write(&writer, &mem->data, sizeof(uint32_t), &err);

				bx::write(&writer, &rc->m_material, sizeof(RenderComponent::Material), &err);
			}

			// Transform Component
			TransformComponent* tc = max::getComponent<TransformComponent>(it->second.m_handle);
			bool hasTransformComponent = tc != NULL;
			bx::write(&writer, &hasTransformComponent, sizeof(bool), &err);
			if (hasTransformComponent)
			{
				bx::write(&writer, &tc->m_position, sizeof(bx::Vec3), &err);
				bx::write(&writer, &tc->m_rotation, sizeof(bx::Quaternion), &err);
				bx::write(&writer, &tc->m_scale, sizeof(bx::Vec3), &err);
			}
			
		}

		bx::close(&writer);
		BX_TRACE("Scene serialized to %s", filepath.getCPtr());
		return true;
	}
		
	BX_TRACE("Failed to open file at path %s", filepath.getCPtr())
	return false;
}

bool Scene::deserialize(const char* _filepath)
{
	bx::Error err;
	bx::FileReader reader;

	bx::FilePath filepath = bx::FilePath(bx::Dir::Executable).getPath();
	filepath.join("runtime");
	filepath.join(_filepath);

	if (bx::open(&reader, _filepath, &err))
	{
		// 
		uint32_t numEntities = 0;
		bx::read(&reader, &numEntities, sizeof(uint32_t), &err);

		for (uint32_t ii = 0; ii < numEntities; ++ii)
		{
			max::EntityHandle entity = max::createEntity();

			// Render Component
			bool hasRenderComponent = false;
			bx::read(&reader, &hasRenderComponent, sizeof(bool), &err);
			if (hasRenderComponent)
			{
				uint32_t size = 0;
				bx::read(&reader, &size, sizeof(uint32_t), &err);
				const max::Memory* mem = max::alloc(size);
				bx::read(&reader, mem->data, size, &err);

				RenderComponent::Material material = { {0.0f, 0.0f, 0.0f} };
				bx::read(&reader, &material, sizeof(RenderComponent::Material), &err);

				max::addComponent<RenderComponent>(entity, max::createComponent<RenderComponent>({
					max::createMesh(mem),
					material
				}));
			}
			
			// Transform Component
			bool hasTransformComponent = false;
			bx::read(&reader, &hasTransformComponent, sizeof(bool), &err);
			if (hasTransformComponent)
			{
				bx::Vec3 pos = { 0.0f, 0.0f, 0.0f };
				bx::Quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
				bx::Vec3 scale = { 0.0f, 0.0f, 0.0f };

				bx::read(&reader, &pos, sizeof(bx::Vec3), &err);
				bx::read(&reader, &rotation, sizeof(bx::Quaternion), &err);
				bx::read(&reader, &scale, sizeof(bx::Vec3), &err);

				max::addComponent<TransformComponent>(entity, max::createComponent<TransformComponent>({
					pos,
					rotation,
					scale
				}));
			}

		}

		bx::close(&reader);
		BX_TRACE("Scene deserialized from %s", _filepath);
		return true;
	}

	BX_TRACE("Failed to open file at path %s", _filepath)
	return false;
}


