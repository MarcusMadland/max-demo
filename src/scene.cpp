#include "scene.h"
#include "components.h"

#include <bx/readerwriter.h>
#include <bx/file.h>

void Scene::load()
{
	if (deserialize("scenes/scene.bin") || true)
	{
		// Resources
		max::DynamicMeshHandle mesh = max::loadDynamicMesh("meshes/orb.bin");

		Material material;
		bx::strCopy(material.m_colorPath, 1024, "textures/fieldstone-rgba.dds");
		material.m_color = max::loadTexture(material.m_colorPath);
		bx::strCopy(material.m_normalPath, 1024, "textures/fieldstone-n.dds");
		material.m_normal = max::loadTexture(material.m_normalPath);

		// Player
		m_entities["Player"].m_handle = max::createEntity();
		max::addComponent<CameraComponent>(m_entities["Player"].m_handle, 
			max::createComponent<CameraComponent>(CameraComponent(1, 45.0f))
		);
		max::addComponent<TransformComponent>(m_entities["Player"].m_handle, 
			max::createComponent<TransformComponent>({ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } })
		);
		max::addComponent<RenderComponent>(m_entities["Player"].m_handle, 
			max::createComponent<RenderComponent>({ mesh, material })
		);
	}
	else
	{
		BX_TRACE("Failed to load scene binary, doesn't exist.")
	}
}

