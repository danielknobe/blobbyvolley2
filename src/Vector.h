/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

/**
 * @file Vector.h
 * @brief Contains a class to handle two dimensional vectors
 */

#pragma once

#include <cassert>
#include <cmath>
#include <iostream>

/// \brief class for repesenting 2d vectors
/// \details e.g. positions, velocities.
class Vector2
{
	public:
		union
		{
			struct
			{
				float x;
				float y;
			};
			float val[2];
		};

		Vector2();
		Vector2(float x, float y);
		Vector2(const Vector2& v1, const Vector2& v2);

		void clear();

		Vector2 reflectX() const;
		Vector2 reflectY() const;
		Vector2 scale(float factor) const;
		Vector2 scaleX(float factor) const;
		Vector2 scaleY(float factor) const;
		float length() const;
		float lengthSQ() const;
		Vector2 normalise();
		Vector2 contraVector() const ;

		inline Vector2 halfVector(const Vector2& vec) const
		{
			return {x + (vec.x - x) / 2, y + (vec.y - y) / 2};
		}

		inline Vector2& operator = (const Vector2& newVector)
		{
			x = newVector.x;
			y = newVector.y;
			return *this;
		}

		inline bool operator == (const Vector2& vector) const
		{
			return (x == vector.x && y == vector.y);
		}

		inline bool operator != (const Vector2& vector) const
		{
			return (x != vector.x || y != vector.y);
		}

		inline Vector2 operator + (const Vector2& vector) const
		{
			return {x + vector.x, y + vector.y};
		}

		inline Vector2 operator - (const Vector2& vector) const
		{
			return {x - vector.x, y - vector.y};
		}

		inline Vector2 operator * (float scalar) const
		{
			return {x * scalar, y * scalar};
		}

		inline Vector2 operator * (const Vector2& vector) const
		{
			return {x * vector.x, y * vector.y};
		}

		inline Vector2 operator / (float scalar) const
		{
			assert(scalar != 0.0);
			float invert = 1.f / scalar;
			return {x * invert, y * invert};
		}

		inline Vector2 operator - () const
		{
			return {-x, -y};
		}

		inline Vector2& operator += (const Vector2& vector)
		{
			x += vector.x;
			y += vector.y;
			return *this;
		}

		inline Vector2& operator -= (const Vector2& vector)
		{
			x -= vector.x;
			y -= vector.y;
			return *this;
		}

		inline Vector2& operator *= (const Vector2& vector)
		{
			x *= vector.x;
			y *= vector.y;
			return *this;
		}


		inline float dotProduct(const Vector2& vector) const
		{
			return x * vector.x + y * vector.y;
		}

		inline float crossProduct(const Vector2& vector) const
		{
			return x * vector.y - y * vector.x;
		}

		inline Vector2 reflect(const Vector2& normal) const
		{
			return Vector2(*this - (normal * 2 * dotProduct(normal)));
		}

};

// -------------------------------------------------------------------------------------------------
//    							INLINE IMPLEMENTATION
// -------------------------------------------------------------------------------------------------

inline Vector2::Vector2() : x(0), y(0)
{
}

inline Vector2::Vector2(float x_, float y_) : x(x_), y(y_)
{
}

inline Vector2::Vector2(const Vector2& v1, const Vector2& v2) : x(v2.x - v1.x), y(v2.y - v1.y)
{

}

inline Vector2 Vector2::reflectX() const
{
	return {-x, y};
}

inline Vector2 Vector2::reflectY() const
{
	return {x, -y};
}

inline Vector2 Vector2::scale(float factor) const
{
	return {x * factor, y * factor};
}

inline Vector2 Vector2::scaleX(float factor) const
{
	return {x * factor, y};
}

inline Vector2 Vector2::scaleY(float factor) const
{
	return {x, y * factor};
}

inline float Vector2::length() const
{
	return std::sqrt( lengthSQ() );
}

inline float Vector2::lengthSQ() const
{
	return x*x + y*y;
}

inline Vector2 Vector2::normalise()
{
	float fLength = length();
	if (fLength > 1e-08)
		return {x / fLength, y / fLength};

	return *this;
}

inline Vector2 Vector2::contraVector() const
{
	return {-x, -y};
}

inline void Vector2::clear()
{
	x = 0.0;
	y = 0.0;
}

inline bool operator < (const Vector2& v1, const Vector2& v2)
{
	if (v1.x < v2.x)
	{
		if (v1.y < v2.y)
			return true;
	}

	return false;

}

inline bool operator > (const Vector2& v1, const Vector2& v2)
{
	if (v1.x > v2.x)
	{
		if (v1.y > v2.y)
			return true;
	}
	return false;
}

inline std::ostream& operator<<( std::ostream& stream, Vector2 v )
{
	return stream << "(" << v.x << ", " << v.y << ")";
}
