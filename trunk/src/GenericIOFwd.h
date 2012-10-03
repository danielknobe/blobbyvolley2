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

/** \file GenericIOFwd.h
	
	Including this header will declare the types 
	\p GenericIn an \p GenericOut so they can be passed
	as parameters etc. If you wan't to use these classes
	for actual io, you have to include GenericIO.h.
	Including this header spares the compiler from having to
	parse all the template stuff, though.
*/

/// base class template for all GenericIO operations. do not use this class directly. 
/// use the provided typedefs GenericIn and GenericOut instead.
template<class tag>
class GenericIO;

// implementation detail, if you just use GenericIO, ignore this namespace ;)
namespace detail
{
	struct READER_TAG;
	struct WRITER_TAG;
}

// the two typedefs
/// Base class for generic input operations.
typedef GenericIO<detail::READER_TAG> GenericIn;
/// Base class for generic output operations.
typedef GenericIO<detail::WRITER_TAG> GenericOut;


/// to make GenericIO support a user defined type, you have to implement
///	the two functions in this template for that type.
template<class T>
struct UserSerializer
{
	static void serialize( GenericOut& out, const T& value);
	static void serialize( GenericIn& in, T& value);
};
