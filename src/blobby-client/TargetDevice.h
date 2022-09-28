/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)
Copyright (C) 2022 Erik Schultheis (erik-schultheis@freenet.de)

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

// Detect whether we have a desktop or mobile target
/// TODO should we do this in CMake?
#if (defined __ANDROID__) || (defined __APPLE__ && TARGET_OS_IPHONE) || (defined __APPLE__ && TARGET_OS_SIMULATOR) || (defined __SWITCH__)
#define BLOBBY_ON_MOBILE true
#define BLOBBY_ON_DESKTOP false
#else
#define BLOBBY_ON_MOBILE false
#define BLOBBY_ON_DESKTOP true
#endif

// Detect features
#if (defined __APPLE__ && TARGET_OS_IPHONE) || (defined __APPLE__ && TARGET_IPHONE_SIMULATOR)
#define BLOBBY_FEATURE_HAS_BACKBUTTON false
#else
#define BLOBBY_FEATURE_HAS_BACKBUTTON true
#endif

const char AppTitle[] = "Blobby Volley 2 Version 1.0";
const int BASE_RESOLUTION_X = 800;
const int BASE_RESOLUTION_Y = 600;