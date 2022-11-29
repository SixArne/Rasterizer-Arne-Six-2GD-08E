#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

#include <stdexcept>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		SDL_Surface* pTextureSurface = IMG_Load(path.data());
		if (pTextureSurface == NULL)
		{
			throw std::runtime_error("Error, texture file not found");
		}

		return new Texture(pTextureSurface);
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		const int16_t pixelX = m_pSurface->w * uv.x;
		const int16_t pixelY = m_pSurface->h * uv.y;
		const int32_t pixelIndex = pixelY * m_pSurface->w + pixelX;

		uint8_t r{}, g{}, b{};
		SDL_GetRGB(m_pSurfacePixels[pixelIndex], m_pSurface->format, &r, &g, &b);

		return{
			(float)r / 255.f,
			(float)g / 255.f,
			(float)b / 255.f
		};
	}
}