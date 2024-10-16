#include "camera.h"

#include <max/max.h>

#include "components.h"
#include "input.h"

struct CameraSystem
{
	void create(CameraSettings* _settings)
	{
		m_settings = _settings;
	}

	void destroy()
	{

	}

	void update()
	{
		max::System<CameraComponent> camera;
		camera.each(10, [](max::EntityHandle _entity, void* _userData)
		{
			CameraSystem* system = (CameraSystem*)_userData;
			CameraSettings* settings = system->m_settings;

			// Update with active camera only.
			CameraComponent* cc = max::getComponent<CameraComponent>(_entity);
				
			// Camera animation.
			if (cc->m_numSplinePoints > 0)
			{
			}
			// Camera from input.
			else
			{
				// Orbit camera.
				TransformComponent* tc = max::getComponent<TransformComponent>(_entity);
				if (tc != NULL && false) // @todo
				{
					bx::mtxLookAt(cc->m_view,
						{ 0.0f, 5.0f, -8.0f },
						{ 0.0f, 0.5f, 0.0f },
						{ 0.0f, 1.0f, 0.0f },
						bx::Handedness::Right
					);

					cc->m_aspect = (float)settings->m_viewport.m_width / (float)settings->m_viewport.m_height;

					bx::mtxProj(cc->m_proj,
						cc->m_fov,
						cc->m_aspect,
						settings->m_near,
						settings->m_far,
						max::getCaps()->homogeneousDepth,
						bx::Handedness::Right
					);
				}
				// Fly camera.
				else
				{
					const bx::Vec3 right = bx::normalize(bx::cross(cc->m_up, cc->m_direction));
					const bx::Vec3 forward = bx::normalize(cc->m_direction);
							
					cc->m_position = bx::add(cc->m_position, bx::mul(bx::mul(forward, max::inputGetAsFloat(0, Action::MoveForward)), settings->m_moveSpeed * max::getDeltaTime()));
					cc->m_position = bx::add(cc->m_position, bx::mul(bx::mul(right, max::inputGetAsFloat(0, Action::MoveRight)), settings->m_moveSpeed * max::getDeltaTime()));
					cc->m_position = bx::add(cc->m_position, bx::mul(bx::mul({0.0f, 1.0f, 0.0f}, max::inputGetAsFloat(0, Action::MoveUp)), settings->m_moveSpeed * max::getDeltaTime()));

					// @todo Into component
					static float pitch = 0.0f;
					static float yaw = 180.0f;

					yaw -= max::inputGetAsFloat(0, Action::LookRight) * settings->m_lookSpeed * max::getDeltaTime();
					pitch -= max::inputGetAsFloat(0, Action::LookUp) * settings->m_lookSpeed * max::getDeltaTime();
					pitch = bx::clamp(pitch, -89.0f, 89.0f);

					bx::Vec3 direction = { 0.0f, 0.0f, 0.0f };
					direction.x = bx::cos(bx::toRad(yaw)) * bx::cos(bx::toRad(pitch));
					direction.y = bx::sin(bx::toRad(pitch));
					direction.z = bx::sin(bx::toRad(yaw)) * bx::cos(bx::toRad(pitch));
					cc->m_direction = bx::normalize(direction);

					bx::mtxLookAt(cc->m_view,
						cc->m_position,
						bx::add(cc->m_position, cc->m_direction),
						{0.0f, 1.0f, 0.0f},
						bx::Handedness::Right);

					cc->m_aspect = (float)settings->m_viewport.m_width / (float)settings->m_viewport.m_height;

					bx::mtxProj(cc->m_proj,
						cc->m_fov,
						cc->m_aspect,
						settings->m_near,
						settings->m_far, 
						max::getCaps()->homogeneousDepth,
						bx::Handedness::Right);

					max::dbgTextPrintf(0, 0, 0xf, "Position: %f, %f, %f | Direction: %f, %f, %f",
						cc->m_position.x, cc->m_position.y, cc->m_position.z,
						cc->m_direction.x, cc->m_direction.y, cc->m_direction.z);
				}
			}

		}, this);
	}

	CameraSettings* m_settings;
};

static CameraSystem* s_ctx = NULL;

void cameraCreate(CameraSettings* _settings)
{
	s_ctx = BX_NEW(max::getAllocator(), CameraSystem);
	s_ctx->create(_settings);
}

void cameraDestroy()
{
	s_ctx->destroy();
	bx::deleteObject<CameraSystem>(max::getAllocator(), s_ctx);
	s_ctx = NULL;
}

void cameraUpdate()
{
	s_ctx->update();
}

