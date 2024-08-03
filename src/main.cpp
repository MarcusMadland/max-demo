#include <max/max.h>

#include <bx/math.h>
#include <bx/sharedbuffer.h>

#include <unordered_map>

#include "../../../maya-bridge/include/shared_data.h" // Plugin's shared data...

// Components
struct TransformComponent
{
	bx::Vec3 m_position;
	bx::Quaternion m_rotation;
	bx::Vec3 m_scale;
};

struct RenderComponent
{
	max::MeshHandle m_mesh;
	max::MaterialHandle m_material;
};

// Scene
struct Scene
{
	void load()
	{
		m_isSynced = false;

		// Init shared buffer.
		if (m_buffer.init("maya-bridge", sizeof(SharedData)))
		{
			m_shared = BX_NEW(max::getAllocator(), SharedData);
			m_shared->m_processed = false;

			m_shared->m_meshChanged.m_changed = false;
			m_shared->m_transformChanged.m_changed = false;

			BX_TRACE("Shared memory initialized successfully.");
		}
		else
		{
			BX_TRACE("Failed to initialize shared memory.");
		}
	}

	void unload()
	{
		// Destroy all entities in scene.
		for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
		{
			if (RenderComponent* rc = max::getComponent<RenderComponent>(it->second.m_handle))
			{
				max::destroy(rc->m_mesh);
				max::destroy(rc->m_material);
			}
			max::destroy(it->second.m_handle);
		}

		// Shutdown shared buffer.
		bx::deleteObject(max::getAllocator(), m_shared);
		m_shared = NULL;

		m_buffer.shutdown();
	}

	void update()
	{
		if (!m_buffer.read(m_shared, sizeof(SharedData)))
		{
			m_isSynced = false;
			return;
		}

		if (!m_shared->m_processed)
		{
			m_isSynced = false;
			return;
		}

		// Update synced.
		SharedData::SyncEvent& syncEvent = m_shared->m_sync;
		m_isSynced = syncEvent.m_isSynced;

		// Update camera.
		SharedData::CameraEvent& cameraEvent = m_shared->m_camera;
		max::setViewTransform(0, cameraEvent.m_view, cameraEvent.m_proj); // @todo

		// Update mesh.
		SharedData::MeshEvent& meshEvent = m_shared->m_meshChanged;
		if (meshEvent.m_changed)
		{
			max::VertexLayout layout;
			layout.begin()
				.add(max::Attrib::Position, 3, max::AttribType::Float)
				.add(max::Attrib::Normal, 3, max::AttribType::Float)
				.add(max::Attrib::TexCoord0, 2, max::AttribType::Float)
				.end();

			max::EntityHandle& entity = m_entities[meshEvent.m_name].m_handle;
			if (isValid(entity))
			{
				if (meshEvent.m_numVertices == 0 && meshEvent.m_numIndices == 0)
				{
					// Destroy entity.
					if (RenderComponent* rc = max::getComponent<RenderComponent>(entity))
					{
						max::destroy(rc->m_mesh);
						max::destroy(rc->m_material);
					}
					max::destroy(entity);
					entity = MAX_INVALID_HANDLE;
				}
				else
				{
					// Update entity.
					RenderComponent* rc = max::getComponent<RenderComponent>(entity);
					if (rc != NULL)
					{
						const max::Memory* vertices = max::copy(meshEvent.m_vertices, layout.getSize(meshEvent.m_numVertices));
						const max::Memory* indices = max::copy(meshEvent.m_indices, meshEvent.m_numIndices * sizeof(uint16_t));

						max::update(rc->m_mesh, vertices, indices);
					}
				}
			}
			else if (meshEvent.m_numVertices != 0 && meshEvent.m_numIndices != 0)
			{
				// Create entity.
				entity = max::createEntity();

				TransformComponent tc = {
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 1.0f },
					{ 1.0f, 1.0f, 1.0f },
					// @todo Will it always be this? Even when importing meshes?
				};
				max::addComponent<TransformComponent>(entity, max::createComponent<TransformComponent>(tc));

				const max::Memory* vertices = max::copy(meshEvent.m_vertices, layout.getSize(meshEvent.m_numVertices));
				const max::Memory* indices = max::copy(meshEvent.m_indices, meshEvent.m_numIndices * sizeof(uint16_t));

				max::MaterialHandle whiteMaterial = max::createMaterial(max::loadProgram("vs_cube", "fs_cube"));
				float white[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
				max::addParameter(whiteMaterial, "u_color", white);

				RenderComponent rc;
				rc.m_mesh = max::createMesh(vertices, indices, layout, true);
				rc.m_material = whiteMaterial;
				max::addComponent<RenderComponent>(entity, max::createComponent<RenderComponent>(rc));
			}
		}

		// Update transform.
		SharedData::TransformEvent transformEvent = m_shared->m_transformChanged;
		if (transformEvent.m_changed)
		{
			max::EntityHandle& entity = m_entities[transformEvent.m_name].m_handle;
			if (isValid(entity))
			{
				// Update entity.
				TransformComponent* tc = max::getComponent<TransformComponent>(entity);
				if (tc != NULL)
				{
					tc->m_position = {
						transformEvent.m_pos[0],
						transformEvent.m_pos[1],
						transformEvent.m_pos[2]
					};
					tc->m_rotation = {
						transformEvent.m_rotation[0],
						transformEvent.m_rotation[1],
						transformEvent.m_rotation[2],
						transformEvent.m_rotation[3]
					};
					tc->m_scale = {
						transformEvent.m_scale[0],
						transformEvent.m_scale[1],
						transformEvent.m_scale[2]
					};
				}
			}
		}
	}

