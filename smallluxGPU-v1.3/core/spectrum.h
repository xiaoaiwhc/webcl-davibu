/***************************************************************************
 *   Copyright (C) 1998-2009 by David Bucciarelli (davibu@interfree.it)    *
 *                                                                         *
 *   This file is part of SmallLuxGPU.                                     *
 *                                                                         *
 *   SmallLuxGPU is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *  SmallLuxGPU is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   and Lux Renderer website : http://www.luxrender.net                   *
 ***************************************************************************/

#ifndef LUX_RGB_H
#define LUX_RGB_H

#include <cmath>
#include <algorithm>
#include <ostream>

using namespace std;
using std::ostream;

class Spectrum {
public:
	// Spectrum Public Methods

	Spectrum(float _r = 0.f, float _g = 0.f, float _b = 0.f)
	: r(_r), g(_g), b(_b) {
	}

	Spectrum operator+(const Spectrum &v) const {
		return Spectrum(r + v.r, g + v.g, b + v.b);
	}

	Spectrum operator*(const Spectrum &v) const {
		return Spectrum(r * v.r, g * v.g, b * v.b);
	}

	Spectrum & operator*=(const Spectrum &v) {
		r *= v.r;
		g *= v.g;
		b *= v.b;
		return *this;
	}

	Spectrum & operator+=(const Spectrum &v) {
		r += v.r;
		g += v.g;
		b += v.b;
		return *this;
	}

	Spectrum operator-(const Spectrum &v) const {
		return Spectrum(r - v.r, g - v.g, b - v.b);
	}

	Spectrum & operator-=(const Spectrum &v) {
		r -= v.r;
		g -= v.g;
		b -= v.b;
		return *this;
	}

	bool operator==(const Spectrum &v) const {
		return r == v.r && g == v.g && b == v.b;
	}

	Spectrum operator*(float f) const {
		return Spectrum(f*r, f*g, f * b);
	}

	Spectrum & operator*=(float f) {
		r *= f;
		g *= f;
		b *= f;
		return *this;
	}

	Spectrum operator/(float f) const {
		float inv = 1.f / f;
		return Spectrum(r * inv, g * inv, b * inv);
	}

	Spectrum & operator/=(float f) {
		float inv = 1.f / f;
		r *= inv;
		g *= inv;
		b *= inv;
		return *this;
	}

	Spectrum operator-() const {
		return Spectrum(-r, -g, -b);
	}

	float operator[](int i) const {
		return (&r)[i];
	}

	float &operator[](int i) {
		return (&r)[i];
	}

	float filter() const {
		return max<float>(r, max<float>(g, b));
	}

	bool Black() const {
		return (r == 0.f) && (g == 0.f) && (g == 0.f);
	}

	float r, g, b;
};

inline ostream & operator<<(ostream &os, const Spectrum &v) {
	os << v.r << ", " << v.g << ", " << v.b;
	return os;
}

inline Spectrum operator*(float f, const Spectrum &v) {
	return v * f;
}

#endif //LUX_Spectrum_H
