#pragma once

#include <bx/sharedbuffer.h>

#include "../../maya-bridge/include/shared_data.h" // Plugin's shared data...

struct World;
struct Entities;

struct MayaBridge
{
	bool begin();
	bool end();

	void read(World* _scene, Entities* _entities);

	bx::SharedBuffer m_buffer;
	bx::SharedBuffer m_readbuffer;
	SharedData* m_shared;
};
