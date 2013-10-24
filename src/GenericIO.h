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

#include <string>
#include <iosfwd>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "GenericIOFwd.h"
#include "GenericIODetail.h"
#include "Global.h"

// forward declarations

class FileWrite;
class FileRead;

namespace RakNet
{
	class BitStream;
}


// Factory functions
/// creates a generic writer that writes to a file
boost::shared_ptr< GenericOut > createGenericWriter(boost::shared_ptr<FileWrite> file);
/// creates a generic writer that writes to a BitStream
boost::shared_ptr< GenericOut > createGenericWriter(RakNet::BitStream* stream);
/// creates a generic writer that writes hman readable to a stream
/// currently, there is no corresponding reader because this is mostly for debugging purposes
boost::shared_ptr< GenericOut > createGenericWriter(std::ostream& stream);

/// creates a generic reader that reads from a file
boost::shared_ptr< GenericIn > createGenericReader(boost::shared_ptr<FileRead> file);
/// creates a generic reader that reads from a BitStream
boost::shared_ptr< GenericIn > createGenericReader(RakNet::BitStream* stream);


// GenericIO class template

/*! \class GenericIO
	\brief Class template that abstracts IO
	\details This class abstract IO to/from different sources. Current implementations are
			File and BitStream input/output. The template parameter tag decides wether
			the actual instance is an input or an output object. This design ensure that
			input and output have exactly the same interface and enables writing algorithms
			that read and write data with exactly the same code, reducing the chance of
			errors.
			This class derives from boost::noncopyable, which seems a reasonable choice for these
			IO classes. Having different GenericIO objects which read/write from/to the same source/target
			just makes things more complicated and error prone.
*/
template<class tag>
class GenericIO : public boost::noncopyable
{
	public:
		/// virtual d'tor to ensure correct cleanup
		virtual ~GenericIO() { };

		/// reads/writes one byte
		virtual void byte ( typename detail::conster<tag, unsigned char>::type data) = 0;

		/// reads/writes one boolean.
		virtual void boolean( typename detail::conster<tag, bool>::type data) = 0;

		/// reads/writes on 32 bit unsigned integer
		virtual void uint32( typename detail::conster<tag, unsigned int>::type data) = 0;
		/// reads/writes a floating point number
		virtual void number( typename detail::conster<tag, float>::type data) = 0;
		/// reads/writes a character string.
		virtual void string( typename detail::conster<tag, std::string>::type string) = 0;
		/// reads/writes a character array of certain length
		virtual void array ( typename detail::conster<tag, char*>::type data, unsigned int length) = 0;


		/// returns the current read/write position
		virtual unsigned int tell() const = 0;
		/// sets the current read/write position
		/// \attention Use for pos only values you have received
		///			from a prior call to tell of the same instance
		///			of GenericIO as these positions are not
		///			guaranteed to match for different source/target
		///			types.
		virtual void seek(unsigned int pos) const = 0;

		// generic implementation

		/// this is a nonvirtual generic function which can be used to write or read arbitrary (supported)
		///	types. these types are serialized using the methods above for the primitive types.
		/// supported values for \p T are all the basic types which can be written directly,
		/// PlayerInput, PlayerSide and Color. If T can be serialized, this function can serialize
		///	containers of T provided they have the following methods:
		///	 * begin()
		///  * end()
		///	 * size()
		///  * resize()
		/// Additional user types can be serialized if the user defines the appropriate methods
		///	in UserSerializer<T>.
		template<class T>
		void generic( typename detail::conster<tag, T>::type data )
		{
			// thats a rather complicated template construct. It uses the serialize_dispatch template
			// with working type T, boost::true_type or boost::false_type as init depending wether the
			// supplied type T has a default implementation associated (as determined by the
			// has_default_io_implementation template).
			// the second parameter is a bool which is true if the type offeres a container interface
			// and false otherwise (determined by the is_container_type template)
			// depending on the second two template parameters, serialize_dispatch is either
			// derived from detail::predifined_serializer (when init is boost::true_type)
			// or UserSerializer if init is boost::false_type and container is false.
			// if it is a container type, the partial template specialisation foudn below is
			// used to serialize that template.
			detail::serialize_dispatch<T,
										typename detail::has_default_io_implementation<T>::type,
										detail::is_container_type<T>::value >::serialize(*this, data);
		}


		typedef tag tag_type;
};

/*! \def USER_SERIALIZER_IMPLEMENTATION_HELPER
	\brief Helper macro for autogenerated user serializers
	\details use like this:
	\code
	USER_SERIALIZER_IMPLEMENTATION_HELPER( \p type )
	{
		generic serialisation algorithm for both input and output. Variable \p io
		contains the GenericIO object, variable value the \p type object.
	}
	\endcode
	remember to use generic\< \p type\> like this:
	\code
		io.template generic\< \p type\>(value)
	\endcode
	otherwise, the compiler won't recognise generic as a template function.

	\example USER_SERIALIZER_IMPLEMENTATION_HELPER(int)
	{
		io.uint32(value);
	}
*/
#define USER_SERIALIZER_IMPLEMENTATION_HELPER( UD_TYPE )											\
template<class TAG>																					\
void doSerialize##UD_TYPE(GenericIO<TAG>&, typename detail::conster<TAG, UD_TYPE>::type value);	\
template<>																							\
void UserSerializer<UD_TYPE>::serialize( GenericOut& out, const UD_TYPE& value)						\
{																									\
	doSerialize##UD_TYPE(out, value);																\
}																									\
template<>																							\
void UserSerializer<UD_TYPE>::serialize( GenericIn& in, UD_TYPE& value)								\
{																									\
	doSerialize##UD_TYPE(in, value);																\
}																									\
template<class TAG>																					\
void doSerialize##UD_TYPE(GenericIO<TAG>& io, typename detail::conster<TAG, UD_TYPE>::type value)


// -------------------------------------------------------------------------------------------------
//         						Implementation detail
// -------------------------------------------------------------------------------------------------


namespace detail
{
	// serialisation algorithm for container types:
	//  read/write size of the container.
	//  if reading, resize container to fit
	//  iterate over all elements and read/write


	template<class T>
	struct serialize_dispatch<T, boost::false_type, true>
	{
		static void serialize( GenericOut& out, const T& list)
		{
			out.uint32( list.size() );
			for(typename T::const_iterator i = list.begin(); i != list.end(); ++i)
			{
				out.generic<typename T::value_type>( *i );
			}
		}

		static void serialize( GenericIn& in, T& list)
		{
			unsigned int size;

			in.uint32( size );
			list.resize( size );

			for(typename T::iterator i = list.begin(); i != list.end(); ++i)
			{
				in.generic<typename T::value_type>( *i );
			}
		}
	};
}
