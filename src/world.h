#pragma once

#include <unordered_map>

#include "texture_manager.h"
#include "maya_bridge.h"

struct World
{
	World()
		: m_filepath("")
	{}

	void load(const char* _filepath);
	void unload();

	void update();

	bool serialize();
	bool deserialize();

	char m_filepath[1024];

	std::unordered_map<std::string, EntityHandle> m_entities;
	std::unordered_map<std::string, TextureHandle> m_textures;

	TextureManager m_textureManager;
};