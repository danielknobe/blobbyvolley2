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

#pragma once

#include "Vector.h"

// flags
extern bool FLAG_BOUNCE;

void reset_flags();

float time_to_x(const Vector2& pos, const Vector2& vel, float destination);
float time_to_y(const Vector2& pos, const Vector2& vel, float destination);

float predict_x(const Vector2& pos, const Vector2& vel, float time);
float predict_y(const Vector2& pos, const Vector2& vel, float time);

float y_at_x(const Vector2& pos, const Vector2& vel, float destination);
float x_at_y(const Vector2& pos, const Vector2& vel, float destination);

float next_event(const Vector2& pos, const Vector2& vel);