void Scene::unload()
{
	if (m_entities.size() <= 0)
	{
		return;
	}

	// Destroy all entities in scene.
	for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
	{
		if (isValid(it->second.m_handle))
		{
			if (RenderComponent* rc = max::getComponent<RenderComponent>(it->second.m_handle))
			{
				max::destroy(rc->m_mesh);
				rc->m_mesh = MAX_INVALID_HANDLE;

				if (isValid(rc->m_material.m_color))
				{
					max::destroy(rc->m_material.m_color);
					rc->m_material.m_color = MAX_INVALID_HANDLE;
				}

				if (isValid(rc->m_material.m_normal))
				{
					max::destroy(rc->m_material.m_normal);
					rc->m_material.m_normal = MAX_INVALID_HANDLE;
				}
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

	// Debug print scene.
	uint32_t idx = 0;
	for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
	{
		max::dbgTextPrintf(0, 1 + idx, 0xf, "[%u]: %s", idx, it->first.c_str());
		++idx;
	}
}

void Scene::beginMayaBridge()
{
	// Debug Player
#ifdef BX_CONFIG_DEBUG
	m_entities["DebugPlayer"].m_handle = max::createEntity();
	max::addComponent<CameraComponent>(m_entities["DebugPlayer"].m_handle,
		max::createComponent<CameraComponent>(CameraComponent(0, 45.0f))
	);
#endif

	m_mayaBridge = BX_NEW(max::getAllocator(), MayaBridge);
	BX_ASSERT(m_mayaBridge->begin(), "Failed to begin maya bridge, not enough memory.")
}

void Scene::endMayaBridge()
{
	BX_ASSERT(m_mayaBridge->end(), "Failed to end maya bridge.")
	bx::deleteObject(max::getAllocator(), m_mayaBridge);
	m_mayaBridge = NULL;
}

bool Scene::serialize(const char* _filepath)
{
	bx::Error err;
	bx::FileWriter writer;

	bx::FilePath filepath = _filepath;

	bx::remove(filepath);
	bx::makeAll(filepath.getPath());

	if (bx::open(&writer, filepath, &err))
	{
		// 
		uint32_t numEntities = (uint32_t)m_entities.size();
		bx::write(&writer, &numEntities, sizeof(uint32_t), &err);

		for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
		{
			char name[1024];
			bx::strCopy(name, 1024, it->first.c_str());
			bx::write(&writer, &name, 1024, &err);

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
			
			// Render Component
			RenderComponent* rc = max::getComponent<RenderComponent>(it->second.m_handle);
			bool hasRenderComponent = rc != NULL;
			bx::write(&writer, &hasRenderComponent, sizeof(bool), &err);
			if (hasRenderComponent)
			{
				// Mesh
				max::DynamicMeshQuery* query = max::queryDynamicMesh(rc->m_mesh);

				uint32_t num = query->m_num;
				bx::write(&writer, &num, sizeof(uint32_t), &err);
				BX_ASSERT(num <= 1, "Serialization of meshes with multiple groups not supported.")

				max::VertexLayout layout = max::getLayout(rc->m_mesh);
				bx::write(&writer, &layout.m_hash, sizeof(uint32_t), &err);
				bx::write(&writer, &layout.m_stride, sizeof(uint16_t), &err);
				bx::write(&writer, &layout.m_offset, sizeof(uint16_t) * max::Attrib::Count, &err);
				bx::write(&writer, &layout.m_attributes, sizeof(uint16_t) * max::Attrib::Count, &err);

				max::DynamicMeshQuery::Data& data = query->m_data[0];

				uint32_t verticesSize = layout.getSize(data.m_numVertices);
				bx::write(&writer, &verticesSize, sizeof(uint32_t), &err);
				bx::write(&writer, data.m_vertices, verticesSize, &err);

				uint32_t indicesSize = data.m_numIndices * sizeof(uint16_t);
				bx::write(&writer, &indicesSize, sizeof(uint32_t), &err);
				bx::write(&writer, data.m_indices, indicesSize, &err);

				// Material
				bx::write(&writer, rc->m_material.m_colorPath, 1024, &err);
				bx::write(&writer, rc->m_material.m_normalPath, 1024, &err);
			}
		}

		if (!err.isOk())
		{
			BX_TRACE("Failed to serialize file to path %s", filepath.getCPtr())
			return false;
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

	bx::FilePath filepath = _filepath;

	if (bx::open(&reader, filepath, &err))
	{
		// 
		uint32_t numEntities = 0;
		bx::read(&reader, &numEntities, sizeof(uint32_t), &err);

		for (uint32_t ii = 0; ii < numEntities; ++ii)
		{
			max::EntityHandle entity = max::createEntity();

			char name[1024];
			bx::read(&reader, &name, 1024, &err);

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

			// Render Component
			bool hasRenderComponent = false;
			bx::read(&reader, &hasRenderComponent, sizeof(bool), &err);
			if (hasRenderComponent)
			{
				// Mesh
				uint32_t num = 0;
				bx::read(&reader, &num, sizeof(uint32_t), &err);
				BX_ASSERT(num <= 1, "Deserialization of meshes with multiple groups not supported.")

				max::VertexLayout layout;
				bx::read(&reader, &layout.m_hash, sizeof(uint32_t), &err);
				bx::read(&reader, &layout.m_stride, sizeof(uint16_t), &err);
				bx::read(&reader, &layout.m_offset, sizeof(uint16_t) * max::Attrib::Count, &err);
				bx::read(&reader, &layout.m_attributes, sizeof(uint16_t) * max::Attrib::Count, &err);

				// @todo Something is wrong with how data is used to create meshes, we getting leaks
				uint32_t verticesSize = 0;
				bx::read(&reader, &verticesSize, sizeof(uint32_t), &err);
				const max::Memory* vertices = max::alloc(verticesSize);
				bx::read(&reader, vertices->data, verticesSize, &err);

				uint32_t indicesSize = 0;
				bx::read(&reader, &indicesSize, sizeof(uint32_t), &err);
				const max::Memory* indices = max::alloc(indicesSize);
				bx::read(&reader, indices->data, indicesSize, &err);

				max::DynamicMeshHandle mesh = max::createDynamicMesh(vertices, indices, layout);

				// Material
				Material material;

				bx::read(&reader, material.m_colorPath, 1024, &err);
				if (bx::strCmp(material.m_colorPath, "") != bx::kExitSuccess)
				{
					material.m_color = max::loadTexture(material.m_colorPath);
				}

				bx::read(&reader, material.m_normalPath, 1024, &err);
				if (bx::strCmp(material.m_normalPath, "") != bx::kExitSuccess)
				{
					material.m_normal = max::loadTexture(material.m_normalPath);
				}

				//
				max::addComponent<RenderComponent>(entity, max::createComponent<RenderComponent>({ mesh, material }));
			}

			EntityHandle entityHandle;
			entityHandle.m_handle = entity;
			m_entities[name] = entityHandle;
		}
		
		if (!err.isOk() || m_entities.size() <= 0)
		{
			BX_TRACE("Failed to deserialize file at path %s", filepath.getCPtr())
			return false;
		}

		bx::close(&reader);
		BX_TRACE("Scene deserialized from %s", _filepath);
		return true;
	}

	BX_TRACE("Failed to open file at path %s", _filepath)
	return false;
}