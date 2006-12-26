/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

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

#pragma once

#include <cassert>
#include <cmath>


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
	
	Vector2 reflectX();
	Vector2 reflectY();
	Vector2 scale(float factor);
	Vector2 scaleX(float factor);
	Vector2 scaleY(float factor);
	float length() const;
	Vector2 normalise();
	Vector2 contraVector();
	
	inline Vector2 halfVector(const Vector2& vec)
	{
		return Vector2(x + (vec.x - x) / 2, y + (vec.y - y) / 2);
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
		return Vector2(x + vector.x, y + vector.y);
	}
	
	inline Vector2 operator - (const Vector2& vector) const
	{
		return Vector2(x - vector.x, y - vector.y);
	}
	
	inline Vector2 operator * (float scalar) const
	{
		return Vector2(x * scalar, y * scalar);
	}
	
	inline Vector2 operator * (Vector2 vector) const
	{
		return Vector2(x * vector.x, y * vector.y);
	}
	
	inline Vector2 operator / (float scalar) const
	{
		assert(scalar != 0.0);
		float invert = 1.0 / scalar;
		return Vector2(x * invert, y * invert);
	}
	
	inline Vector2 operator - () const
	{
		return Vector2(-x, -y);
	}
	
	inline Vector2& operator += (Vector2 vector)
	{
		x += vector.x;
		y += vector.y;
		return *this;
	}
	
	inline Vector2& operator -= (Vector2 vector)
	{
		x -= vector.x;
		y -= vector.y;
		return *this;
	}
	
	inline Vector2& operator *= (Vector2 vector)
	{
		x *= vector.x;
		y *= vector.y;
		return *this;
	}


	inline float dotProduct(const Vector2& vector)
	{
		return x * vector.x + y * vector.y;
	}
	
	inline float crossProduct(const Vector2& vector)
	{
		return x * vector.y - y * vector.x;
	}
	
	inline Vector2 reflect(const Vector2& normal)
	{
		return Vector2(*this - (normal * 2 * dotProduct(normal)));
	}


};

inline Vector2::Vector2()
{
	this->x=0;
	this->y=0;
}

inline Vector2::Vector2(float x, float y)
{
	this->x=x;
	this->y=y;	
}

inline Vector2::Vector2(const Vector2& v1, const Vector2& v2)
{
	x = v2.x - v1.x;
	y = v2.y - v1.y;
}

inline Vector2 Vector2::reflectX()
{
	return Vector2(-x, y);
}

inline Vector2 Vector2::reflectY()
{
	return Vector2(x, -y);
}

inline Vector2 Vector2::scale(float factor)
{
	return Vector2(x * factor, y * factor);
}

inline Vector2 Vector2::scaleX(float factor)
{
	return Vector2(x * factor, y);
}

inline Vector2 Vector2::scaleY(float factor)
{
	return Vector2(x, y * factor);
}

inline float Vector2::length() const
{
#ifdef USE_SSE
	float ret;
	asm (
		"movss 	%1,	%%xmm0	\n"
		"movss 	%2,	%%xmm1	\n"
		"mulss 	%%xmm0,	%%xmm0	\n"
		"mulss 	%%xmm1,	%%xmm1	\n"
		"addss 	%%xmm1,	%%xmm0	\n"
		"sqrtss %%xmm0,	%%xmm0	\n"
		"movss 	%%xmm0,	%0	\n"
		: "=m"(ret)
		: "m"(x), "m"(y)
		: "%xmm0", "%xmm1"
	);
	return ret;
#else
	return sqrt(this->x * this->x + this->y * this->y);
#endif
}

inline Vector2 Vector2::normalise()
{
	float fLength = length();
	if (fLength > 1e-08)
		return Vector2(x / fLength, y / fLength);	
	return *this;
}

inline Vector2 Vector2::contraVector()
{
	return Vector2(-x, -y);
}

inline void Vector2::clear()
{
	x = 0.0;
	y = 0.0;
}

inline bool operator < (Vector2 v1, Vector2 v2)
{
	if (v1.x < v2.x)
	if (v1.y < v2.y)
		return true;
	return false;
	
}

inline bool operator > (Vector2 v1, Vector2 v2)
{
	if (v1.x > v2.x)
	if (v1.y > v2.y)
		return true;
	return false;
}

