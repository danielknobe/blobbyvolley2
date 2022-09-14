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

#include "GenericIOFwd.h"

#include <string>
#include <type_traits>

#include "Global.h"

struct PlayerInput;
struct Color;
struct DuelMatchState;
struct PlayerID;

namespace detail
{
	// this class indicates the GenericIO<READER_TAG> is an input handler
	struct READER_TAG{};
	// this class indicates the GenericIO<WRITER_TAG> is an output handler
	struct WRITER_TAG{};


	// helper class to make parameter types
	/* \class conster
		helper class for parameter types.
		This class is used to adapt the parameters of the read/write methods,
		making them const references for writing and reference for reading.
		The partial specialisations ensure that behaviour.
	*/
	template<class tag, class type>
	struct conster
	{
	};

	// partial specialisations which generate the conster logic
	template<class T>
	struct conster<detail::READER_TAG, T>
	{
		typedef T& type;
	};

	template<class T>
	struct conster<detail::READER_TAG, T*>
	{
		typedef T* type;
	};


	template<class T>
	struct conster<detail::WRITER_TAG, T>
	{
		typedef const T& type;
	};

	template<class T>
	struct conster<detail::WRITER_TAG, T*>
	{
		typedef const T* type;
	};


	// helper classes to determine which io algorithm to use for which type

	// if any template specialisation of this class inherits from std::true_type,
	// this means that the io algorithm uses a predefined_serializer.
	template<class T>
	struct has_default_io_implementation : public std::false_type
	{
	};

	// specialisations for default types
	template<>
	struct has_default_io_implementation<bool> : public std::true_type
	{
	};

	template<>
	struct has_default_io_implementation<unsigned char> : public std::true_type
	{
	};

	template<>
	struct has_default_io_implementation<unsigned int> : public std::true_type
	{
	};

	template<>
	struct has_default_io_implementation<float> : public std::true_type
	{
	};

	template<>
	struct has_default_io_implementation<std::string> : public std::true_type
	{
	};

	template<>
	struct has_default_io_implementation<PlayerSide> : public std::true_type
	{
	};

	template<>
	struct has_default_io_implementation<Color> : public std::true_type
	{
	};

	template<>
	struct has_default_io_implementation<PlayerInput> : public std::true_type
	{
	};

	template<>
	struct has_default_io_implementation<PlayerID> : public std::true_type
	{
	};


	// this class uses SFINAE to determine whether a type can be used as a container type, i.e.
	// 	provided the methods size and begin
	template<class T>
	struct is_container_type
	{
		struct yes {char p[1];};
		struct no{char p[2];};

		static_assert( sizeof(yes) != sizeof(no), "yes and no type need different sizes" );

		template<typename C>
		static yes& test_size( int arg =  (C()).size());

		template<typename>
		static no& test_size(...);

		template<typename C>
		static decltype(std::declval<C>().resize(5), yes())& test_resize( int );

		template<typename>
		static no& test_resize(...);

		template<typename C>
		static yes& test_begin( typename C::iterator* it = &(C()).begin());

		template<typename>
		static no& test_begin(...);

		// remove reference from type
		typedef typename std::remove_reference<T>::type simplified_type;

		static const bool has_size = sizeof(test_size<simplified_type>(0)) == sizeof(yes);
		static const bool has_begin = sizeof(test_begin<simplified_type>(0)) == sizeof(yes);
		static const bool has_resize = sizeof(test_resize<simplified_type>(0)) == sizeof(yes);

		// this is only true if size and begin functions exist.
		static const bool value = has_size && has_begin;
	};


	/*! \class serialize_dispatch
		\brief manages which generic implementation is used to serialize certain types
		\details uses partial specialisation with the parameters init and container.
				init has to be std::true_type or std::false_type.
	*/
	template<class T, class init, bool container>
	struct serialize_dispatch;


	template<class T>
	struct predefined_serializer
	{
		static void serialize( GenericOut& out, const T& c);
		static void serialize( GenericIn& in, T& c);
	};

	// inserts the methods from predefined_serializer, which are forward declared and
	//  implemented in GenericIO.cpp. This happens when init is true_type
	template<class T, bool b>
	struct serialize_dispatch<T, std::true_type, b> : public predefined_serializer<T>
	{

	};

	// uses a UserSerializer<T>
	// the user has to implement the UserSerializer<T>::serialize(GenericOut, const T&) and UserSerializer<T>::serialize(GenericIn, T&)
	// somewhere, otherwise a link error happens.
	// User serializers are used when there is no default implementation and the type does not provide a container
	// interface.
	template<class T>
	struct serialize_dispatch<T, std::false_type, false> : public UserSerializer<T>
	{

	};

}
