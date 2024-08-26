#pragma once

#include <max/max.h>

#include <unordered_map>

#include "maya_bridge.h"

struct Scene
{
	Scene()
		: m_mayaBridge(NULL)
	{}

	void load();
	void unload();
	void update();

	void beginMayaBridge();
	void endMayaBridge();

	bool serialize(const char* _filepath);
	bool deserialize(const char* _filepath);
	
	MayaBridge* m_mayaBridge;
	std::unordered_map<std::string, EntityHandle> m_entities;
};