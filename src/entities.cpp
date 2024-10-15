#include "entities.h"
#include "components.h"

#include <bx/readerwriter.h>
#include <bx/file.h>
#include <bx/pixelformat.h>

void createCube(std::unordered_map<std::string, EntityHandle>& _entities, const char* _name, const TransformComponent& _transform, max::MeshHandle _cube, const bx::Vec3 _color)
{
	RenderComponent rc;
	rc.m_mesh = _cube;

	MaterialComponent mc;
	bx::strCopy(mc.m_diffuse.m_filepath, 1024, "");
	mc.m_diffuse.m_texture = MAX_INVALID_HANDLE;

	mc.m_diffuseFactor[0] = _color.x;
	mc.m_diffuseFactor[1] = _color.y;
	mc.m_diffuseFactor[2] = _color.z;

	bx::strCopy(mc.m_normal.m_filepath, 1024, "");
	mc.m_normal.m_texture = MAX_INVALID_HANDLE;

	mc.m_normalFactor[0] = 1.0f;
	mc.m_normalFactor[1] = 1.0f;
	mc.m_normalFactor[2] = 1.0f;

	bx::strCopy(mc.m_metallic.m_filepath, 1024, "");
	mc.m_metallic.m_texture = MAX_INVALID_HANDLE;

	mc.m_metallicFactor = 1.0f;

	bx::strCopy(mc.m_roughness.m_filepath, 1024, "");
	mc.m_roughness.m_texture = MAX_INVALID_HANDLE;

	mc.m_roughnessFactor = 1.0f;

	_entities[_name].m_handle = max::createEntity();
	max::addComponent<TransformComponent>(_entities[_name].m_handle,
		max::createComponent<TransformComponent>(_transform)
	);
	max::addComponent<RenderComponent>(_entities[_name].m_handle,
		max::createComponent<RenderComponent>(rc)
	);
	max::addComponent<MaterialComponent>(_entities[_name].m_handle,
		max::createComponent<MaterialComponent>(mc)
	);
}

void Entities::load()
{
	// Resources
	max::MeshHandle cube = max::loadMesh("meshes/cube.bin", true); // THIS DOESNT HAVE TANGENTS

	// Player (Maya)
	m_entities["MayaPlayer"].m_handle = max::createEntity();
	max::addComponent<CameraComponent>(m_entities["MayaPlayer"].m_handle,
		max::createComponent<CameraComponent>(CameraComponent(0, 45.0f))
	);

	// Player
	CameraComponent cc = CameraComponent(1, 45.0f);
	cc.m_position = { 15.0f, 5.f, 0.0 };
	m_entities["Player"].m_handle = max::createEntity();
	max::addComponent<CameraComponent>(m_entities["Player"].m_handle,
		max::createComponent<CameraComponent>(cc)
	);
	
	createCube(m_entities, 
		"Floor", 
		{ {0.0f, -0.25, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {5.0f, 0.5f, 5.0f} },
		cube,
		{1.0f, 1.0f, 1.0f});

	createCube(m_entities,
		"Right",
		{ {0.0f, 5.0f, -5.5f}, {0.0f, 0.0f, 0.0f, 1.0f}, {5.0f, 4.75f, 0.5f} },
		cube,
		{ 0.0f, 1.0f, 0.0f });

	createCube(m_entities,
		"Left",
		{ {0.0f, 5.0f, 5.5f}, {0.0f, 0.0f, 0.0f, 1.0f}, {5.0f, 4.75f, 0.5f} },
		cube,
		{ 1.0f, 0.0f, 0.0f });

	createCube(m_entities,
		"Left Cube",
		{ {0.5f, 2.25f, 2.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.5f, 3.0f, 1.5f} },
		cube,
		{ 1.0f, 1.0f, 1.0f });

	createCube(m_entities,
		"Right Cube",
		{ {0.5f, 0.75f, -2.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.5f, 1.5f, 1.5f} },
		cube,
		{ 1.0f, 1.0f, 1.0f });

	// Sun
	m_entities["Sky"].m_handle = max::createEntity();
	max::addComponent<SkyComponent>(m_entities["Sky"].m_handle,
		max::createComponent<SkyComponent>()
	);
}

void Entities::unload()
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
			if (MaterialComponent* rc = max::getComponent<MaterialComponent>(it->second.m_handle))
			{
				if (isValid(rc->m_diffuse.m_texture))
				{
					max::destroy(rc->m_diffuse.m_texture);
					rc->m_diffuse.m_texture = MAX_INVALID_HANDLE;
				}

				if (isValid(rc->m_normal.m_texture))
				{
					max::destroy(rc->m_normal.m_texture);
					rc->m_normal.m_texture = MAX_INVALID_HANDLE;
				}

				if (isValid(rc->m_roughness.m_texture))
				{
					max::destroy(rc->m_roughness.m_texture);
					rc->m_roughness.m_texture = MAX_INVALID_HANDLE;
				}

				if (isValid(rc->m_metallic.m_texture))
				{
					max::destroy(rc->m_metallic.m_texture);
					rc->m_metallic.m_texture = MAX_INVALID_HANDLE;
				}
			}
			max::destroy(it->second.m_handle);
			it->second.m_handle = MAX_INVALID_HANDLE;
		}
	}

	m_entities.clear();
}

void Entities::update()
{
}