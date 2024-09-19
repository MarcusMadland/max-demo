#include "entities.h"
#include "components.h"

#include <bx/readerwriter.h>
#include <bx/file.h>

void Entities::load()
{
	// Resources
	RenderComponent render;
	render.m_mesh = max::loadMesh("meshes/bunny.bin", true);
	render.m_castShadows = true;

	MaterialComponent material1;
	
	bx::strCopy(material1.m_diffuse.m_filepath, 1024, "textures/test_albedo.jpg");
	material1.m_diffuse.m_texture = max::loadTexture(material1.m_diffuse.m_filepath);
	material1.m_diffuseFactor[0] = 1.0f;
	material1.m_diffuseFactor[1] = 1.0f;
	material1.m_diffuseFactor[2] = 1.0f;

	bx::strCopy(material1.m_normal.m_filepath, 1024, "textures/test_normal.jpg");
	material1.m_normal.m_texture = max::loadTexture(material1.m_normal.m_filepath);
	material1.m_normalFactor[0] = 1.0f;
	material1.m_normalFactor[1] = 1.0f;
	material1.m_normalFactor[2] = 1.0f;
	
	bx::strCopy(material1.m_roughness.m_filepath, 1024, "textures/test_roughness.jpg");
	material1.m_roughness.m_texture = max::loadTexture(material1.m_roughness.m_filepath);
	material1.m_roughnessFactor = 1.0f;
	
	bx::strCopy(material1.m_metallic.m_filepath, 1024, "textures/test_metallic.jpg");
	material1.m_metallic.m_texture = max::loadTexture(material1.m_metallic.m_filepath);
	material1.m_metallicFactor = 1.0f;

	MaterialComponent material2;
	bx::strCopy(material2.m_diffuse.m_filepath, 1024, "textures/test2_albedo.jpg");
	material2.m_diffuse.m_texture = max::loadTexture(material2.m_diffuse.m_filepath);
	material2.m_diffuseFactor[0] = 1.0f;
	material2.m_diffuseFactor[1] = 1.0f;
	material2.m_diffuseFactor[2] = 1.0f;

	bx::strCopy(material2.m_normal.m_filepath, 1024, "textures/test2_normal.jpg");
	material2.m_normal.m_texture = max::loadTexture(material2.m_normal.m_filepath);
	material2.m_normalFactor[0] = 1.0f;
	material2.m_normalFactor[1] = 1.0f;
	material2.m_normalFactor[2] = 1.0f;

	bx::strCopy(material2.m_roughness.m_filepath, 1024, "textures/test2_roughness.jpg");
	material2.m_roughness.m_texture = max::loadTexture(material2.m_roughness.m_filepath);
	material2.m_roughnessFactor = 1.0f;

	bx::strCopy(material2.m_metallic.m_filepath, 1024, "");
	material2.m_metallic.m_texture = MAX_INVALID_HANDLE;
	material2.m_metallicFactor = 1.0f;

	// Player (Maya)
	m_entities["MayaPlayer"].m_handle = max::createEntity();
	max::addComponent<CameraComponent>(m_entities["MayaPlayer"].m_handle,
		max::createComponent<CameraComponent>(CameraComponent(0, 45.0f))
	);

	// Player
	m_entities["Player"].m_handle = max::createEntity();
	max::addComponent<CameraComponent>(m_entities["Player"].m_handle,
		max::createComponent<CameraComponent>(CameraComponent(1, 45.0f))
	);
	
	// 
	m_entities["Orb1"].m_handle = max::createEntity();
	max::addComponent<TransformComponent>(m_entities["Orb1"].m_handle,
		max::createComponent<TransformComponent>({ { 0.0f, 0.0f, 2.5f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 2.0f, 2.0f, 2.0f } })
	);
	max::addComponent<RenderComponent>(m_entities["Orb1"].m_handle,
		max::createComponent<RenderComponent>(render)
	);
	max::addComponent<MaterialComponent>(m_entities["Orb1"].m_handle,
		max::createComponent<MaterialComponent>(material1)
	);
	
	// 
	m_entities["Orb2"].m_handle = max::createEntity();
	max::addComponent<TransformComponent>(m_entities["Orb2"].m_handle,
		max::createComponent<TransformComponent>({ { 0.0f, 0.0f, -2.5f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 2.0f, 2.0f, 2.0f } })
	);
	max::addComponent<RenderComponent>(m_entities["Orb2"].m_handle,
		max::createComponent<RenderComponent>(render)
	);
	max::addComponent<MaterialComponent>(m_entities["Orb2"].m_handle,
		max::createComponent<MaterialComponent>(material2)
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