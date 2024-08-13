#pragma once

#include <max/max.h>
#include <bx/sharedbuffer.h>

#include <unordered_map>

#include "../../maya-bridge/include/shared_data.h" // Plugin's shared data...

struct EntityHandle
{
	EntityHandle()
		: m_handle(MAX_INVALID_HANDLE)
	{}

	max::EntityHandle m_handle;
};

struct MayaBridge
{
	bool begin();
	bool end();
	void update(std::unordered_map<std::string, EntityHandle>& _entities);

	bx::SharedBuffer m_buffer;
	bx::SharedBuffer m_readbuffer;
	SharedData* m_shared;
};

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