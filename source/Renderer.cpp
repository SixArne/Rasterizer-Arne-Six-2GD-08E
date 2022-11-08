//External includes
#include "SDL.h"
#include "SDL_surface.h"

// Std includes
#include <iostream>

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// Define triangle [NDC] space
	const std::vector<Vector3> vertices_ndc
	{
		{0.f, .5f, 1.f},
		{.5f, -.5f, 1.f},
		{-.5f, -.5f, 1.f}
	};


	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			/*float gradient = px / static_cast<float>(m_Width);
			gradient += py / static_cast<float>(m_Width);
			gradient /= 2.0f;*/

			ColorRGB finalColor{ 0,0,0 };

			const std::vector<Vector2> screenSpaceCoordinates = BufferToScreenSpace(vertices_ndc);

			if (DoesCover(screenSpaceCoordinates, px, py))
				finalColor = { 1,1,1 };

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

const std::vector<Vector2> Renderer::BufferToScreenSpace(const std::vector<Vector3>& triangles)
{
	std::vector<Vector2> screenSpaceCoordinates{};
	screenSpaceCoordinates.reserve(3);

	for (const Vector3 triangle : triangles)
	{
		Vector2 screenSpacePixel{};

		screenSpacePixel.x = ((triangle.x + 1) * (float)m_Width) / 2.f;
		screenSpacePixel.y = ((1 - triangle.y) * (float)m_Height) / 2.f;
		
		screenSpaceCoordinates.emplace_back(screenSpacePixel);
	}

	return screenSpaceCoordinates;
}

inline bool EdgeFunction(const Vector2& a, const Vector2& b, const Vector2& c)
{
	return ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x) >= 0);
}

#define USE_EDGE
//#define CROSS

bool Renderer::DoesCover(const std::vector<Vector2>& screenTriangleCoordinates, uint32_t pixelX, uint32_t pixelY)
{
#ifdef USE_EDGE
	Vector2 p{ (float)pixelX, (float)pixelY };

	auto V0 = screenTriangleCoordinates[0];
	auto V1 = screenTriangleCoordinates[1];
	auto V2 = screenTriangleCoordinates[2];

	bool inside = true;

	inside &= EdgeFunction(V0, V1, p);
	inside &= EdgeFunction(V1, V2, p);
	inside &= EdgeFunction(V2, V0, p);


	if (inside == true)
	{
		std::cout << "I'm inside yey" << "\n";
	}
	return inside;
#endif
#ifdef CROSS
#endif
}