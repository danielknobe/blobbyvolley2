#define BOOST_TEST_MODULE FileAbstraction
#include <boost/test/unit_test.hpp>

#include "FileRead.h"
#include "FileWrite.h"
#include "FileSystem.h"
#include <iostream>
#include <cstring>
#include "GenericIO.h"
#include "InputSource.h"
#include "DuelMatchState.h"
#include "raknet/BitStream.h"
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/make_shared.hpp>
#include <vector>
#include <list>
#include <deque>

#define TEST_EXECUTION_PATH "C:\\Dokumente und Einstellungen\\Erik\\Eigene Dateien\\Blobby Volley 2\\test"

//#define DISABLE_COMPILATION_TEST

// helper
void init_Physfs() 
{
	static bool initialised = false;
	if(!initialised) 
	{
		std::cout << "initialising physfs to " << TEST_EXECUTION_PATH << "\n";
		static FileSystem fs( TEST_EXECUTION_PATH );
		fs.setWriteDir(".");
		initialised = true;
	}
}

void generic_io_types_test_f(boost::shared_ptr<GenericIn> in, boost::shared_ptr<GenericOut> out);
void generic_io_types_test_generics_f(boost::shared_ptr<GenericIn> in, boost::shared_ptr<GenericOut> out);
template<class T>
void generic_io_types_test_vector_f(boost::shared_ptr<GenericIn> in, boost::shared_ptr<GenericOut> out);
void generic_io_seek_tell_f(boost::shared_ptr<GenericIn> in, boost::shared_ptr<GenericOut> out);
void generic_io_types_test_special_f(boost::shared_ptr<GenericIn> in, boost::shared_ptr<GenericOut> out);

