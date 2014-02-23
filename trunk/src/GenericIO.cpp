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

#include "GenericIO.h"

#include <cstring>
#include <ostream>

#include <boost/make_shared.hpp>

#include "raknet/BitStream.h"
#include "raknet/NetworkTypes.h"

#include "FileWrite.h"
#include "FileRead.h"
#include "PlayerInput.h"

// -------------------------------------------------------------------------------------------------
//							File Output Class
// -------------------------------------------------------------------------------------------------

class FileOut : public GenericOut
{
	public:
		FileOut(boost::shared_ptr<FileWrite> file) : mFile(file)
		{

		}

	private:
		virtual void byte(const unsigned char& data)
		{
			mFile->writeByte(data);
		}

		virtual void boolean(const bool& data)
		{
			mFile->writeByte(data);
		}

		virtual void uint32(const unsigned int& data)
		{
			mFile->writeUInt32(data);
		}

		virtual void number(const float& data)
		{
			mFile->writeFloat(data);
		}

		virtual void string(const std::string& string)
		{
			uint32(string.size());
			mFile->write(string.data(), string.size());
		}


		virtual unsigned int tell() const
		{
			return mFile->tell();
		}

		virtual void seek(unsigned int pos) const
		{
			mFile->seek(pos);
		}

		virtual void array(const char* data, unsigned int length)
		{
			mFile->write(data, length);
		}

		boost::shared_ptr<FileWrite> mFile;
};

// -------------------------------------------------------------------------------------------------
//							File Input Class
// -------------------------------------------------------------------------------------------------

class FileIn : public GenericIn
{
	public:
		FileIn(boost::shared_ptr<FileRead> file) : mFile(file)
		{

		}

	private:
		virtual void byte(unsigned char& data)
		{
			data = mFile->readByte();
		}

		virtual void boolean(bool& data)
		{
			data = mFile->readByte();
		}

		virtual void uint32(unsigned int& data)
		{
			data = mFile->readUInt32();
		}

		virtual void number(float& data)
		{
			data = mFile->readFloat();
		}

		virtual void string(std::string& string)
		{
			/// \todo this might be bad performance wise
			unsigned int ts;
			uint32(ts);

			string.resize(ts);
			for(unsigned int i = 0; i < ts; ++i)
			{
				unsigned char nc;
				byte(nc);
				string[i] = nc;
			}
		}


		virtual unsigned int tell() const
		{
			return mFile->tell();
		}

		virtual void seek(unsigned int pos) const
		{
			mFile->seek(pos);
		}

		virtual void array(char* data, unsigned int length)
		{
			mFile->readRawBytes(data, length);
		}

		boost::shared_ptr<FileRead> mFile;
};


// -------------------------------------------------------------------------------------------------
//							Bitstream Output Class
// -------------------------------------------------------------------------------------------------

class NetworkOut : public GenericOut
{
	public:
		NetworkOut(RakNet::BitStream* stream) : mStream(stream)
		{

		}

	private:
		virtual void byte(const unsigned char& data)
		{
			mStream->Write(data);
		}

		virtual void boolean(const bool& data)
		{
			mStream->Write(data);
		}

		virtual void uint32(const unsigned int& data)
		{
			mStream->Write(data);
		}

		virtual void number(const float& data)
		{
			mStream->Write(data);
		}

		virtual void string(const std::string& string)
		{
			uint32(string.size());
			mStream->Write(string.c_str(), string.size());
		}


		virtual unsigned int tell() const
		{
			return mStream->GetNumberOfBitsUsed();
		}

		virtual void seek(unsigned int pos) const
		{
			mStream->SetWriteOffset(pos);
		}

		virtual void array(const char* data, unsigned int length)
		{
			mStream->Write(data, length);
		}

		RakNet::BitStream* mStream;
};

// -------------------------------------------------------------------------------------------------
//							Bistream Input Class
// -------------------------------------------------------------------------------------------------

class NetworkIn : public GenericIn
{
	public:
		NetworkIn(RakNet::BitStream* stream) : mStream(stream)
		{

		}

	private:
		virtual void byte(unsigned char& data)
		{
			mStream->Read(data);
		}

		virtual void boolean(bool& data)
		{
			mStream->Read(data);
		}

		virtual void uint32( unsigned int& data)
		{
			mStream->Read(data);
		}

		virtual void number( float& data)
		{
			mStream->Read(data);
		}

		virtual void string( std::string& string)
		{
			/// \todo this might be bad performance wise
			unsigned int ts;
			uint32(ts);

			string.resize(ts);
			for(unsigned int i = 0; i < ts; ++i)
			{
				unsigned char nc;
				byte(nc);
				string[i] = nc;
			}
		}


		virtual unsigned int tell() const
		{
			return mStream->GetReadOffset();
		}

