#include "maya_bridge.h"

#include <max/max.h>

#include "world.h"
#include "entities.h"
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

void MayaBridge::read(World* _world, Entities* _entities)
{
	// Will always read this data. Maya writes this.
	m_buffer.read(m_shared, sizeof(SharedData));

	uint32_t status = UINT32_MAX;
	m_readbuffer.read(&status, sizeof(uint32_t));

	// Update camera.
	SharedData::CameraUpdate& cameraUpdate = m_shared->m_camera;
	CameraComponent* cc = max::getComponent<CameraComponent>(_entities->m_entities["MayaPlayer"].m_handle);
	bx::memCopy(cc->m_view, cameraUpdate.m_view, sizeof(float) * 16);
	bx::memCopy(cc->m_proj, cameraUpdate.m_proj, sizeof(float) * 16);

	// Update events.
	if (status == SHARED_DATA_MESSAGE_NONE)
	{
		// Update mesh. (Also responsible for creating and destroying entities.)
		SharedData::MeshEvent& meshEvent = m_shared->m_meshChanged;
		if (meshEvent.m_changed)
		{
			max::VertexLayout layout;
			layout.begin()
				.add(max::Attrib::Position, 3, max::AttribType::Float)
				.add(max::Attrib::Normal, 3, max::AttribType::Float)
				.add(max::Attrib::TexCoord0, 2, max::AttribType::Float)
				.end();

			max::EntityHandle& entity = _world->m_entities[meshEvent.m_name].m_handle;
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

					_world->m_entities.erase(meshEvent.m_name);
				}
				else
				{
					// @todo
					// Update entity.
					//RenderComponent* rc = max::getComponent<RenderComponent>(entity);
					//if (rc != NULL)
					//{
					//	const max::Memory* vertices = max::copy(meshEvent.m_vertices, layout.getSize(meshEvent.m_numVertices));
					//	const max::Memory* indices = max::copy(meshEvent.m_indices, meshEvent.m_numIndices * sizeof(uint32_t));
					//
					//	max::update(rc->m_mesh, vertices, indices);
					//}
				}
			}
			else if (meshEvent.m_numVertices != 0 && meshEvent.m_numIndices != 0)
			{
				// Create entity.
				entity = max::createEntity();

				// Transform.
				TransformComponent tc = {
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 1.0f },
					{ 1.0f, 1.0f, 1.0f },
					// @todo Will it always be this? Even when importing meshes?
				};
				max::addComponent<TransformComponent>(entity, max::createComponent<TransformComponent>(tc));

				// Mesh.
				// @todo MakeRef or Copy?
				const max::Memory* vertices = max::copy(&meshEvent.m_vertices[0][0], layout.getSize(meshEvent.m_numVertices));
				const max::Memory* indices = max::copy(&meshEvent.m_indices[0], meshEvent.m_numIndices * sizeof(uint32_t));

				RenderComponent rc = {
					max::createMesh(vertices, indices, layout), true, // @todo Add way to choose cast shadow in maya.
				};
				max::addComponent<RenderComponent>(entity, max::createComponent<RenderComponent>(rc));

				// Material.
				MaterialComponent::Texture emptyTexture;
				bx::strCopy(emptyTexture.m_filepath, 1024, "");
				emptyTexture.m_texture = MAX_INVALID_HANDLE;

				MaterialComponent mc = {
					emptyTexture, { 1.0f, 1.0f, 1.0f }, // Diffuse
					emptyTexture, { 1.0f, 1.0f, 1.0f }, // Normal
					emptyTexture, 1.0f,                 // Roughness
					emptyTexture, 1.0f                  // Metallic
					// @todo Will it always be this? Even when importing meshes?
				};
				max::addComponent<MaterialComponent>(entity, max::createComponent<MaterialComponent>(mc));
			}
		}

		// Update transform.
		SharedData::TransformEvent transformEvent = m_shared->m_transformChanged;
		if (transformEvent.m_changed)
		{
			if (!(_world->m_entities.find(transformEvent.m_name) == _world->m_entities.end()))
			{
				max::EntityHandle& entity = _world->m_entities[transformEvent.m_name].m_handle;
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
			if (!(_world->m_entities.find(materialEvent.m_name) == _world->m_entities.end()))
			{
				max::EntityHandle& entity = _world->m_entities[materialEvent.m_name].m_handle;
				if (isValid(entity))
				{
					// Update entity.
					MaterialComponent* mc = max::getComponent<MaterialComponent>(entity);
					if (mc != NULL)
					{
						// Color
						if (bx::strCmp(mc->m_diffuse.m_filepath, materialEvent.m_diffusePath) != bx::kExitSuccess)
						{
							// Set new paths.
							bx::strCopy(mc->m_diffuse.m_filepath, 1024, materialEvent.m_diffusePath);

							// Destroy current textures if they exist.
							if (isValid(mc->m_diffuse.m_texture))
							{
								max::destroy(mc->m_diffuse.m_texture);
							}

							// Load new textures with new paths.
							mc->m_diffuse.m_texture = _world->m_textureManager.load(mc->m_diffuse.m_filepath);
						}
						else
						{
							mc->m_diffuseFactor[0] = materialEvent.m_diffuse[0];
							mc->m_diffuseFactor[1] = materialEvent.m_diffuse[1];
							mc->m_diffuseFactor[2] = materialEvent.m_diffuse[2];
						}

						// Normal
						if (bx::strCmp(mc->m_normal.m_filepath, materialEvent.m_normalPath) != bx::kExitSuccess)
						{
							// Set new paths.
							bx::strCopy(mc->m_normal.m_filepath, 1024, materialEvent.m_normalPath);

							// Destroy current textures if they exist.
							if (isValid(mc->m_normal.m_texture))
							{
								max::destroy(mc->m_normal.m_texture);
							}

							// Load new textures with new paths.
							mc->m_normal.m_texture = _world->m_textureManager.load(mc->m_normal.m_filepath);
						}

						// Roughness
						if (bx::strCmp(mc->m_roughness.m_filepath, materialEvent.m_roughnessPath) != bx::kExitSuccess)
						{
							// Set new paths.
							bx::strCopy(mc->m_roughness.m_filepath, 1024, materialEvent.m_roughnessPath);

							// Destroy current textures if they exist.
							if (isValid(mc->m_roughness.m_texture))
							{
								max::destroy(mc->m_roughness.m_texture);
							}

							// Load new textures with new paths.
							mc->m_roughness.m_texture = _world->m_textureManager.load(mc->m_roughness.m_filepath);
						}

						// Metallic
						if (bx::strCmp(mc->m_metallic.m_filepath, materialEvent.m_metallicPath) != bx::kExitSuccess)
						{
							// Set new paths.
							bx::strCopy(mc->m_metallic.m_filepath, 1024, materialEvent.m_metallicPath);

							// Destroy current textures if they exist.
							if (isValid(mc->m_metallic.m_texture))
							{
								max::destroy(mc->m_metallic.m_texture);
							}

							// Load new textures with new paths.
							mc->m_metallic.m_texture = _world->m_textureManager.load(mc->m_metallic.m_filepath);
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