#pragma once

#include <max/max.h>

#include <unordered_map>

#include "maya_bridge.h"

struct Scene
{
	Scene()
		: m_mayaBridge(NULL)
	{}

	void beginMayaBridge();
	void endMayaBridge();

	void update();
	void unload(std::unordered_map<std::string, EntityHandle>& _entities);

	// Entities
	void loadEntities();
	void unloadEntities();

	// World
	bool serializeWorld(const char* _filepath);
	bool deserializeWorld(const char* _filepath);
	void unloadWorld();
	
	MayaBridge* m_mayaBridge;

	std::unordered_map<std::string, EntityHandle> m_world;
	std::unordered_map<std::string, EntityHandle> m_entitiess;
};