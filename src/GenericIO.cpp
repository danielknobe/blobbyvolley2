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
#include <utility>

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
		explicit FileOut(std::shared_ptr<FileWrite> file) : mFile(std::move(file))
		{

		}

	private:
		void byte(const unsigned char& data) override
		{
			mFile->writeByte(data);
		}

		void boolean(const bool& data) override
		{
			mFile->writeByte(data);
		}

		void uint32(const unsigned int& data) override
		{
			mFile->writeUInt32(data);
		}

		void number(const float& data) override
		{
			mFile->writeFloat(data);
		}

		void string(const std::string& string) override
		{
			uint32(string.size());
			mFile->write(string.data(), string.size());
		}


		unsigned int tell() const override
		{
			return mFile->tell();
		}

		void seek(unsigned int pos) const override
		{
			mFile->seek(pos);
		}

		void array(const char* data, unsigned int length) override
		{
			mFile->write(data, length);
		}

		std::shared_ptr<FileWrite> mFile;
};

// -------------------------------------------------------------------------------------------------
//							File Input Class
// -------------------------------------------------------------------------------------------------

class FileIn : public GenericIn
{
	public:
		explicit FileIn(std::shared_ptr<FileRead> file) : mFile(std::move(file))
		{

		}

	private:
		void byte(unsigned char& data) override
		{
			data = mFile->readByte();
		}

		void boolean(bool& data) override
		{
			data = mFile->readByte();
		}

		void uint32(unsigned int& data) override
		{
			data = mFile->readUInt32();
		}

		void number(float& data) override
		{
			data = mFile->readFloat();
		}

		void string(std::string& string) override
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


		unsigned int tell() const override
		{
			return mFile->tell();
		}

		void seek(unsigned int pos) const override
		{
			mFile->seek(pos);
		}

		void array(char* data, unsigned int length) override
		{
			mFile->readRawBytes(data, length);
		}

		std::shared_ptr<FileRead> mFile;
};


// -------------------------------------------------------------------------------------------------
//							Bitstream Output Class
// -------------------------------------------------------------------------------------------------

class NetworkOut : public GenericOut
{
	public:
		explicit NetworkOut(RakNet::BitStream* stream) : mStream(stream)
		{

		}

	private:
		void byte(const unsigned char& data) override
		{
			mStream->Write(data);
		}

		void boolean(const bool& data) override
		{
			mStream->Write(data);
		}

		void uint32(const unsigned int& data) override
		{
			mStream->Write(data);
		}

		void number(const float& data) override
		{
			mStream->Write(data);
		}

		void string(const std::string& string) override
		{
			uint32(string.size());
			mStream->Write(string.c_str(), string.size());
		}


		unsigned int tell() const override
		{
			return mStream->GetNumberOfBitsUsed();
		}

		void seek(unsigned int pos) const override
		{
			mStream->SetWriteOffset(pos);
		}

		void array(const char* data, unsigned int length) override
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
		explicit NetworkIn(RakNet::BitStream* stream) : mStream(stream)
		{

		}

	private:
		void byte(unsigned char& data) override
		{
			mStream->Read(data);
		}

		void boolean(bool& data) override
		{
			mStream->Read(data);
		}

		void uint32( unsigned int& data) override
		{
			mStream->Read(data);
		}

		void number( float& data) override
		{
			mStream->Read(data);
		}

		void string( std::string& string) override
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


		unsigned int tell() const override
		{
			return mStream->GetReadOffset();
		}

		void seek(unsigned int pos) const override
		{
			mStream->ResetReadPointer();
			mStream->IgnoreBits(pos);
		}

		void array( char* data, unsigned int length) override
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
		explicit StreamOut(std::ostream& stream) : mStream(stream)
		{

		}

	private:
		void byte(const unsigned char& data) override
		{
			mStream << data << "\n";
		}

		void boolean(const bool& data) override
		{
			mStream << data << "\n";
		}

		void uint32(const unsigned int& data) override
		{
			mStream << data << "\n";
		}

		void number(const float& data) override
		{
			mStream << data << "\n";
		}

		void string(const std::string& string) override
		{
			mStream << string << "\n";
		}

		/// currently not supported by StreamOut
		unsigned int tell() const override
		{
			return -1;
		}

		void seek(unsigned int pos) const override
		{
		}

		void array(const char* data, unsigned int length) override
		{
			std::string stringed(data, length);
			mStream << stringed << "\n";
		}

		std::ostream& mStream;
};



// -------------------------------------------------------------------------------------------------
//							Factory Functions
// -------------------------------------------------------------------------------------------------

std::shared_ptr< GenericOut > createGenericWriter(std::shared_ptr<FileWrite> file)
{
	return std::make_shared< FileOut > (std::move(file));
}

std::shared_ptr< GenericOut > createGenericWriter(RakNet::BitStream* stream)
{
	return std::make_shared< NetworkOut > (stream);
}

std::shared_ptr< GenericOut > createGenericWriter(std::ostream& stream)
{
	return std::make_shared< StreamOut > (stream);
}

std::shared_ptr< GenericIn > createGenericReader(std::shared_ptr<FileRead> file)
{
	return std::make_shared< FileIn > (std::move(file));
}

std::shared_ptr< GenericIn > createGenericReader(RakNet::BitStream* stream)
{
	return std::make_shared< NetworkIn > (stream);
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
		value.setAll(target);
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