		virtual void seek(unsigned int pos) const
		{
			mStream->ResetReadPointer();
			mStream->IgnoreBits(pos);
		}

		virtual void array( char* data, unsigned int length)
		{
			mStream->Read(data, length);
		}

		RakNet::BitStream* mStream;
};

// -------------------------------------------------------------------------------------------------
//							File Output Class
// -------------------------------------------------------------------------------------------------

class StreamOut : public GenericOut
{
	public:
		StreamOut(std::ostream& stream) : mStream(stream)
		{

		}

	private:
		virtual void byte(const unsigned char& data)
		{
			mStream << data << "\n";
		}

		virtual void boolean(const bool& data)
		{
			mStream << data << "\n";
		}

		virtual void uint32(const unsigned int& data)
		{
			mStream << data << "\n";
		}

		virtual void number(const float& data)
		{
			mStream << data << "\n";
		}

		virtual void string(const std::string& string)
		{
			mStream << string << "\n";
		}

		/// currently not supported by StreamOut
		virtual unsigned int tell() const
		{
			return -1;
		}

		virtual void seek(unsigned int pos) const
		{
		}

		virtual void array(const char* data, unsigned int length)
		{
			std::string stringed(data, length);
			mStream << stringed << "\n";
		}

		std::ostream& mStream;
};



// -------------------------------------------------------------------------------------------------
//							Factory Functions
// -------------------------------------------------------------------------------------------------

boost::shared_ptr< GenericOut > createGenericWriter(boost::shared_ptr<FileWrite> file)
{
	return boost::make_shared< FileOut > (file);
}

boost::shared_ptr< GenericOut > createGenericWriter(RakNet::BitStream* stream)
{
	return boost::make_shared< NetworkOut > (stream);
}

boost::shared_ptr< GenericOut > createGenericWriter(std::ostream& stream)
{
	return boost::shared_ptr< StreamOut > ( new StreamOut(stream) );
}

boost::shared_ptr< GenericIn > createGenericReader(boost::shared_ptr<FileRead> file)
{
	return boost::make_shared< FileIn > (file);
}

boost::shared_ptr< GenericIn > createGenericReader(RakNet::BitStream* stream)
{
	return boost::make_shared< NetworkIn > (stream);
}

// -------------------------------------------------------------------------------------------------
//			Default generic implementations
// -------------------------------------------------------------------------------------------------

/*	Several instantiations of serializer functions are made so these can be used without
	further user actions. These are the instantiations for the standard types
		* unsigned char
		* bool
		* unsigned int
		* float
		* string
	Furthermore, some types used in BlobbyVolley get their serialisation algorithms here
		* Color
		* PlayerInput
		* PlayerSide
*/

// these templates help to avoid boilderplate code

#define GENERATE_STD_SERIALIZER_OUT(type)		\
	template<>									\
	void predifined_serializer<type>::serialize(GenericOut& io, const type& value)

#define GENERATE_STD_SERIALIZER_IN(type)		\
	template<>									\
	void predifined_serializer<type>::serialize(GenericIn& io, type& value)

#define GENERATE_STD_SERIALIZER(type, func)		\
	GENERATE_STD_SERIALIZER_OUT(type) { io.func(value); };	\
	GENERATE_STD_SERIALIZER_IN(type) { io.func(value); };


namespace detail
{
	// std implementations
	GENERATE_STD_SERIALIZER(unsigned char, byte);
	GENERATE_STD_SERIALIZER(unsigned int, uint32);
	GENERATE_STD_SERIALIZER(bool, boolean);
	GENERATE_STD_SERIALIZER(float, number);
	GENERATE_STD_SERIALIZER(std::string, string);

	// Blobby types


	GENERATE_STD_SERIALIZER_OUT(Color)
	{
		io.uint32(value.toInt());
	}

	GENERATE_STD_SERIALIZER_IN(Color)
	{
		unsigned int target;
		io.uint32(target);
		value = Color(target);
	}

	GENERATE_STD_SERIALIZER_OUT(PlayerInput)
	{
		io.uint32(value.getAll());
	}

	GENERATE_STD_SERIALIZER_IN(PlayerInput)
	{
		unsigned int target;
		io.uint32(target);
		value.set(target);
	}

	GENERATE_STD_SERIALIZER_OUT(PlayerSide)
	{
		io.uint32(value);
	}

	GENERATE_STD_SERIALIZER_IN(PlayerSide)
	{
		unsigned int target;
		io.uint32(target);
		value = (PlayerSide)target;
	}

	GENERATE_STD_SERIALIZER_OUT(PlayerID)
	{
		io.uint32(value.binaryAddress);
		io.uint32(value.port);
	}

	GENERATE_STD_SERIALIZER_IN(PlayerID)
	{
		io.uint32(value.binaryAddress);
		unsigned int port;
		io.uint32(port);
		value.port = port;
	}

}


