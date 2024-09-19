#pragma once

#include <unordered_map>

#include "maya_bridge.h"
#include "handles.h"

struct Entities
{
	Entities()
	{}

	void load();
	void unload();
	void update();
	
	std::unordered_map<std::string, EntityHandle> m_entities;
};