//External includes
#include "SDL.h"
#include "SDL_surface.h"

// Std includes
#include <iostream>
#include <algorithm>

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

inline float EdgeFunction(const Vector2& a, const Vector2& b, const Vector2& c)
{
	// TODO: look into this math later

	// point to side => c - a
	// side to end => b - a
	return Vector2::Cross(b - a, c - a);
}

inline float EdgeFunction(const Vector3& a, const Vector3& b, const Vector3& c)
{
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
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

	//Render_W1_Part1();
	//Render_W1_Part2();
	//Render_W1_Part3();
	//Render_W1_Part4();
	Render_W1_Part5();
	//Render_Test();

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::Render_W1_Part1()
{
	std::vector<Vector3> vertices_ndc
	{
		{0.f, .5f, 1.f},
		{.5f, -.5f, 1.f},
		{-.5f, -.5f, 1.0f}
	};

	// To Raster space
	for (Vector3& vec: vertices_ndc)
	{
		vec.x = ((vec.x + 1) * (float)m_Width) / 2.f;
		vec.y = ((1 - vec.y) * (float)m_Height) / 2.f;
	}

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			ColorRGB finalColor{ 0,0,0 };

			if (IsInTriangle(vertices_ndc, px, py))
				finalColor = { 1,1,1 };

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void Renderer::Render_W1_Part2()
{
	// World coordinates
	const std::vector<Vertex> vertices_world
	{
		// Triangle 1
		{{0.f, 2.f, 0.f}},
		{{1.f, .0f, .0f}},
		{{-1.f, .0f, .0f}},
	};

	// Buffer for raster
	std::vector<Vertex> vertices_raster{};
	vertices_raster.reserve(vertices_world.size());

	// Transform from World -> View -> Projected -> Raster
	VertexTransformationFunction(vertices_world, vertices_raster);

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			ColorRGB finalColor{ 0,0,0 };

			if (IsInTriangle(vertices_raster, px, py))
				finalColor = { 1,1,1 };

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void Renderer::Render_W1_Part3()
{
	// World coordinates
	const std::vector<Vertex> vertices_world
	{
		{{0.f, 4.f, 2.f}, {1,0,0}},
		{{3.f, -2.f, 2.f}, {0,1,0}},
		{{-3.f, -2.f, 2.f}, {0,0,1}},
	};

	// Buffer for raster
	std::vector<Vertex> vertices_raster{};
	vertices_raster.reserve(vertices_world.size());

	// Transform from World -> View -> Projected -> Raster
	VertexTransformationFunction(vertices_world, vertices_raster);

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			ColorRGB finalColor{ 0,0,0 };

			Vector3 point{(float)px, (float)py, 0.f };

			if (IsInTriangle(vertices_raster, px, py))
			{
				const float area = EdgeFunction(vertices_raster[0].position, vertices_raster[1].position, vertices_raster[2].position);
				const float w0 = EdgeFunction(vertices_raster[1].position, vertices_raster[2].position, point);
				const float w1 = EdgeFunction(vertices_raster[2].position, vertices_raster[0].position, point);
				const float w2 = EdgeFunction(vertices_raster[0].position, vertices_raster[1].position, point);

				finalColor = { vertices_raster[0].color * (w0 / area) + vertices_raster[1].color * (w1 / area) + vertices_raster[2].color * (w2 / area) };
			}
				

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void Renderer::Render_W1_Part4()
{
	// World coordinates
	const std::vector<Vertex> vertices_world
	{
		// Triangle 0
		{{0.f, 2.f, 0.f}, {1,0,0}},
		{{1.5f, -1.f, .0f}, {1,0,0}},
		{{-1.5f, -1.f, .0f}, {1,0,0}},

		// Triangle 1
		{{0.f, 4.f, 2.f}, {1,0,0}},
		{{3.f, -2.f, 2.f}, {0,1,0}},
		{{-3.f, -2.f, 2.f}, {0,0,1}},
	};

	// Buffer for raster
	std::vector<Vertex> vertices_raster{};
	vertices_raster.reserve(vertices_world.size());

	// Transform from World -> View -> Projected -> Raster
	VertexTransformationFunction(vertices_world, vertices_raster);

	// Make float array size of image that will act as depth buffer
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	// Clear back buffer
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100,100,100));

	for (int vertexIndex{}; vertexIndex < vertices_raster.size(); vertexIndex += 3)
	{
		std::vector<Vertex> triangle{
			vertices_raster[vertexIndex], 
			vertices_raster[vertexIndex + 1], 
			vertices_raster[vertexIndex + 2] 
		};

		const Vector2 vertex0 = Vector2{ triangle[0].position.x, triangle[0].position.y };
		const Vector2 vertex1 = Vector2{ triangle[1].position.x, triangle[1].position.y };
		const Vector2 vertex2 = Vector2{ triangle[2].position.x, triangle[2].position.y };

		for (int px{}; px < m_Width; ++px)
		{
			for (int py{}; py < m_Height; ++py)
			{
				Vector2 point{ (float)px, (float)py };

				// Barycentric coordinates
				const float w0 = EdgeFunction(vertex1, vertex2, point);
				const float w1 = EdgeFunction(vertex2, vertex0, point);
				const float w2 = EdgeFunction(vertex0, vertex1, point);
				const float area = w0 + w1 + w2;

				// In triangle
				const bool isInTriangle = w0 >= 0 && w1 >= 0 && w2 >= 0;

				if (isInTriangle)
				{
					// Get relative Z components of triangle vertices
					float vertex0z = triangle[0].position.z;
					float vertex1z = triangle[1].position.z;
					float vertex2z = triangle[2].position.z;

					// Get the hit point Z with the barycentric weights

					// Get the relative z
					const float z = vertex0z * w0 + vertex1z * w1 + vertex2z * w2;

					const int pixelZIndex = py * m_Width + px;

					// If new z value of pixel is lower than stored:
					if (z < m_pDepthBufferPixels[pixelZIndex])
					{
						m_pDepthBufferPixels[pixelZIndex] = z;
						ColorRGB finalColor = { triangle[0].color * (w0 / area) + triangle[1].color * (w1 / area) + triangle[2].color * (w2 / area) };

						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}
}

void Renderer::Render_W1_Part5()
{
	// World coordinates
	const std::vector<Vertex> vertices_world
	{
		// Triangle 0
		{{0.f, 2.f, 0.f}, {1,0,0}},
		{{1.5f, -1.f, .0f}, {1,0,0}},
		{{-1.5f, -1.f, .0f}, {1,0,0}},

		// Triangle 1
		{{0.f, 4.f, 2.f}, {1,0,0}},
		{{3.f, -2.f, 2.f}, {0,1,0}},
		{{-3.f, -2.f, 2.f}, {0,0,1}},
	};

	// Buffer for raster
	std::vector<Vertex> vertices_raster{};
	vertices_raster.reserve(vertices_world.size());

	// Transform from World -> View -> Projected -> Raster
	VertexTransformationFunction(vertices_world, vertices_raster);

	// Make float array size of image that will act as depth buffer
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	// Clear back buffer
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	for (int vertexIndex{}; vertexIndex < vertices_raster.size(); vertexIndex += 3)
	{
		std::vector<Vertex> triangle{
			vertices_raster[vertexIndex],
			vertices_raster[vertexIndex + 1],
			vertices_raster[vertexIndex + 2]
		};

		const Vector2 vertex0 = Vector2{ triangle[0].position.x, triangle[0].position.y };
		const Vector2 vertex1 = Vector2{ triangle[1].position.x, triangle[1].position.y };
		const Vector2 vertex2 = Vector2{ triangle[2].position.x, triangle[2].position.y };

		// create and clamp bounding box top left
		const int topLeftX = static_cast<int>(std::min(vertex0.x, std::min(vertex1.x, vertex2.x)));
		const int topLeftY = static_cast<int>(std::min(vertex0.y, std::min(vertex1.y, vertex2.y)));
		const std::pair<uint32_t, uint32_t> topLeft = std::make_pair<uint32_t, uint32_t>(
			static_cast<uint32_t>(std::clamp(topLeftX, 0, m_Width - 1)),
			static_cast<uint32_t>(std::clamp(topLeftY, 0, m_Height - 1))
		);

		// create and clamp bounding box bottom right
		const int rightBottomX = static_cast<int>(std::max(vertex0.x, std::max(vertex1.x, vertex2.x)));
		const int rightBottomY = static_cast<int>(std::max(vertex0.y, std::max(vertex1.y, vertex2.y)));
		const std::pair<uint32_t, uint32_t> rightBottom = std::make_pair<uint32_t, uint32_t>(
			static_cast<uint32_t>(std::clamp(rightBottomX, 0, m_Width - 1)),
			static_cast<uint32_t>(std::clamp(rightBottomY, 0, m_Height - 1))
		);


		for (uint32_t px{topLeft.first}; px < rightBottom.first; ++px)
		{
			for (uint32_t py{topLeft.second}; py < rightBottom.second; ++py)
			{
				Vector2 point{ (float)px, (float)py };

				// Barycentric coordinates
				const float w0 = EdgeFunction(vertex1, vertex2, point);
				const float w1 = EdgeFunction(vertex2, vertex0, point);
				const float w2 = EdgeFunction(vertex0, vertex1, point);
				const float area = w0 + w1 + w2;

				// In triangle
				const bool isInTriangle = w0 >= 0 && w1 >= 0 && w2 >= 0;

				if (isInTriangle)
				{
					// Get relative Z components of triangle vertices
					float vertex0z = triangle[0].position.z;
					float vertex1z = triangle[1].position.z;
					float vertex2z = triangle[2].position.z;

					// Get the hit point Z with the barycentric weights

					// Get the relative z
					const float z = vertex0z * w0 + vertex1z * w1 + vertex2z * w2;

					const int pixelZIndex = py * m_Width + px;

					// If new z value of pixel is lower than stored:
					if (z < m_pDepthBufferPixels[pixelZIndex])
					{
						m_pDepthBufferPixels[pixelZIndex] = z;
						ColorRGB finalColor = { triangle[0].color * (w0 / area) + triangle[1].color * (w1 / area) + triangle[2].color * (w2 / area) };

						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}
}

void Renderer::Render_Test()
{
	for (uint32_t x{}; x < m_Width; x++)
	{
		for (uint32_t y{}; y < m_Height; y++)
		{
			ColorRGB finalColor = { 1,1,1 };

			//Update Color in Buffer
			finalColor.MaxToOne();

			if (x >= m_Width - 20 && y >= m_Height - 20)
			{
				m_pBackBufferPixels[x + (y * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}

			

		}
	}


	
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	for (size_t vIndex{}; vIndex < vertices_in.size(); ++vIndex)
	{
		Vertex vertexWorldSpace = vertices_in[vIndex];

		// Transform world to view (camera space)
		const Vector3 viewSpaceVertex = m_Camera.viewMatrix.TransformPoint(vertexWorldSpace.position);

		// Position buffer
		Vector3 position{};

		// Perspective divide -> Projection
		position.x = (viewSpaceVertex.x / viewSpaceVertex.z) / (((float)m_Width / (float)m_Height) * m_Camera.fov);
		position.y = (viewSpaceVertex.y / viewSpaceVertex.z) / m_Camera.fov;
		position.z = viewSpaceVertex.z;

		// Projection -> raster
		position.x = ((position.x + 1) * (float)m_Width) / 2.f;
		position.y = ((1 - position.y) * (float)m_Height) / 2.f;

		Vertex rasterVertex{ position, vertices_in[vIndex].color };
		vertices_out.emplace_back(rasterVertex);
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

bool Renderer::IsInTriangle(const std::vector<Vertex>& screenTriangleCoordinates, uint32_t pixelX, uint32_t pixelY)
{
	const Vector3 p{ (float)pixelX, (float)pixelY, 0 };

	const Vector3 V0 = screenTriangleCoordinates[2].position;
	const Vector3 V1 = screenTriangleCoordinates[1].position;
	const Vector3 V2 = screenTriangleCoordinates[0].position;

	if (EdgeFunction(V0, V1, p) < 0.f)
		return false;

	if (EdgeFunction(V1, V2, p) < 0.f)
		return false;

	if (EdgeFunction(V2, V0, p) < 0.f)
		return false;

	return true;
}

bool Renderer::IsInTriangle(const std::vector<Vector3>& screenTriangleCoordinates, uint32_t pixelX, uint32_t pixelY)
{
	const Vector3 p{ (float)pixelX, (float)pixelY, 0.f};

	const Vector3 V0 = screenTriangleCoordinates[2];
	const Vector3 V1 = screenTriangleCoordinates[1];
	const Vector3 V2 = screenTriangleCoordinates[0];

	if (EdgeFunction(V0, V1, p) < 0.f)
		return false;

	if (EdgeFunction(V1, V2, p) < 0.f)
		return false;

	if (EdgeFunction(V2, V0, p) < 0.f)
		return false;

	return true;
}
