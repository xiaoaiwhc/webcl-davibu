/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

#ifndef LUX_RAY_H
#define LUX_RAY_H

#include "vector.h"
#include "point.h"

#define RAY_EPSILON 1e-4f

class  Ray {
public:
	// Ray Public Methods
	Ray() : mint(RAY_EPSILON), maxt(INFINITY) { }

	Ray(const Point &origin, const Vector &direction)
		: o(origin), d(direction), mint(RAY_EPSILON), maxt(INFINITY) { }

	Ray(const Point &origin, const Vector &direction,
		float start, float end = INFINITY)
		: o(origin), d(direction), mint(start), maxt(end) { }

	Point operator()(float t) const { return o + d * t; }
	void GetDirectionSigns(int signs[3]) const {
		signs[0] = d.x < 0.f;
		signs[1] = d.y < 0.f;
		signs[2] = d.z < 0.f;
	}
	// Ray Public Data
	Point o;
	Vector d;
	mutable float mint, maxt;
};

inline ostream &operator<<(ostream &os, Ray &r) {
	os << "org: " << r.o << "dir: " << r.d << " range [" <<
		r.mint << "," << r.maxt << "]";
	return os;
}

#endif //LUX_RAY_H
