#include "camera.h"

#include <max/max.h>

#include "components.h"

void camera(CameraSettings* _settings)
{
	max::System<CameraComponent> system;
	system.each(10, [](max::EntityHandle _entity, void* _userData)
		{
			CameraSettings* settings = (CameraSettings*)_userData;

			// Update with active camera only.
			CameraComponent* cc = max::getComponent<CameraComponent>(_entity);
			if (cc->m_idx == settings->m_activeCameraIdx)
			{
				// Camera animation.
				if (cc->m_numAnimations > 0)
				{
				}
				// Camera from input.
				else
				{
					// Orbit camera.
					TransformComponent* tc = max::getComponent<TransformComponent>(_entity); 
					if (tc != NULL)
					{
						bx::mtxLookAt(cc->m_view,
							{ 0.0f, 5.0f, -8.0f },
							{ 0.0f, 0.5f, 0.0f }
						);

						bx::mtxProj(cc->m_proj,
							cc->m_fov,
							(float)settings->m_width / (float)settings->m_height,
							settings->m_near,
							settings->m_far,
							max::getCaps()->homogeneousDepth
						);
					}
					// Fly camera.
					else
					{
						bx::mtxLookAt(cc->m_view,
							{ 0.0f, 5.0f, -8.0f },
							{ 0.0f, 0.5f, 0.0f }
						);

						bx::mtxProj(cc->m_proj,
							cc->m_fov,
							(float)settings->m_width / (float)settings->m_height,
							settings->m_near,
							settings->m_far,
							max::getCaps()->homogeneousDepth
						);
					}
				}
			}
			
		}, _settings);
	
}