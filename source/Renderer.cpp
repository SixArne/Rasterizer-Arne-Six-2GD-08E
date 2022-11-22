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
#include <ppl.h>

using namespace dae;

inline float EdgeFunction(const Vector2& a, const Vector2& b, const Vector2& c)
{
	// TODO: look into this math later

	// point to side => c - a
	// side to end => b - a
	return Vector2::Cross(b - a, c - a);
}

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pTextureBuffer = Texture::LoadFromFile("Resources/uv_grid_2.png");
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize((float)m_Width / (float)m_Height, 60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTextureBuffer;
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

	//Render_W1();
	Render_W2_P1();
	//Render_Test();

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::Render_W1()
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

void dae::Renderer::Render_W2_P1()
{
	std::vector<Mesh> meshes
	{
		Mesh
		{
			{
				Vertex{{ -3, 3,-2},{},{ 0, 0}},
				Vertex{{  0, 3,-2},{},{.5, 0}},
				Vertex{{  3, 3,-2},{},{ 1, 0}},
				Vertex{{ -3, 0,-2},{},{ 0,.5}},
				Vertex{{  0, 0,-2},{},{.5,.5}},
				Vertex{{  3, 0,-2},{},{ 1,.5}},
				Vertex{{ -3,-3,-2},{},{ 0, 1}},
				Vertex{{  0,-3,-2},{},{.5, 1}},
				Vertex{{  3,-3,-2},{},{ 1, 1}},
			},
			{
				3,0,1,   1,4,3,   4,1,2,
				2,5,4,   6,3,4,   4,7,6,
				7,4,5,   5,8,7,
			},
			PrimitiveTopology::TriangeList,
		}
	};

	/*std::vector<Mesh> meshes
	{
		Mesh
		{
			{
				Vertex{{ -3,3,-2}},
				Vertex{{ 0,3,-2}},
				Vertex{{ 3,3,-2}},
				Vertex{{ -3,0,-2}},
				Vertex{{ 0,0,-2}},
				Vertex{{ 3,0,-2}},
				Vertex{{ -3,-3,-2}},
				Vertex{{ 0,-3,-2}},
				Vertex{{ 3,-3,-2}},
			},
			{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5
			},
			PrimitiveTopology::TriangleStrip,
		}
	};*/

	// Transform from World -> View -> Projected -> Raster
	VertexTransformationFunction(meshes);

	// Make float array size of image that will act as depth buffer
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	// Clear back buffer
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	for (const auto& mesh : meshes)
	{
		if (mesh.primitiveTopology == PrimitiveTopology::TriangeList)
		{
			const uint32_t amountOfTriangles = ((uint32_t)mesh.indices.size()) / 3;

			/*for (uint32_t index{}; index < amountOfTriangles; index++)
			{
				const Vertex_Out vertex1 = mesh.vertices_out[mesh.indices[3 * index]];
				const Vertex_Out vertex2 = mesh.vertices_out[mesh.indices[3 * index + 1]];
				const Vertex_Out vertex3 = mesh.vertices_out[mesh.indices[3 * index + 2]];

				RenderTriangle(vertex1, vertex2, vertex3);
			}*/

			// fps go BRRRRRRRRRRRRRRR

			concurrency::parallel_for(0u, amountOfTriangles, [=, this](int index)
			{
				const Vertex_Out vertex1 = mesh.vertices_out[mesh.indices[3 * index]];
				const Vertex_Out vertex2 = mesh.vertices_out[mesh.indices[3 * index + 1]];
				const Vertex_Out vertex3 = mesh.vertices_out[mesh.indices[3 * index + 2]];

				// cull triangles
				if (vertex1.position.x < 0 || vertex2.position.x < 0 || vertex3.position.x < 0
					|| vertex1.position.x > m_Width || vertex2.position.x > m_Width || vertex3.position.x > m_Width
					|| vertex1.position.y < 0 || vertex2.position.y < 0 || vertex3.position.y < 0
					|| vertex1.position.y > m_Height || vertex2.position.y > m_Height || vertex3.position.y > m_Height
					)
				{
					return;
				}

				RenderTriangle(vertex1, vertex2, vertex3);
			});
		}
		else if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			for (uint32_t indice{}; indice < mesh.indices.size() - 2; indice++)
			{
				const Vertex_Out vertex1 = mesh.vertices_out[mesh.indices[indice]];
				Vertex_Out vertex2{};
				Vertex_Out vertex3{};

				if (indice & 1)
				{
					vertex2 = mesh.vertices_out[mesh.indices[indice + 2]];
					vertex3 = mesh.vertices_out[mesh.indices[indice + 1]];
				}
				else
				{
					vertex2 = mesh.vertices_out[mesh.indices[indice + 1]];
					vertex3 = mesh.vertices_out[mesh.indices[indice + 2]];
				}

				if (vertex1.position.x < 0 || vertex2.position.x < 0 || vertex3.position.x < 0
					|| vertex1.position.x > m_Width || vertex2.position.x > m_Width || vertex3.position.x > m_Width
					|| vertex1.position.y < 0 || vertex2.position.y < 0 || vertex3.position.y < 0
					|| vertex1.position.y > m_Height || vertex2.position.y > m_Height || vertex3.position.y > m_Height
					)
				{
					return;
				}

				RenderTriangle(vertex1, vertex2, vertex3);
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

#define proj

void Renderer::VertexTransformationFunction(std::vector<Mesh>& meshes) const
{
	// Calculate once
	const auto worldViewProjectionMatrix = m_Camera.viewMatrix * m_Camera.projectionMatrix;
	for (auto& mesh : meshes)
	{
		// Loop over indices (every 3 indices is triangle)
		for (const auto vertex : mesh.vertices)
		{
#ifndef proj
			// Transform world to view (camera space)
			const Vector3 viewSpaceVertex = m_Camera.viewMatrix.TransformPoint(vertex.position);

			// Position buffer
			Vector4 position{};

			// Perspective divide -> Projection
			position.x = (viewSpaceVertex.x / viewSpaceVertex.z) / (((float)m_Width / (float)m_Height) * m_Camera.fov);
			position.y = (viewSpaceVertex.y / viewSpaceVertex.z) / m_Camera.fov;
			position.z = viewSpaceVertex.z;

			// Projection -> raster
			position.x = ((position.x + 1) * (float)m_Width) / 2.f;
			position.y = ((1 - position.y) * (float)m_Height) / 2.f;

			// Add vertex to vertices out and color
			Vertex_Out rasterVertex{ position, vertex.color, vertex.uv };
			mesh.vertices_out.push_back(rasterVertex);
#endif

			

#ifdef proj
			// Transform world to view (camera space)
			auto position = Vector4{ vertex.position, 1 };

			// Transform point
			auto transformedVertex =  worldViewProjectionMatrix.TransformPoint(position);
	
			// perspective divide
			transformedVertex.x /= transformedVertex.w;
			transformedVertex.y /= transformedVertex.w;
			transformedVertex.z /= transformedVertex.w;

			// NDC to raster coords
			transformedVertex.x = ((transformedVertex.x + 1) * (float)m_Width)  / 2.f;
			transformedVertex.y = ((1 - transformedVertex.y) * (float)m_Height) / 2.f;

			// Add vertex to vertices out and color
			Vertex_Out rasterVertex{ transformedVertex, vertex.color, vertex.uv };
			mesh.vertices_out.push_back(rasterVertex);
#endif
		}
	}
}

void dae::Renderer::RenderTriangle(Vertex_Out v1, Vertex_Out v2, Vertex_Out v3)
{
	std::vector<Vertex_Out> triangle{
		v1,
		v2,
		v3,
	};

	const Vector2 vertex0 = Vector2{ triangle[0].position.x, triangle[0].position.y };
	const Vector2 vertex1 = Vector2{ triangle[1].position.x, triangle[1].position.y };
	const Vector2 vertex2 = Vector2{ triangle[2].position.x, triangle[2].position.y };

	// create and clamp bounding box top left
	int maxX{}, maxY{};
	maxX = static_cast<int>(std::max(vertex0.x, std::max(vertex1.x, vertex2.x)));
	maxY = static_cast<int>(std::max(vertex0.y, std::max(vertex1.y, vertex2.y)));
	
	int minX{}, minY{};
	minX = static_cast<int>(std::min(vertex0.x, std::min(vertex1.x, vertex2.x)));
	minY = static_cast<int>(std::min(vertex0.y, std::min(vertex1.y, vertex2.y)));

	minX = std::clamp(minX - 1, 0, m_Width);
	minY = std::clamp(minY - 1, 0, m_Height);

	maxX = std::clamp(maxX + 1, 0, m_Width);
	maxY = std::clamp(maxY + 1, 0, m_Height);


	for (int px{ minX }; px < maxX; ++px)
	{
		for (int py{ minY }; py < maxY; ++py)
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
				float v0w = triangle[0].position.w;
				float v1w = triangle[1].position.w;
				float v2w = triangle[2].position.w;

				float v0z = triangle[0].position.z;
				float v1z = triangle[1].position.z;
				float v2z = triangle[2].position.z;

				// Get the hit point Z with the barycentric weights

				// Get the relative z
				const float z = 1 / ( (1 / v0z * w0) + ( 1 / v1z * w1) +  (1 / v2z * w2));

				const int pixelZIndex = py * m_Width + px;

				// If new z value of pixel is lower than stored:
				if (z < m_pDepthBufferPixels[pixelZIndex])
				{
					m_pDepthBufferPixels[pixelZIndex] = z;
					float wInterpolated = 1.f / ((1 / v0w * w0) + (1 / v1w * w1) + (1 / v2w * w2));
					Vector2 uvInterpolated = (v1.uv * (w0 / v0w) + v2.uv * (w1 /  v1w) + v3.uv * (w2 / v2w));

					ColorRGB finalColor = m_pTextureBuffer->Sample(uvInterpolated * wInterpolated);


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

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
