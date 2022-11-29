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
#include "Shading.h"
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

	// Texture maps
	m_pTextureBuffer = Texture::LoadFromFile("Resources/vehicle_diffuse.png");
	m_pNormalBuffer = Texture::LoadFromFile("Resources/vehicle_normal.png");
	m_pSpecularBuffer = Texture::LoadFromFile("Resources/vehicle_specular.png");
	m_pGlossinessBuffer = Texture::LoadFromFile("Resources/vehicle_gloss.png");

	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize((float)m_Width / (float)m_Height, 60.f, { .0f,.0f,-10.f });

	std::vector<Vertex> vertices{};
	std::vector<uint32_t> indices{};

	Utils::ParseOBJ("Resources/vehicle.obj", vertices, indices);

	Mesh mesh{};
	mesh.vertices = vertices;
	mesh.indices = indices;
	mesh.primitiveTopology = PrimitiveTopology::TriangleList;
	mesh.transformMatrix = Matrix::CreateTranslation({ 0,0,50 });
	mesh.scaleMatrix = Matrix::CreateScale({ 1,1,1 });
	mesh.yawRotation = 90.f * TO_RADIANS;
	mesh.rotationMatrix = Matrix::CreateRotationY(mesh.yawRotation);
	mesh.worldMatrix = mesh.scaleMatrix * mesh.rotationMatrix * mesh.transformMatrix;

	m_Meshes.push_back(mesh);
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTextureBuffer;
	delete m_pNormalBuffer;
	delete m_pGlossinessBuffer;
	delete m_pSpecularBuffer;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	for (Mesh& mesh : m_Meshes)
	{
		if (m_ShouldRotateModel)
		{
			// 1 deg per second
			const float degreesPerSecond = 25.f;
			mesh.AddRotationY((degreesPerSecond * pTimer->GetElapsed())* TO_RADIANS);
		}
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

void Renderer::ToggleDisplayRenderDepthBuffer()
{
	if (m_CurrentCycle != ShadingCycle::DepthMode)
	{
		m_LastCycle = m_CurrentCycle;
		m_CurrentCycle = ShadingCycle::DepthMode;
	}
	else
	{
		m_CurrentCycle = m_LastCycle;
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
			Vertex_Out rasterVertex{};

			
			auto position = Vector4{ vertex.position, 1 };

			// Transform model to raster (screen space)
			auto transformedVertex =  worldViewProjectionMatrix.TransformPoint(position);
			rasterVertex.viewDirection = worldMatrix.TransformPoint(vertex.position) - m_Camera.origin;
	
			// perspective divide
			transformedVertex.x /= transformedVertex.w;
			transformedVertex.y /= transformedVertex.w;
			transformedVertex.z /= transformedVertex.w;

			// NDC to raster coords
			transformedVertex.x = ((transformedVertex.x + 1) * (float)m_Width)  / 2.f;
			transformedVertex.y = ((1 - transformedVertex.y) * (float)m_Height) / 2.f;

			// Add vertex to vertices out and color
	
			rasterVertex.position = transformedVertex;
			rasterVertex.color = vertex.color;
			rasterVertex.uv = vertex.uv;

			Vector3 transformedNormals = worldMatrix.TransformVector(vertex.normal);
			transformedNormals.Normalize();

			Vector3 transformedTangent = worldMatrix.TransformVector(vertex.tangent);
			transformedTangent.Normalize();

			rasterVertex.normal = transformedNormals;
			rasterVertex.tangent = transformedTangent;

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
					const float wInterpolated = 1.f / ((w0 / vertex1.position.w) + (w1 / vertex2.position.w) + (w2 / vertex3.position.w));

					// color interpolated
					ColorRGB interpolatedColor = (vertex1.color * (w0 / vertex1.position.w)) + (vertex2.color * (w1 / vertex2.position.w)) + (vertex3.color * (w2 / vertex3.position.w));
					interpolatedColor *= wInterpolated;

					// uv interpolated
					Vector2 uvInterpolated = (vertex1.uv * (w0 / vertex1.position.w)) + (vertex2.uv * (w1 / vertex2.position.w)) + (vertex3.uv * (w2 / vertex3.position.w));
					uvInterpolated *= wInterpolated;

					uvInterpolated.x = std::clamp(uvInterpolated.x, 0.f, 1.f);
					uvInterpolated.y = std::clamp(uvInterpolated.y, 0.f, 1.f);

					// normal interpolated
					Vector3 normalInterpolated =
						(vertex1.normal * (w0 / vertex1.position.w)) +
						(vertex2.normal * (w1 / vertex2.position.w)) +
						(vertex3.normal * (w2 / vertex3.position.w));
					normalInterpolated *= wInterpolated;
					normalInterpolated.Normalize();

					// tangent interpolated
					Vector3 tangentInterpolated = (vertex1.tangent * (w0 / vertex1.position.w)) + (vertex2.tangent * (w1 / vertex2.position.w)) + (vertex3.tangent* (w2 / vertex3.position.w));
					tangentInterpolated *= wInterpolated;
					tangentInterpolated.Normalize();

					// view dir interpolated
					Vector3 viewDirInterpolated = (vertex1.viewDirection * (w0 / vertex1.position.w)) + (vertex2.viewDirection * (w1 / vertex2.position.w)) + (vertex3.viewDirection * (w2 / vertex3.position.w));
					viewDirInterpolated *= wInterpolated;
					viewDirInterpolated.Normalize();

					Vertex_Out fragmentToShade{};
					fragmentToShade.color = interpolatedColor;
					fragmentToShade.position = Vector4{(float)px,(float)py,z, wInterpolated};
					fragmentToShade.uv = uvInterpolated;
					fragmentToShade.normal = normalInterpolated;
					fragmentToShade.tangent = tangentInterpolated;
					fragmentToShade.viewDirection = viewDirInterpolated;


					ColorRGB finalColor{  };

					if (m_CurrentCycle != ShadingCycle::DepthMode)
					{
						finalColor = ShadePixel(fragmentToShade);
					}
					else
					{
						float depthValue = Utils::Remap(z, 0.995f, 1.f);
						finalColor = { depthValue, depthValue, depthValue };
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
	}
}

ColorRGB Renderer::ShadePixel(const Vertex_Out& vertex)
{
	// Normal map stuff
	const Vector3 binormal = Vector3::Cross(vertex.normal, vertex.tangent).Normalized();
	const Matrix tangentSpaceAxis = Matrix{vertex.tangent, binormal, vertex.normal, {0,0,0}};

	// Sample normal
	const ColorRGB normalColor = m_pNormalBuffer->Sample(vertex.uv);
	Vector3 normalSample = { normalColor.r, normalColor.g, normalColor.b };
	normalSample = 2.f * normalSample - Vector3{1.f, 1.f, 1.f};
	normalSample = tangentSpaceAxis.TransformPoint(normalSample);
	normalSample.Normalize();

	// Sample color
	const ColorRGB color = m_pTextureBuffer->Sample(vertex.uv);

	// Sample specular
	const ColorRGB specularColor = m_pSpecularBuffer->Sample(vertex.uv);

	// Sample glossiness
	const ColorRGB glossinessColor = m_pGlossinessBuffer->Sample(vertex.uv);

	// Lights
	const Vector3 lightDirection = { .577f, -.577f, .577f };
	const float lightIntensity = 7.f;
	const float shininess = 25.f;
	const ColorRGB ambient = { .025f, .025f, .025f };
	const ColorRGB light = ColorRGB{ 1,1,1 } *(lightIntensity / lightDirection.Magnitude());

	// Visible areas with normal map taken into account
	const float lambertCosine = Vector3::Dot(normalSample, -lightDirection);

	if (lambertCosine <= 0)
	{
		return { 0,0,0 };
	}

	switch (m_CurrentCycle)
	{
	case ShadingCycle::ObservedArea:
		return { lambertCosine, lambertCosine, lambertCosine };
		break;
	case ShadingCycle::Diffuse:
		{
			const ColorRGB diffuse = Shading::Lambert(1.f, color);
			return light * diffuse * lambertCosine;
		}
		break;
	case ShadingCycle::Specular:
		{
			
			const auto specularReflectance = specularColor * shininess;
			const auto phongExponent = glossinessColor * shininess;

			const ColorRGB specular = Shading::Phong(
				specularReflectance, 
				phongExponent, 
				lightDirection, 
				vertex.viewDirection, 
				vertex.normal
			);

			return light * specular * lambertCosine;
		}
		
		break;
	case ShadingCycle::Combined:
		{
			const auto specularReflectance = specularColor * shininess;
			const auto phongExponent = glossinessColor * shininess;

			const ColorRGB specular = Shading::Phong(
				specularReflectance,
				phongExponent,
				lightDirection,
				vertex.viewDirection,
				vertex.normal
			);

			const ColorRGB diffuse = Shading::Lambert(1.f, color);

			return light * (diffuse + specular + ambient) * lambertCosine;
		}
		
		break;
	case ShadingCycle::ENUM_LENGTH:
		throw std::runtime_error("Unknown mode, bug in code");
	}
}

void Renderer::ToggleShadingCycle()
{
	const auto shadingCycleIndex = static_cast<int8_t>(m_CurrentCycle);
	const auto newShadingCycleIndex = (shadingCycleIndex + 1) % static_cast<int8_t>(ShadingCycle::ENUM_LENGTH);

	m_CurrentCycle = static_cast<ShadingCycle>(newShadingCycleIndex);

	switch (m_CurrentCycle)
	{
	case ShadingCycle::DepthMode:
		std::cout << "Shading mode: Depth" << "\n";
		break;
	case ShadingCycle::ObservedArea:
		std::cout << "Shading mode: Observed area" << "\n";
		break;
	case ShadingCycle::Diffuse:
		std::cout << "Shading mode: Diffuse" << "\n";
		break;
	case ShadingCycle::Specular:
		std::cout << "Shading mode: Specular" << "\n";
		break;
	case ShadingCycle::Combined:
		std::cout << "Shading mode: Combined" << "\n";
		break;
	case ShadingCycle::ENUM_LENGTH:
		throw std::runtime_error("Unknown mode, bug in code");
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
