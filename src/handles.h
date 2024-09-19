#pragma once

#include <max/max.h>

struct EntityHandle
{
	EntityHandle()
		: m_handle(MAX_INVALID_HANDLE)
	{}

	max::EntityHandle m_handle;
};

struct TextureHandle
{
	TextureHandle()
		: m_handle(MAX_INVALID_HANDLE)
	{}

	max::TextureHandle m_handle;
};


