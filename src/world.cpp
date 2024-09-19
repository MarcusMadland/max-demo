#include "world.h"
#include "components.h"

#include <bx/readerwriter.h>
#include <bx/file.h>

void World::load(const char* _filepath)
{
	bx::strCopy(m_filepath, 1024, _filepath);

	deserialize();
}

void World::unload()
{
	if (m_entities.size() <= 0)
	{
		return;
	}

	for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
	{
		if (isValid(it->second.m_handle))
		{
			if (RenderComponent* rc = max::getComponent<RenderComponent>(it->second.m_handle))
			{
				max::destroy(rc->m_mesh);
				rc->m_mesh = MAX_INVALID_HANDLE;
			}
			if (MaterialComponent* mc = max::getComponent<MaterialComponent>(it->second.m_handle))
			{
				if (isValid(mc->m_diffuse.m_texture))
				{
					m_textureManager.unload(mc->m_diffuse.m_filepath);
					mc->m_diffuse.m_texture = MAX_INVALID_HANDLE;
				}

				if (isValid(mc->m_normal.m_texture))
				{
					m_textureManager.unload(mc->m_normal.m_filepath);
					mc->m_normal.m_texture = MAX_INVALID_HANDLE;
				}

				if (isValid(mc->m_roughness.m_texture))
				{
					m_textureManager.unload(mc->m_roughness.m_filepath);
					mc->m_roughness.m_texture = MAX_INVALID_HANDLE;
				}

				if (isValid(mc->m_metallic.m_texture))
				{
					m_textureManager.unload(mc->m_metallic.m_filepath);
					mc->m_metallic.m_texture = MAX_INVALID_HANDLE;
				}
			}
			max::destroy(it->second.m_handle);
			it->second.m_handle = MAX_INVALID_HANDLE;
		}
	}

	m_entities.clear();
}

void World::update()
{
	
}

