#pragma once

#include "handles.h"

#include <max/max.h>

#include <unordered_map>
#include <string>

struct TextureRef
{
	TextureHandle m_texture;
	uint32_t m_refCount;
};

struct TextureManager
{
	max::TextureHandle load(const char* _filePath);
	void unload(const char* _filePath);

	std::unordered_map<std::string, TextureRef> m_textures;
};
