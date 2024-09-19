#include "texture_manager.h"

max::TextureHandle TextureManager::load(const char* _filePath)
{
	TextureRef& tr = m_textures[_filePath];

	if (isValid(tr.m_texture.m_handle))
	{
		++tr.m_refCount;
		return tr.m_texture.m_handle;
	}
	else
	{
		max::TextureHandle handle = max::loadTexture(_filePath);

		if (isValid(handle))
		{
			tr.m_refCount = 1;
			tr.m_texture.m_handle = handle;
			return tr.m_texture.m_handle;
		}
		else
		{
			BX_TRACE("Failed to load texture at, %s", _filePath)
			return MAX_INVALID_HANDLE;
		}
	}
}

void TextureManager::unload(const char* _filePath)
{
	TextureRef& tr = m_textures[_filePath];

	if (isValid(tr.m_texture.m_handle))
	{
		--tr.m_refCount;

		if (tr.m_refCount == 0)
		{
			max::destroy(tr.m_texture.m_handle);
			tr.m_texture.m_handle = MAX_INVALID_HANDLE;
		}
	}
}