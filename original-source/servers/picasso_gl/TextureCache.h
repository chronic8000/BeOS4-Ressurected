#ifndef TEXTURE_CACHE_H
#define TEXTURE_CACHE_H

#include <support2/KeyedVector.h>
#include <support2/SupportDefs.h>
#include <support2/Binder.h>
#include <render2/Rect.h>

namespace B {
namespace Render2 {
	class IVisual;
}};

using B::Support2::BKeyedVector;
using B::Support2::IBinder;
using B::Render2::BRect;

class TextureCache
{
public:
	struct texture_t {
		uint32	texture_id;
		uint32	cache_id;
		BRect	frame;
	};

				TextureCache();
	virtual		~TextureCache();
	
	const texture_t	*GetTexture(const IBinder::ref& view);
	status_t CacheTexture(const IBinder::ref& view, const texture_t& texture);
	status_t UpdateTexture(const IBinder::ref& view, const texture_t& texture);
private:
	BKeyedVector<IBinder::ref, texture_t>	m_textures;
};


#endif
