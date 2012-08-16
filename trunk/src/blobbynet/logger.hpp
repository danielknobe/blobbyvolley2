/*=============================================================================
blobNet
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
 * Macros for primitive plattform independend logging and debugging
 *
 * This header include the the logging macros for debugging. 
 * You can log information by calling the LOG(class, title, message) macro.
 * There are the following log-modes available which you can set by changing the 
 * definition of the LOGGER_MODE macro:
 */
#define LOGGER_OFF 0     // Debugging is off and all overhead is removed
#define LOGGER_CONSOLE 1 // The debugging information is printed to the std::out

/**
 * Set the mode here:
 */
#define LOGGER_MODE LOGGER_CONSOLE

/**
 * Implemenation
 */
#if LOGGER_MODE == LOGGER_OFF
#define LOG(class, title, message) 
#endif

#if LOGGER_MODE == LOGGER_CONSOLE
#include <iostream>
#include <time.h>
#include <string.h>
#define LOG(class, title, message) \
	{ \
		time_t timeRaw; \
		struct tm * timeInfo; \
		char* timeAsc; \
		time(&timeRaw); \
		timeInfo = localtime(&timeRaw); \
		timeAsc = asctime(timeInfo); \
		timeAsc[strlen(timeAsc)-1] = '\0'; \
		std::cout << timeAsc << " - " << title << ": " << message << std::endl; \
	}
#endif