// @todo Right now we serialize all meshes as unique. Use hashes to detect identical meshes and only serialize one.
bool World::serialize()
{
	bx::Error err;
	bx::FileWriter writer;

	bx::FilePath filepath = m_filepath;

	bx::remove(filepath);
	bx::makeAll(filepath.getPath());

	if (bx::open(&writer, filepath, &err))
	{
		// 
		uint32_t numEntities = (uint32_t)m_entities.size();
		if (numEntities <= 0)
		{
			bx::close(&writer);
			return false;
		}

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
				max::MeshQuery* query = max::queryMesh(rc->m_mesh);

				uint32_t num = query->m_num;
				bx::write(&writer, &num, sizeof(uint32_t), &err);
				BX_ASSERT(num <= 1, "Serialization of meshes with multiple groups not supported.")

				max::VertexLayout layout = max::getLayout(rc->m_mesh);
				bx::write(&writer, &layout.m_hash, sizeof(uint32_t), &err);
				bx::write(&writer, &layout.m_stride, sizeof(uint16_t), &err);
				bx::write(&writer, &layout.m_offset, sizeof(uint16_t) * max::Attrib::Count, &err);
				bx::write(&writer, &layout.m_attributes, sizeof(uint16_t) * max::Attrib::Count, &err);

				max::MeshQuery::Data& data = query->m_data[0];

				uint32_t verticesSize = layout.getSize(data.m_numVertices);
				bx::write(&writer, &verticesSize, sizeof(uint32_t), &err);
				bx::write(&writer, data.m_vertices, verticesSize, &err);

				uint32_t indicesSize = data.m_numIndices * sizeof(uint32_t);
				bx::write(&writer, &indicesSize, sizeof(uint32_t), &err);
				bx::write(&writer, data.m_indices, indicesSize, &err);
			}

			// Material Component
			MaterialComponent* mc = max::getComponent<MaterialComponent>(it->second.m_handle);
			bool hasMaterialComponent = mc != NULL;
			bx::write(&writer, &hasMaterialComponent, sizeof(bool), &err);
			if (hasMaterialComponent)
			{
				// Textures
				bool hasColor = bx::strCmp(mc->m_diffuse.m_filepath, "") != bx::kExitSuccess;
				bx::write(&writer, &hasColor, sizeof(bool), &err);
				if (hasColor)
				{
					bx::write(&writer, mc->m_diffuse.m_filepath, 1024, &err);
				}

				bool hasNormal = bx::strCmp(mc->m_normal.m_filepath, "") != bx::kExitSuccess;
				bx::write(&writer, &hasNormal, sizeof(bool), &err);
				if (hasNormal)
				{
					bx::write(&writer, mc->m_normal.m_filepath, 1024, &err);
				}

				bool hasRoughness = bx::strCmp(mc->m_roughness.m_filepath, "") != bx::kExitSuccess;
				bx::write(&writer, &hasRoughness, sizeof(bool), &err);
				if (hasRoughness)
				{
					bx::write(&writer, mc->m_roughness.m_filepath, 1024, &err);
				}

				bool hasMetallic = bx::strCmp(mc->m_metallic.m_filepath, "") != bx::kExitSuccess;
				bx::write(&writer, &hasMetallic, sizeof(bool), &err);
				if (hasMetallic)
				{
					bx::write(&writer, mc->m_metallic.m_filepath, 1024, &err);
				}

				// Factors 
				bx::write(&writer, &mc->m_diffuseFactor, sizeof(float) * 3, &err);
				bx::write(&writer, &mc->m_normalFactor, sizeof(float) * 3, &err);
				bx::write(&writer, &mc->m_roughnessFactor, sizeof(float), &err);
				bx::write(&writer, &mc->m_metallicFactor, sizeof(float), &err);
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

bool World::deserialize()
{
	bx::Error err;
	bx::FileReader reader;

	bx::FilePath filepath = m_filepath;

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
			BX_ASSERT(bx::strCmp(name, "") != bx::kExitSuccess, "Trying to create entity with invalid name.")

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

				max::MeshHandle mesh = max::createMesh(vertices, indices, layout);

				max::addComponent<RenderComponent>(entity, max::createComponent<RenderComponent>({ mesh }));
			}

			// Material Component
			bool hasMaterialCompoenent = false;
			bx::read(&reader, &hasMaterialCompoenent, sizeof(bool), &err);
			if (hasMaterialCompoenent)
			{
				MaterialComponent material;

				// Textures.
				bx::strCopy(material.m_diffuse.m_filepath, 1024, "");
				material.m_diffuse.m_texture = MAX_INVALID_HANDLE;

				bx::strCopy(material.m_normal.m_filepath, 1024, "");
				material.m_normal.m_texture = MAX_INVALID_HANDLE;

				bx::strCopy(material.m_roughness.m_filepath, 1024, "");
				material.m_roughness.m_texture = MAX_INVALID_HANDLE;

				bx::strCopy(material.m_metallic.m_filepath, 1024, "");
				material.m_metallic.m_texture = MAX_INVALID_HANDLE;

				bool hasDiffuse = false;
				bx::read(&reader, &hasDiffuse, sizeof(bool), &err);
				if (hasDiffuse)
				{
					bx::read(&reader, material.m_diffuse.m_filepath, 1024, &err);
					material.m_diffuse.m_texture = m_textureManager.load(material.m_diffuse.m_filepath);
				}

				bool hasNormal = false;
				bx::read(&reader, &hasNormal, sizeof(bool), &err);
				if (hasNormal)
				{
					bx::read(&reader, material.m_normal.m_filepath, 1024, &err);
					material.m_normal.m_texture = m_textureManager.load(material.m_normal.m_filepath);
				}

				bool hasRoughness = false;
				bx::read(&reader, &hasRoughness, sizeof(bool), &err);
				if (hasRoughness)
				{
					bx::read(&reader, material.m_roughness.m_filepath, 1024, &err);
					material.m_roughness.m_texture = m_textureManager.load(material.m_roughness.m_filepath);
				}

				bool hasMetallic = false;
				bx::read(&reader, &hasMetallic, sizeof(bool), &err);
				if (hasMetallic)
				{
					bx::read(&reader, material.m_metallic.m_filepath, 1024, &err);
					material.m_metallic.m_texture = m_textureManager.load(material.m_metallic.m_filepath);
				}

				// Factors. 
				bx::read(&reader, &material.m_diffuseFactor, sizeof(float) * 3, &err);
				bx::read(&reader, &material.m_normalFactor, sizeof(float) * 3, &err);
				bx::read(&reader, &material.m_roughnessFactor, sizeof(float), &err);
				bx::read(&reader, &material.m_metallicFactor, sizeof(float), &err);

				//
				max::addComponent<MaterialComponent>(entity, max::createComponent<MaterialComponent>(material));
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
		BX_TRACE("Scene deserialized from %s", filepath.getCPtr());
		return true;
	}

	BX_TRACE("Failed to open file at path %s", filepath.getCPtr())
	return false;
}