	bool m_isSynced;

	bx::SharedBuffer m_buffer;
	SharedData* m_shared;

	struct EntityHandle
	{
		EntityHandle()
			: m_handle(MAX_INVALID_HANDLE)
		{}

		max::EntityHandle m_handle;
	};
	std::unordered_map<std::string, EntityHandle> m_entities;
};

class ExampleMayaBridge : public max::AppI
{
public:
	ExampleMayaBridge(const char* _name)
		: max::AppI(_name)
	{}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		m_width  = _width;
		m_height = _height;
		m_debug  = MAX_DEBUG_TEXT;
		m_reset  = MAX_RESET_VSYNC;

		// Initialize engine.
		max::Init init;
		init.rendererType = max::RendererType::Count;
		init.physicsType = max::PhysicsType::Count;
		init.vendorId = MAX_PCI_ID_NONE;
		init.platformData.nwh  = max::getNativeWindowHandle({0});
		init.platformData.type = max::getNativeWindowHandleType();
		init.platformData.ndt  = max::getNativeDisplayHandle();
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		max::init(init);

		// Load scene.
		m_scene.load();
	}

	virtual int shutdown() override
	{
		// Unload scene.
		m_scene.unload();

		// Shutdown engine.
		max::shutdown();

		return 0;
	}

	bool update() override
	{
		max::MouseState mouseState;

		// Process events.
		if (!max::processEvents(m_width, m_height, m_debug, m_reset, &mouseState) )
		{
			// Set debug mode.
			max::setDebug(m_debug);

			// Debug drawing.
			max::dbgTextClear();

			max::dbgDrawBegin(0);
			max::dbgDrawGrid(max::Axis::Y, { 0.0f, 0.0f, 0.0f });
			max::dbgDrawEnd();

			// Update scene.
			m_scene.update();
			if (m_scene.m_isSynced)
			{
				max::dbgTextPrintf(0, 0, 0xf, "Connected to Maya...");
			}

			// Camera
			float view[16];
			bx::mtxLookAt(view, { 0.0f, 5.0f, -8.0f }, { 0.0f, 0.5f, 0.0f });
			float proj[16];
			bx::mtxProj(proj, 45.0f, (float)m_width / (float)m_height, 0.01f, 100.0f, max::getCaps()->homogeneousDepth);

			// Basic render system.
			max::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
			max::setViewClear(0
				, MAX_CLEAR_COLOR | MAX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
			);

			max::System<RenderComponent, TransformComponent> renderer;
			renderer.each(10, [](max::EntityHandle _entity, void* _userData)
				{
					RenderComponent* rc = max::getComponent<RenderComponent>(_entity);
					TransformComponent* tc = max::getComponent<TransformComponent>(_entity);
					
					max::MeshQuery* query = max::queryMesh(rc->m_mesh);

					for (uint32_t ii = 0; ii < query->m_num; ++ii)
					{
						float mtx[16];
						bx::mtxSRT(mtx, 
							tc->m_scale.x, tc->m_scale.y, tc->m_scale.z,
							tc->m_rotation.x, tc->m_rotation.y, tc->m_rotation.z, tc->m_rotation.w,
							tc->m_position.x, tc->m_position.y, tc->m_position.z);

						max::setTransform(mtx);

						max::MeshQuery::HandleData& handleData = query->m_handleData[ii];
						if (handleData.m_dynamic)
						{
							max::DynamicVertexBufferHandle dvbh = { handleData.m_vertexHandleIdx };
							max::setVertexBuffer(0, dvbh);

							max::DynamicIndexBufferHandle dibh = { handleData.m_indexHandleIdx };
							max::setIndexBuffer(dibh);
						}
						else
						{
							max::VertexBufferHandle vbh = { handleData.m_vertexHandleIdx };
							max::setVertexBuffer(0, vbh);

							max::IndexBufferHandle ibh = { handleData.m_indexHandleIdx };
							max::setIndexBuffer(ibh);
						}
						
						max::setMaterial(rc->m_material);
						max::setState(0
							| MAX_STATE_WRITE_RGB
							| MAX_STATE_WRITE_A
							| MAX_STATE_WRITE_Z
							| MAX_STATE_DEPTH_TEST_LESS
							| MAX_STATE_MSAA
						);

						max::submit(0, rc->m_material);
					}
				});
			max::frame();

			return true;
		}

		return false;
	}

	Scene m_scene;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
};

MAX_IMPLEMENT_MAIN(ExampleMayaBridge, "01-mayabridge");
