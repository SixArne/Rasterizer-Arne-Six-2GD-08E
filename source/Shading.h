#pragma once
#include <SDL_stdinc.h>

#include "ColorRGB.h"
#include "Vector3.h"


namespace Shading
{
	static dae::ColorRGB Lambert(float kd, const dae::ColorRGB& cd)
	{
		return cd * (kd / (float)M_PI);
	}

	static dae::ColorRGB Phong(float ks, float exp, const dae::Vector3& l, const dae::Vector3& v, const dae::Vector3& n)
	{
		const dae::Vector3 reflect = l - (2.f * (dae::Vector3::Dot(n, l) * n));
		const float angle = std::max(0.f, dae::Vector3::Dot(reflect, v));
		const float reflection = ks * std::powf(angle, exp);

		// return reflection for all colors
		return dae::ColorRGB{ reflection, reflection, reflection };
	}

	static dae::ColorRGB Phong(dae::ColorRGB ks, dae::ColorRGB exp, const dae::Vector3& l, const dae::Vector3& v, const dae::Vector3& n)
	{
		const auto reflect = l - (2.f * (dae::Vector3::Dot(n, l) * n));
		const auto angle = std::max(0.f, dae::Vector3::Dot(reflect, v));
		const auto reflection = ks * std::powf(angle, exp.r);

		// return reflection for all colors
		return dae::ColorRGB{ reflection.r, reflection.g, reflection.b };
	}
}
