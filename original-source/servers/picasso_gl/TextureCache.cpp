#include <support2/SupportDefs.h>

#include "TextureCache.h"

TextureCache::TextureCache()
{
}

TextureCache::~TextureCache()
{
}
	
const TextureCache::texture_t	*TextureCache::GetTexture(const IBinder::ref& view)
{
	bool found;
	const texture_t *result = &(m_textures.ValueFor(view, &found));
	return (found) ? result : NULL;
}

status_t TextureCache::CacheTexture(const IBinder::ref& view, const TextureCache::texture_t& texture)
{
	ssize_t s = m_textures.AddItem(view, texture);
	return (s<0) ? (status_t)s : B_OK;
}

status_t TextureCache::UpdateTexture(const IBinder::ref& view, const TextureCache::texture_t& texture)
{
	bool found;
	texture_t& edit = m_textures.EditValueFor(view, &found);
	if (!found)
		return B_ENTRY_NOT_FOUND;
	edit = texture;
	return B_OK;
}

