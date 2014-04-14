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

#include <new>
#include "Assert.h"
#include "World.h"
#include "Allocator.h"
#include "Device.h"
#include "ResourceManager.h"
#include "DebugLine.h"
#include "Actor.h"
#include "LuaEnvironment.h"

namespace crown
{

//-----------------------------------------------------------------------------
World::World()
	: m_unit_pool(default_allocator(), CE_MAX_UNITS, sizeof(Unit), CE_ALIGNOF(Unit))
	, m_camera_pool(default_allocator(), CE_MAX_CAMERAS, sizeof(Camera), CE_ALIGNOF(Camera))
	, m_physics_world(*this)
	, m_events(default_allocator())
{
	m_id.id = INVALID_ID;
	m_sound_world = SoundWorld::create(default_allocator());
}

//-----------------------------------------------------------------------------
World::~World()
{
	// Destroy all units
	for (uint32_t i = 0; i < m_units.size(); i++)
	{
		CE_DELETE(m_unit_pool, m_units[i]);
	}

	SoundWorld::destroy(default_allocator(), m_sound_world);
}

//-----------------------------------------------------------------------------
WorldId World::id() const
{
	return m_id;
}

//-----------------------------------------------------------------------------
void World::set_id(WorldId id)
{
	m_id = id;
}

//-----------------------------------------------------------------------------
UnitId World::spawn_unit(const char* name, const Vector3& pos, const Quaternion& rot)
{
	UnitResource* ur = (UnitResource*) device()->resource_manager()->lookup(UNIT_EXTENSION, name);
	ResourceId id = device()->resource_manager()->resource_id(UNIT_EXTENSION, name);
	return spawn_unit(id, ur, pos, rot);
}

//-----------------------------------------------------------------------------
UnitId World::spawn_unit(const ResourceId id, UnitResource* ur, const Vector3& pos, const Quaternion& rot)
{
	Unit* u = (Unit*) m_unit_pool.allocate(sizeof(Unit), CE_ALIGNOF(Unit));
	const UnitId unit_id = m_units.create(u);
	new (u) Unit(*this, unit_id, id, ur, Matrix4x4(rot, pos));

	// SpawnUnitEvent ev;
	// ev.unit = unit_id;
	// event_stream::write(m_events, EventType::SPAWN, ev);

	return unit_id;
}

//-----------------------------------------------------------------------------
void World::destroy_unit(UnitId id)
{
	CE_ASSERT(m_units.has(id), "Unit does not exist");

	CE_DELETE(m_unit_pool, m_units.lookup(id));
	m_units.destroy(id);

	// DestroyUnitEvent ev;
	// ev.unit = id;
	// event_stream::write(m_events, EventType::DESTROY, ev);
}

//-----------------------------------------------------------------------------
void World::reload_units(UnitResource* old_ur, UnitResource* new_ur)
{
	for (uint32_t i = 0; i < m_units.size(); i++)
	{
		if (m_units[i]->resource() == old_ur)
		{
			m_units[i]->reload(new_ur);
		}
	}
}

//-----------------------------------------------------------------------------
uint32_t World::num_units() const
{
	return m_units.size();
}

//-----------------------------------------------------------------------------
void World::link_unit(UnitId child, UnitId parent, int32_t node)
{
	CE_ASSERT(m_units.has(child), "Child unit does not exist");
	CE_ASSERT(m_units.has(parent), "Parent unit does not exist");

	Unit* parent_unit = lookup_unit(parent);
	parent_unit->link_node(0, node);
}

//-----------------------------------------------------------------------------
void World::unlink_unit(UnitId child)
{
	CE_ASSERT(m_units.has(child), "Child unit does not exist");
}

//-----------------------------------------------------------------------------
Unit* World::lookup_unit(UnitId unit)
{
	CE_ASSERT(m_units.has(unit), "Unit does not exist");

	return m_units.lookup(unit);
}

//-----------------------------------------------------------------------------
Camera* World::lookup_camera(CameraId camera)
{
	CE_ASSERT(m_cameras.has(camera), "Camera does not exist");

	return m_cameras.lookup(camera);
}

//-----------------------------------------------------------------------------
void World::update(float dt)
{
	m_physics_world.update(dt);

	m_scenegraph_manager.update();

	m_sound_world->update();

	process_physics_events();
}

//-----------------------------------------------------------------------------
void World::render(Camera* camera)
{
	m_render_world.update(camera->world_pose(), camera->m_projection, camera->m_view_x, camera->m_view_y,
							camera->m_view_width, camera->m_view_height, device()->last_delta_time());
}

//-----------------------------------------------------------------------------
CameraId World::create_camera(SceneGraph& sg, int32_t node, ProjectionType::Enum type, float near, float far)
{
	// Allocate memory for camera
	Camera* camera = CE_NEW(m_camera_pool, Camera)(sg, node, type, near, far);

	// Create Id for the camera
	const CameraId camera_id = m_cameras.create(camera);

	return camera_id;
}

//-----------------------------------------------------------------------------
void World::destroy_camera(CameraId id)
{
	CE_ASSERT(m_cameras.has(id), "Camera does not exist");

	CE_DELETE(m_camera_pool, m_cameras.lookup(id));
	m_cameras.destroy(id);
}

//-----------------------------------------------------------------------------
SoundInstanceId World::play_sound(const char* name, const bool loop, const float volume, const Vector3& pos, const float range)
{
	return m_sound_world->play(name, loop, volume, pos);
}

//-----------------------------------------------------------------------------
void World::stop_sound(SoundInstanceId id)
{
	m_sound_world->stop(id);
}

//-----------------------------------------------------------------------------
void World::link_sound(SoundInstanceId id, Unit* unit, int32_t node)
{
}

//-----------------------------------------------------------------------------
void World::set_listener_pose(const Matrix4x4& pose)
{
	m_sound_world->set_listener_pose(pose);
}

//-----------------------------------------------------------------------------
void World::set_sound_position(SoundInstanceId id, const Vector3& pos)
{
	m_sound_world->set_sound_positions(1, &id, &pos);
}

//-----------------------------------------------------------------------------
void World::set_sound_range(SoundInstanceId id, float range)
{
	m_sound_world->set_sound_ranges(1, &id, &range);
}

//-----------------------------------------------------------------------------
void World::set_sound_volume(SoundInstanceId id, float vol)
{
	m_sound_world->set_sound_volumes(1, &id, &vol);
}

//-----------------------------------------------------------------------------
GuiId World::create_window_gui(const char* name)
{
	GuiResource* gr = (GuiResource*)device()->resource_manager()->lookup(GUI_EXTENSION, name);
	return m_render_world.create_gui(gr);
}

//-----------------------------------------------------------------------------
GuiId World::create_world_gui(const Matrix4x4 pose, const uint32_t width, const uint32_t height)
{
	// Must be implemented
	return GuiId();
}

//-----------------------------------------------------------------------------
void World::destroy_gui(GuiId id)
{
	m_render_world.destroy_gui(id);
}

//-----------------------------------------------------------------------------
Gui* World::lookup_gui(GuiId id)
{
	return m_render_world.lookup_gui(id);
}

//-----------------------------------------------------------------------------
DebugLine* World::create_debug_line(bool depth_test)
{
	return CE_NEW(default_allocator(), DebugLine)(depth_test);
}

//-----------------------------------------------------------------------------
void World::destroy_debug_line(DebugLine* line)
{
	CE_DELETE(default_allocator(), line);
}

//-----------------------------------------------------------------------------
SceneGraphManager* World::scene_graph_manager()
{
	return &m_scenegraph_manager;
}
//-----------------------------------------------------------------------------
RenderWorld* World::render_world()
{
	return &m_render_world;
}

//-----------------------------------------------------------------------------
PhysicsWorld* World::physics_world()
{
	return &m_physics_world;
}

//-----------------------------------------------------------------------------
SoundWorld* World::sound_world()
{
	return m_sound_world;
}

//-----------------------------------------------------------------------------
void World::process_physics_events()
{
	EventStream& events = m_physics_world.events();

	// Read all events
	const char* ee = array::begin(events);
	while (ee != array::end(events))
	{
		event_stream::Header h = *(event_stream::Header*) ee;

		// Log::d("=== PHYSICS EVENT ===");
		// Log::d("type = %d", h.type);
		// Log::d("size = %d", h.size);

		const char* event = ee + sizeof(event_stream::Header);

		switch (h.type)
		{
			case physics_world::EventType::COLLISION:
			{
				physics_world::CollisionEvent coll_ev = *(physics_world::CollisionEvent*) event;

				// Log::d("type    = %s", coll_ev.type == physics_world::CollisionEvent::BEGIN_TOUCH ? "begin" : "end");
				// Log::d("actor_0 = (%p)", coll_ev.actors[0]);
				// Log::d("actor_1 = (%p)", coll_ev.actors[1]);
				// Log::d("unit_0  = (%p)", coll_ev.actors[0]->unit());
				// Log::d("unit_1  = (%p)", coll_ev.actors[1]->unit());
				// Log::d("where   = (%f %f %f)", coll_ev.where.x, coll_ev.where.y, coll_ev.where.z);
				// Log::d("normal  = (%f %f %f)", coll_ev.normal.x, coll_ev.normal.y, coll_ev.normal.z);

				device()->lua_environment()->call_physics_callback(
					coll_ev.actors[0],
					coll_ev.actors[1],
					coll_ev.actors[0]->unit(),
					coll_ev.actors[1]->unit(),
					coll_ev.where,
					coll_ev.normal,
					(coll_ev.type == physics_world::CollisionEvent::BEGIN_TOUCH) ? "begin" : "end");
				break;
			}
			case physics_world::EventType::TRIGGER:
			{
				// physics_world::TriggerEvent trigg_ev = *(physics_world::TriggerEvent*) event;

				// Log::d("type    = %s", trigg_ev.type == physics_world::TriggerEvent::BEGIN_TOUCH ? "begin" : "end");
				// Log::d("trigger = (%p)", trigg_ev.trigger);
				// Log::d("other   = (%p)", trigg_ev.other);
				break;
			}
			default:
			{
				CE_FATAL("Unknown Physics event");
				break;
			}
		}

		// Log::d("=====================");

		// Next event
		ee += sizeof(event_stream::Header) + h.size;
	}

	array::clear(events);
}

} // namespace crown
