#pragma once

#include <max/max.h>
#include <bx/sharedbuffer.h>

#include <unordered_map>

#include "../../maya-bridge/include/shared_data.h" // Plugin's shared data...

// @todo This needs to exist at runtime but maya bridge does not. Basically I should be able to compile the game without this header.
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
	void update(std::unordered_map<std::string, EntityHandle>& _world, std::unordered_map<std::string, EntityHandle>& _entities);

	bx::SharedBuffer m_buffer;
	bx::SharedBuffer m_readbuffer;
	SharedData* m_shared;
};
