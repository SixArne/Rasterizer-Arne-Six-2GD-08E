#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();
		void RenderFrame();

		void ToggleDisplayRenderDepthBuffer();
		void ToggleRotationOfModel() { m_ShouldRotateModel = !m_ShouldRotateModel; };
		void ToggleNormalMap() { m_ShouldDisplayNormalMap = !m_ShouldDisplayNormalMap; };
		void ToggleShadingCycle();

		bool SaveBufferToImage() const;

	private:
		enum class ShadingCycle
		{
			DepthMode,
			ObservedArea,
			Diffuse,
			Specular,
			Combined,
			ENUM_LENGTH,
		};

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		Texture* m_pTextureBuffer{ nullptr };
		Texture* m_pNormalBuffer{ nullptr };

		uint32_t* m_pSurfacePixels{};
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		// settings
		bool m_IsDisplayingDepthBuffer{};
		bool m_ShouldRotateModel{};
		bool m_ShouldDisplayNormalMap{};
		ShadingCycle m_CurrentCycle{ShadingCycle::Combined};
		ShadingCycle m_LastCycle{ ShadingCycle::Combined };

		Camera m_Camera{};

		std::vector<Mesh> m_Meshes{};

		int m_Width{};
		int m_Height{};
		int m_CurrentFrame{};

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(std::vector<Mesh>& meshes) const; //W2 version
		void RenderTriangle(Vertex_Out v1, Vertex_Out v2, Vertex_Out v3);
		ColorRGB ShadePixel(const Vertex_Out& vertex);
	};
}
