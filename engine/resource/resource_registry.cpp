/*
Copyright (c) 2013 Daniele Bartolini, Michele Rossi
Copyright (c) 2012 Daniele Bartolini, Simone Boscaratto

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "resource_registry.h"
#include "lua_resource.h"
#include "texture_resource.h"
#include "mesh_resource.h"
#include "sound_resource.h"
#include "sprite_resource.h"
#include "package_resource.h"
#include "unit_resource.h"
#include "physics_resource.h"
#include "material_resource.h"
#include "font_resource.h"
#include "level_resource.h"

namespace crown
{

static const ResourceCallback RESOURCE_CALLBACK_REGISTRY[] =
{
	{ LUA_TYPE, LuaResource::load, LuaResource::unload, LuaResource::online, LuaResource::offline },
	{ TEXTURE_TYPE, TextureResource::load, TextureResource::unload, TextureResource::online, TextureResource::offline },
	{ MESH_TYPE, MeshResource::load, MeshResource::unload, MeshResource::online, MeshResource::offline },
	{ SOUND_TYPE, SoundResource::load, SoundResource::unload, SoundResource::online, SoundResource::offline },
	{ UNIT_TYPE, UnitResource::load, UnitResource::unload, UnitResource::online, UnitResource::offline },
	{ SPRITE_TYPE, SpriteResource::load, SpriteResource::unload, SpriteResource::online, SpriteResource::offline},
	{ PACKAGE_TYPE, PackageResource::load, PackageResource::unload, PackageResource::online, PackageResource::offline },
	{ PHYSICS_TYPE, PhysicsResource::load, PhysicsResource::unload, PhysicsResource::online, PhysicsResource::offline },
	{ MATERIAL_TYPE, MaterialResource::load, MaterialResource::unload, MaterialResource::online, MaterialResource::offline },
	{ PHYSICS_CONFIG_TYPE, PhysicsConfigResource::load, PhysicsConfigResource::unload, PhysicsConfigResource::online, PhysicsConfigResource::offline },
	{ FONT_TYPE, FontResource::load, FontResource::unload, FontResource::online, FontResource::offline },
	{ LEVEL_TYPE, LevelResource::load, LevelResource::unload, LevelResource::online, LevelResource::offline },
	{ 0, NULL, NULL, NULL, NULL }
};

//-----------------------------------------------------------------------------
static const ResourceCallback* find_callback(uint64_t type)
{
	const ResourceCallback* c = RESOURCE_CALLBACK_REGISTRY;

	while (c->type != 0)
	{
		if (c->type == type)
		{
			return c;
		}

		c++;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
void* resource_on_load(uint64_t type, Allocator& allocator, Bundle& bundle, ResourceId id)
{
	const ResourceCallback* c = find_callback(type);

	CE_ASSERT_NOT_NULL(c);

	return c->on_load(allocator, bundle, id);
}

//-----------------------------------------------------------------------------
void resource_on_unload(uint64_t type, Allocator& allocator, void* resource)
{
	const ResourceCallback* c = find_callback(type);

	CE_ASSERT_NOT_NULL(c);

	return c->on_unload(allocator, resource);
}

//-----------------------------------------------------------------------------
void resource_on_online(uint64_t type, void* resource)
{
	const ResourceCallback* c = find_callback(type);

	CE_ASSERT_NOT_NULL(c);

	return c->on_online(resource);
}

//-----------------------------------------------------------------------------
void resource_on_offline(uint64_t type, void* resource)
{
	const ResourceCallback* c = find_callback(type);

	CE_ASSERT_NOT_NULL(c);

	return c->on_offline(resource);
}

} // namespace crown