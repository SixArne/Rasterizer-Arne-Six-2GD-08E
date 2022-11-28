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
	m_pTextureBuffer = Texture::LoadFromFile("Resources/Vehicle_diffuse.png");
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize((float)m_Width / (float)m_Height, 60.f, { .0f,.0f,-10.f });

	std::vector<Vertex> vertices{};
	std::vector<uint32_t> indices{};

	Utils::ParseOBJ("Resources/Vehicle.obj", vertices, indices);

	Mesh mesh{};
	mesh.vertices = vertices;
	mesh.indices = indices;
	mesh.primitiveTopology = PrimitiveTopology::TriangleList;
	mesh.transformMatrix = Matrix::CreateTranslation({ 0,-5,50 });
	mesh.scaleMatrix = Matrix::CreateScale({ 1,1,1 });
	mesh.rotationMatrix = Matrix::CreateRotationY(90 * TO_RADIANS);
	mesh.worldMatrix = mesh.scaleMatrix * mesh.rotationMatrix * mesh.transformMatrix;

	m_Meshes.push_back(mesh);
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTextureBuffer;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	for (Mesh& mesh : m_Meshes)
	{
		mesh.SetRotationY((cos(pTimer->GetTotal()) + 1.f) / 2.f * PI_2);
	}
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//Render_W1();
	RenderFrame();
	//Render_Test();

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::RenderFrame()
{
	// Transform from World -> View -> Projected -> Raster
	VertexTransformationFunction(m_Meshes);

	// Make float array size of image that will act as depth buffer
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	// Clear back buffer
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	for (const auto& mesh : m_Meshes)
	{
		if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
		{
			const uint32_t amountOfTriangles = ((uint32_t)mesh.indices.size()) / 3;
			
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


void Renderer::VertexTransformationFunction(std::vector<Mesh>& meshes) const
{
	// Calculate once
	for (auto& mesh : meshes)
	{
		Matrix worldMatrix = mesh.scaleMatrix * mesh.rotationMatrix * mesh.transformMatrix;
		const auto worldViewProjectionMatrix = worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;

		mesh.vertices_out.clear();

		// Loop over indices (every 3 indices is triangle)
		for (const auto vertex : mesh.vertices)
		{
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
		}
	}
}

void dae::Renderer::RenderTriangle(Vertex_Out vertex1, Vertex_Out vertex2, Vertex_Out vertex3)
{
	const Vector2 v0 = Vector2{ vertex1.position.x, vertex1.position.y };
	const Vector2 v1 = Vector2{ vertex2.position.x, vertex2.position.y };
	const Vector2 v2 = Vector2{ vertex3.position.x, vertex3.position.y };

	// create and clamp bounding box top left
	int maxX{}, maxY{};
	maxX = static_cast<int>(std::max(v0.x, std::max(v1.x, v2.x)));
	maxY = static_cast<int>(std::max(v0.y, std::max(v1.y, v2.y)));

	int minX{}, minY{};
	minX = static_cast<int>(std::min(v0.x, std::min(v1.x, v2.x)));
	minY = static_cast<int>(std::min(v0.y, std::min(v1.y, v2.y)));

	minX = std::clamp(minX, 0, m_Width);
	minY = std::clamp(minY, 0, m_Height);

	maxX = std::clamp(maxX, 0, m_Width);
	maxY = std::clamp(maxY, 0, m_Height);


	for (int px{ minX }; px <= maxX; ++px)
	{
		for (int py{ minY }; py <= maxY; ++py)
		{
			Vector2 point{ (float)px, (float)py };

			// Barycentric coordinates
			float w0 = EdgeFunction(v1, v2, point);
			float w1 = EdgeFunction(v2, v0, point);
			float w2 = EdgeFunction(v0, v1, point);


			// In triangle
			const bool isInTriangle = w0 >= 0 && w1 >= 0 && w2 >= 0;

			if (isInTriangle)
			{
				const float area = w0 + w1 + w2;
				w0 /= area;
				w1 /= area;
				w2 /= area;


				// Get the hit point Z with the barycentric weights
				float z = 1.f / ((w0 / vertex1.position.z) + (w1 / vertex2.position.z) + (w2 / vertex3.position.z));

				if (z < 0 || z > 1)
				{
					continue;
				}

				const int pixelZIndex = py * m_Width + px;

				// If new z value of pixel is lower than stored:
				if (z < m_pDepthBufferPixels[pixelZIndex])
				{
					m_pDepthBufferPixels[pixelZIndex] = z;

					float wInterpolated = 1.f / ((w0 / vertex1.position.w) + (w1 / vertex2.position.w) + (w2 / vertex3.position.w));
					Vector2 uvInterpolated = (vertex1.uv * (w0 / vertex1.position.w)) + (vertex2.uv * (w1 / vertex2.position.w)) + (vertex3.uv * (w2 / vertex3.position.w));
					uvInterpolated *= wInterpolated;

					uvInterpolated.x = std::clamp(uvInterpolated.x, 0.f, 1.f);
					uvInterpolated.y = std::clamp(uvInterpolated.y, 0.f, 1.f);

					//ColorRGB finalColor = { z,z,z };
					ColorRGB finalColor = m_pTextureBuffer->Sample(uvInterpolated);

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
