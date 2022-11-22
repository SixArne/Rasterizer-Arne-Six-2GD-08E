#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};
		float moveSpeedPerSecond{10.f};
		float aspectRatio{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		void Initialize(float ar, float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);
			aspectRatio = ar;

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			//TODO W1
			//ONB => invViewMatrix

			const Matrix rotation = { Matrix::CreateRotationX(-totalPitch * TO_RADIANS) * Matrix::CreateRotationY(totalYaw * TO_RADIANS) };

			forward = rotation.TransformVector(Vector3::UnitZ).Normalized();
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			invViewMatrix = {
				Vector4{right, 0},
				Vector4{up, 0},
				Vector4{forward, 0},
				Vector4{origin, 1},
			};

			viewMatrix = invViewMatrix.Inverse();

			//Inverse(ONB) => ViewMatrix

			

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			constexpr float nearPlane = .1f;
			constexpr float farPlane = 100.f;

			const float A = farPlane / (farPlane - nearPlane);
			const float B = (-(farPlane * nearPlane)) / (farPlane - nearPlane);
			const float OneOverAStFOV = 1.f / (aspectRatio * fov);
			const float OneOverFOV = 1.f / fov;

			projectionMatrix = Matrix{
				Vector4{OneOverAStFOV, 0, 0, 0},
				Vector4{0,OneOverFOV ,0,0},
				Vector4{0,0,A,1},
				Vector4{0,0,B,0},
			};

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic


			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			// Movement
			if (pKeyboardState[SDL_SCANCODE_W])
			{
				origin += forward * moveSpeedPerSecond * deltaTime;
			}

			if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= forward * moveSpeedPerSecond * deltaTime;
			}

			if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin -= right * moveSpeedPerSecond * deltaTime;
			}

			if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += right * moveSpeedPerSecond * deltaTime;
			}

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			if ((mouseState & SDL_BUTTON_RMASK) != 0)
			{
				if (!(mouseState & SDL_BUTTON_LMASK) != 0)
				{
					totalYaw += mouseX;
					totalPitch += mouseY;

					// Recalculate view matrix
					//CalculateViewMatrix();
				}
			}

			if ((mouseState & SDL_BUTTON_LMASK) != 0 && (mouseState & SDL_BUTTON_RMASK) != 0)
			{
				origin += up * moveSpeedPerSecond * deltaTime * mouseY;
			}
			else if ((mouseState & SDL_BUTTON_LMASK) != 0)
			{
				if (mouseY > 0)
				{
					origin -= forward * moveSpeedPerSecond * deltaTime;
				}
				else if (mouseY < 0)
				{
					origin += forward * moveSpeedPerSecond * deltaTime;
				}

				totalYaw += mouseX;
			}

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