#define CHECK_EXCEPTION_SAFETY(expr, excp) 	try { 	\
											BOOST_TEST_CHECKPOINT("trying " #expr);		\
											expr; \
											BOOST_ERROR(#expr " does not cause " #excp " to be thrown");\
											} \
											catch(excp& e) {\
												 \
											} catch (std::exception& exp) 	\
											{								\
												BOOST_ERROR(std::string("unexpected exception ") + exp.what() + "instead of " #excp " caught from  "#expr);						\
											};


// Tests of common FileSystem functions
// test init

BOOST_AUTO_TEST_SUITE( GenericIOTest )

BOOST_AUTO_TEST_CASE( generic_io_create )
{
	init_Physfs();
	
	boost::shared_ptr<FileWrite> write = boost::make_shared<FileWrite>("test.tmp");
	boost::shared_ptr<FileRead> read = boost::make_shared<FileRead>("test.tmp");
	boost::shared_ptr<RakNet::BitStream> stream = boost::make_shared<RakNet::BitStream>();
	
	// no checks, just let this pass without exceptions
	createGenericReader( read );
	createGenericWriter( write );
	createGenericReader( stream );
	createGenericWriter( stream );
}

BOOST_AUTO_TEST_CASE( generic_io_types_test_file )
{
	boost::shared_ptr<FileWrite> write = boost::make_shared<FileWrite>("test.tmp");
	boost::shared_ptr<FileRead> read = boost::make_shared<FileRead>("test.tmp");
	
	boost::shared_ptr<GenericIn>  inf = createGenericReader( read );
	boost::shared_ptr<GenericOut>  outf = createGenericWriter( write );
	generic_io_types_test_f( inf, outf);
};


BOOST_AUTO_TEST_CASE( generic_io_types_test_stream )
{
	boost::shared_ptr<RakNet::BitStream> stream = boost::make_shared<RakNet::BitStream>();
	boost::shared_ptr<GenericIn>  ins = createGenericReader( stream );
	boost::shared_ptr<GenericOut>  outs = createGenericWriter( stream );
	generic_io_types_test_f( ins, outs);
};

BOOST_AUTO_TEST_CASE( generic_io_generic_types_file )
{
	boost::shared_ptr<FileWrite> write = boost::make_shared<FileWrite>("test.tmp");
	boost::shared_ptr<FileRead> read = boost::make_shared<FileRead>("test.tmp");
	
	boost::shared_ptr<GenericIn>  inf = createGenericReader( read );
	boost::shared_ptr<GenericOut>  outf = createGenericWriter( write );
	generic_io_types_test_generics_f( inf, outf);
};


BOOST_AUTO_TEST_CASE( generic_io_generic_types_stream )
{
	boost::shared_ptr<RakNet::BitStream> stream = boost::make_shared<RakNet::BitStream>();
	
	boost::shared_ptr<GenericIn>  ins = createGenericReader( stream );
	boost::shared_ptr<GenericOut>  outs = createGenericWriter( stream );
	generic_io_types_test_generics_f( ins, outs);
};

BOOST_AUTO_TEST_CASE( generic_io_seek_tell )
{
	boost::shared_ptr<FileWrite> write = boost::make_shared<FileWrite>("test.tmp");
	boost::shared_ptr<FileRead> read = boost::make_shared<FileRead>("test.tmp");
	
	boost::shared_ptr<GenericIn>  inf = createGenericReader( read );
	boost::shared_ptr<GenericOut>  outf = createGenericWriter( write );
	generic_io_seek_tell_f( inf, outf);

	boost::shared_ptr<RakNet::BitStream> stream = boost::make_shared<RakNet::BitStream>();
	
	boost::shared_ptr<GenericIn>  ins = createGenericReader( stream );
	boost::shared_ptr<GenericOut>  outs = createGenericWriter( stream );
	generic_io_seek_tell_f( ins, outs);
};

BOOST_AUTO_TEST_CASE( generic_io_generic_types_vector )
{
	boost::shared_ptr<FileWrite> write = boost::make_shared<FileWrite>("test.tmp");
	boost::shared_ptr<FileRead> read = boost::make_shared<FileRead>("test.tmp");
	
	boost::shared_ptr<GenericIn>  inf = createGenericReader( read );
	boost::shared_ptr<GenericOut>  outf = createGenericWriter( write );
	generic_io_types_test_vector_f<std::vector<unsigned char> >( inf, outf );
	generic_io_types_test_vector_f<std::list<unsigned char> >( inf, outf );
	generic_io_types_test_vector_f<std::deque<unsigned char> >( inf, outf );

	boost::shared_ptr<RakNet::BitStream> stream = boost::make_shared<RakNet::BitStream>();
	
	boost::shared_ptr<GenericIn>  ins = createGenericReader( stream );
	boost::shared_ptr<GenericOut>  outs = createGenericWriter( stream );
	generic_io_types_test_vector_f<std::vector<unsigned char> >( ins, outs );
	generic_io_types_test_vector_f<std::list<unsigned char> >( ins, outs );
	generic_io_types_test_vector_f<std::deque<unsigned char> >( ins, outs );
};


BOOST_AUTO_TEST_CASE( generic_io_special_types )
{
	boost::shared_ptr<FileWrite> write = boost::make_shared<FileWrite>("test.tmp");
	boost::shared_ptr<FileRead> read = boost::make_shared<FileRead>("test.tmp");
	
	boost::shared_ptr<GenericIn>  inf = createGenericReader( read );
	boost::shared_ptr<GenericOut>  outf = createGenericWriter( write );
	generic_io_types_test_special_f( inf, outf);

	boost::shared_ptr<RakNet::BitStream> stream = boost::make_shared<RakNet::BitStream>();
	
	boost::shared_ptr<GenericIn>  ins = createGenericReader( stream );
	boost::shared_ptr<GenericOut>  outs = createGenericWriter( stream );
	generic_io_types_test_special_f( ins, outs);
};



BOOST_AUTO_TEST_SUITE_END()


void generic_io_types_test_f(boost::shared_ptr<GenericIn> in, boost::shared_ptr<GenericOut> out)
{
	// writing
	const unsigned char byte1 = 6;
	const unsigned char byte2 = 222;
	const unsigned char byte3 = rand();
	out->byte( byte1 );
	out->byte( byte2 );
	out->byte( byte3 );
	
	out->boolean(true);
	out->boolean(false);
	
	const unsigned int int1 = 8;
	const unsigned int int2 = 123456;
	const unsigned int int3 = rand();
	
	out->uint32( int1 );
	out->uint32( int2 );
	out->uint32( int3 );
	
	const float f1 = 1.54f;
	const float f2 = -0.785f;
	const float f3 = (float)rand() / RAND_MAX * 1000;
	const float f4 = 1.0e10f;
	
	out->number( f1 );
	out->number( f2 );
	out->number( f3 );
	out->number( f4 );
	
	std::string str1 = "hello world";
	std::string str2 = std::string("s w \0 in it", 11);
	std::string str3 = std::string(1000, 'l');
	
	out->string( str1 );
	out->string( str2 );
	out->string( str3 );
	
	out->array( str1.c_str(), str1.size() );
	out->array( str2.c_str(), str2.size() );
	out->array( str3.c_str(), str3.size() );
	
	
	// reading
	unsigned char bytec;
	in->byte(bytec);
	BOOST_CHECK_EQUAL( bytec, byte1 );
	in->byte(bytec);
	BOOST_CHECK_EQUAL( bytec, byte2 );
	in->byte(bytec);
	BOOST_CHECK_EQUAL( bytec, byte3 );
	
	bool boolc;
	in->boolean(boolc);
	BOOST_CHECK_EQUAL(boolc, true);
	in->boolean(boolc);
	BOOST_CHECK_EQUAL(boolc, false);
	
	unsigned int intc;
	in->uint32( intc );
	BOOST_CHECK_EQUAL(intc, int1);
	in->uint32( intc );
	BOOST_CHECK_EQUAL(intc, int2);
	in->uint32( intc );
	BOOST_CHECK_EQUAL(intc, int3);
	
	float fc;
	in->number(fc);
	BOOST_CHECK_EQUAL(fc, f1);
	in->number(fc);
	BOOST_CHECK_EQUAL(fc, f2);
	in->number(fc);
	BOOST_CHECK_EQUAL(fc, f3);
	in->number(fc);
	BOOST_CHECK_EQUAL(fc, f4);
	
	std::string stringc;
	in->string(stringc);
	BOOST_CHECK_EQUAL(stringc, str1);
	in->string(stringc);
	BOOST_CHECK_EQUAL(stringc, str2);
	in->string(stringc);
	BOOST_CHECK_EQUAL(stringc, str3);
	
	boost::scoped_array<char> ar1(new char[str1.size()]);
	in->array(ar1.get(), str1.size());
	BOOST_CHECK( memcmp(ar1.get(), str1.data(), str1.size()) == 0);
	boost::scoped_array<char> ar2(new char[str2.size()]);
	in->array(ar2.get(), str2.size());
	BOOST_CHECK( memcmp(ar2.get(), str2.data(), str2.size()) == 0);
	boost::scoped_array<char> ar3(new char[str3.size()]);
	in->array(ar3.get(), str3.size());
	BOOST_CHECK( memcmp(ar3.get(), str3.data(), str3.size()) == 0);
}

void generic_io_types_test_generics_f(boost::shared_ptr<GenericIn> in, boost::shared_ptr<GenericOut> out)
{
	#ifndef DISABLE_COMPILATION_TEST
	// writing
	// these are not really run time tests... 
	// it is tested here wether this code compiles.
	const unsigned char byte = 6;
	out->generic<unsigned char>( byte );
	out->generic<bool>(true);
	
	const unsigned int intv = 8;
	
	out->generic<unsigned int>( intv );
	
	const float fv = 1.54f;
	
	out->generic<float>( fv );
	
	std::string str1 = "hello world";
	out->generic<std::string>( str1 );
	
	// reading
	unsigned char bytec;
	in->generic<unsigned char>(bytec);
	BOOST_CHECK_EQUAL( bytec, byte );
	
	bool boolc;
	in->generic<bool>(boolc);
	BOOST_CHECK_EQUAL(boolc, true);
	
	unsigned int intc;
	in->generic<unsigned int>( intc );
	BOOST_CHECK_EQUAL(intc, intv);
	
	float fc;
	in->generic<float>(fc);
	BOOST_CHECK_EQUAL(fc, fv);
	
	std::string stringc;
	in->generic<std::string>(stringc);
	#endif
}

template<class T>
void generic_io_types_test_vector_f(boost::shared_ptr<GenericIn> in, boost::shared_ptr<GenericOut> out)
{
	// writing
	T basevec;
	for(int i=0; i < 100; ++i)
	{
		basevec.push_back(rand());
	}
	
	const T vector_of_bytes(basevec);
	
	out->generic<T>( vector_of_bytes );
	
	// reading
	// make sure at least one entry of basevec is changed.
	*basevec.begin() = ~*basevec.begin();
	in->generic< T >(basevec);
	typename T::const_iterator j = vector_of_bytes.begin();
	for(typename T::iterator i = basevec.begin(); i != basevec.end(); ++i, ++j)
	{
		BOOST_CHECK_EQUAL( *i, *j );
	}
}

void generic_io_seek_tell_f(boost::shared_ptr<GenericIn> in, boost::shared_ptr<GenericOut> out)
{
	out->uint32(5);
	int pos = out->tell();
	out->uint32(76);
	out->uint32(13);
	out->seek(pos);
	out->uint32(42);
	
	unsigned int val;
	in->uint32(val);
	pos = in->tell();
	in->uint32(val);
	
	BOOST_CHECK_EQUAL(val, 42);
	
	in->uint32(val);
	in->seek(pos);
	
	in->uint32(val);
	BOOST_CHECK_EQUAL(val, 42);
}

void generic_io_types_test_special_f(boost::shared_ptr<GenericIn> in, boost::shared_ptr<GenericOut> out)
{
	const PlayerInput pi(true, false, false);
	
	out->generic<PlayerInput>(pi);
	
	const PlayerSide ps1 = LEFT_PLAYER;
	const PlayerSide ps2 = RIGHT_PLAYER;
	const PlayerSide ps3 = NO_PLAYER;
	out->generic<PlayerSide>(ps1);
	out->generic<PlayerSide>(ps2);
	out->generic<PlayerSide>(ps3);
	
	const Color col(255, 127, 127);
	out->generic<Color>(col);
	
	DuelMatchState dlms;
	dlms.logicState.leftScore = 12;
	dlms.logicState.rightScore = 6;
	dlms.logicState.leftSquish = 8;
	dlms.logicState.rightSquish = 3;
	dlms.logicState.servingPlayer = RIGHT_PLAYER;
	dlms.worldState.blobPosition[LEFT_PLAYER] = Vector2(65, 12);
	dlms.worldState.blobPosition[RIGHT_PLAYER] = Vector2(465, 120);
	dlms.worldState.blobVelocity[LEFT_PLAYER] = Vector2(5.2f, 1.f);
	dlms.worldState.blobVelocity[RIGHT_PLAYER] = Vector2(-5.2f, 1.f);
	dlms.worldState.ballVelocity = Vector2(8.2f, 12.f);
	dlms.worldState.ballPosition = Vector2(122.7f, 765.f);
	dlms.worldState.isBallValid = true;
	dlms.worldState.isGameRunning = false;
	dlms.worldState.ballAngularVelocity = 7.43f;
	dlms.worldState.playerInput[LEFT_PLAYER] = pi;
	dlms.worldState.playerInput[RIGHT_PLAYER] = PlayerInput(false, false, true);
	
	out->generic<DuelMatchState>(dlms);
	
	
	PlayerInput piv;
	in->generic<PlayerInput>(piv);
	
	/// \todo we can not use CHECK_EQUAL here because we don't have a pretty printer for PlayerInput
	BOOST_CHECK( pi == piv );
	
	PlayerSide psv;
	in->generic<PlayerSide>(psv);
	BOOST_CHECK_EQUAL(ps1, psv);
	in->generic<PlayerSide>(psv);
	BOOST_CHECK_EQUAL(ps2, psv);
	in->generic<PlayerSide>(psv);
	BOOST_CHECK_EQUAL(ps3, psv);
	
	Color colv;
	in->generic<Color>(colv);
	BOOST_CHECK(col == colv);
	
	DuelMatchState dlmsv;
	in->generic<DuelMatchState> (dlmsv);
	BOOST_CHECK( dlmsv == dlms );
	// sub-object test for better error localisation
	BOOST_CHECK( dlmsv.logicState == dlms.logicState );
	BOOST_CHECK( dlmsv.worldState == dlms.worldState );
}

