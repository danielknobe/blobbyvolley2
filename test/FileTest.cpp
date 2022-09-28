//#define BOOST_TEST_MODULE FileAbstraction
#include <boost/test/unit_test.hpp>

#include "io/FileRead.h"
#include "io/FileWrite.h"
#include "io/FileSystem.h"
#include <iostream>
#include <cstring>
#include <physfs.h>

#define TEST_EXECUTION_PATH "./test/"

// helper
static void init_Physfs()
{
	static bool initialised = false;
	if(!initialised) 
	{
		std::cout << "initialising physfs to " << TEST_EXECUTION_PATH << "\n";
		static FileSystem fs( TEST_EXECUTION_PATH );
		fs.setWriteDir(".");
		fs.addToSearchPath(".", true);
		initialised = true;
	}
}

// Tests of common FileSystem functions
// test init

BOOST_AUTO_TEST_SUITE( FileSystemTest )

BOOST_AUTO_TEST_CASE( default_constructor )
{
	/// \todo spec what happens here; currently asserts
	// FileSystem::getSingleton();
	
	/// \todo how to make this a sensible path on all platforms?
	{
		FileSystem fs( TEST_EXECUTION_PATH );
	
		BOOST_CHECK_EQUAL( &fs, &FileSystem::getSingleton());
	
		/// \todo currently, an assertion fails here!
		// try to init again
		//FileSystem fs2("C:\\Dokumente und Einstellungen\\Erik\\Eigene Dateien\\Blobby Volley 2\\test\\bin\\debug\\");
	}
	// here, fs is deleted so we can create a new file system
	// try to create it with spam path
	/// \todo spec, what error happens here
	FileSystem fs3("__SPAM__");
}

// the functions deleteFile, exists, isDirectory, addToSearchPath, removeFromSearchPath, setWriteDir and getUserDir
// currently just wrap PHYSFS functions, so they actually don't do anything. Thus, these functions are not
// tested here. Once we have a defined error reporting policy etc, tests will be added.

/// \todo test EnumerateFiles
BOOST_AUTO_TEST_CASE( enumerate_files )
{
	/// \todo spec what happens here; currently asserts
	// FileSystem::getSingleton();
	
	/// \todo how to make this a sensible path on all platforms?
	{
		FileSystem fs( TEST_EXECUTION_PATH );
	
		BOOST_CHECK_EQUAL( &fs, &FileSystem::getSingleton());
	
		/// \todo currently, an assertion fails here!
		// try to init again
		//FileSystem fs2("C:\\Dokumente und Einstellungen\\Erik\\Eigene Dateien\\Blobby Volley 2\\test\\bin\\debug\\");
	}
	// here, fs is deleted so we can create a new file system
	// try to create it with spam path
	/// \todo spec, what error happens here
	FileSystem fs3("__SPAM__");
}
/// \todo test probeDir

BOOST_AUTO_TEST_SUITE_END()

// most of the following functions just wrap to PHYSFS calls. 
/// \todo how to test these? 
// test enumerate files
// test deleteFile
// test exists
// test isDirectory
// test mkdir
// ...

/// \todo check all FileSystem methods are covered in tests
/// \todo check all FileRead/FileWrite methods are covered in tests


BOOST_AUTO_TEST_SUITE( ReadFileTest )

BOOST_AUTO_TEST_CASE( default_constructor )
{
	FileRead default_constructed;
	
	// no file is opened after default construction
	BOOST_CHECK( default_constructed.is_open() == false );
	BOOST_CHECK( default_constructed.getFileName() == "" );
	
	// all other operations should raise an assertion!
	BOOST_CHECK_THROW(default_constructed.length(), NoFileOpenedException);
	BOOST_CHECK_THROW(default_constructed.tell(), NoFileOpenedException);
	
	char target;
	BOOST_CHECK_THROW(default_constructed.readRawBytes(&target, 1), NoFileOpenedException);
	BOOST_CHECK_THROW(default_constructed.readRawBytes(1), NoFileOpenedException);
	BOOST_CHECK_THROW(default_constructed.readUInt32(), NoFileOpenedException);
	BOOST_CHECK_THROW(default_constructed.readString(), NoFileOpenedException);
}

BOOST_AUTO_TEST_CASE( open_read_constructor )
{
	init_Physfs();
	
	// create a temp file for the next check
	try 
	{
		
		FileWrite write_file("test_open_read_constructor.tmp");
		write_file.write("test");
		write_file.close();
	} catch (std::exception& s) {
		std::cout << "Error: " << s.what() << "\n";
		BOOST_ERROR("this should never happen!");
	}
	
	// now this file exists
	try{
		FileRead read_file("test_open_read_constructor.tmp");
	
		// now, a file is opened!
		BOOST_REQUIRE( read_file.is_open() == true );
		BOOST_CHECK( read_file.getFileName() == "test_open_read_constructor.tmp" );
		BOOST_CHECK( read_file.length() == 4);
	
		read_file.close();
		BOOST_CHECK( read_file.is_open() == false );
		BOOST_CHECK( read_file.getFileName() == "" );
		
	} catch (std::exception& e)
	{
		BOOST_ERROR(e.what());
	}
	
	BOOST_CHECK_THROW(FileRead read_file("this_file_surely_does_not_exists?!@<|.tmp"), FileLoadException);
	
	PHYSFS_delete("test_open_read_constructor.tmp");
}

// !!!!!!!!!!!
// we don't have any tests for the functions close, getPHYSFS_file and is_open. 
// These are already tested together with the constructors (and other functions) 
//


// wrongly closed file

BOOST_AUTO_TEST_CASE( wrongly_closed_file_test )
{	
	init_Physfs() ;

	FileWrite create_test_file("test_open_close.tmp");
	
	create_test_file.writeByte(1);
	// close the file, loughing wickedly ;)
	// don't ever do that in non-test code!
	create_test_file.close();
	
	FileRead test_file ("test_open_close.tmp");
	test_file.close();
	
	/// \todo this test can't work as we do it now because physfs just crashes when we 
	///			do this.
	//PHYSFS_close( (PHYSFS_file*)test_file.getPHYSFS_file() );
	// now, every action we try to perform on that file should yield an excpetion
	
	/// For now, these are all functions we have to test
	/// make sure to add new ones!
	
	BOOST_CHECK_THROW(test_file.tell(), NoFileOpenedException);
	BOOST_CHECK_THROW(test_file.seek(2), NoFileOpenedException);
	BOOST_CHECK_THROW(test_file.length(), NoFileOpenedException);
	
	char buffer[3];
	BOOST_CHECK_THROW(test_file.readRawBytes(buffer, 3), NoFileOpenedException);
	BOOST_CHECK_THROW(test_file.readRawBytes(1), NoFileOpenedException);
	BOOST_CHECK_THROW(test_file.readUInt32(), NoFileOpenedException);
	BOOST_CHECK_THROW(test_file.readString(), NoFileOpenedException);	
	
	BOOST_CHECK_THROW(create_test_file.writeByte(5), NoFileOpenedException);
	BOOST_CHECK_THROW(create_test_file.writeUInt32(5), NoFileOpenedException);
	BOOST_CHECK_THROW(create_test_file.write( std::string("bye bye world;)") ), NoFileOpenedException);
	BOOST_CHECK_THROW(create_test_file.writeNullTerminated( std::string("bye bye world;)") ), NoFileOpenedException);
	BOOST_CHECK_THROW(create_test_file.write( "bye bye world;)", 8 ), NoFileOpenedException);
	
}

BOOST_AUTO_TEST_CASE( exception_test )
{	
	init_Physfs() ;

	// create a temp helper file
	{
		FileWrite helper("read.tmp");
		helper.writeByte(5);
	}

	FileRead test_file("read.tmp");
	
	BOOST_REQUIRE( test_file.is_open() == true );

	// TODO at least on Linux, physfs happily seeks past the length of a file without complaint
	// contrary to what the documentation promises. For now, I've disabled these tests
	// move reader in front of file beginning
	// BOOST_CHECK_THROW(test_file.seek(-1), PhysfsException);
	// move reader after file ending
	// BOOST_CHECK_THROW(test_file.seek(100), PhysfsException);
	
	char buffer[3];
	// read negative amounts of bytes
	BOOST_CHECK_THROW(test_file.readRawBytes(buffer, -5), PhysfsException);
	// depending on the system, this will either throw a std::length_error or std::bad_alloc.
	BOOST_CHECK_THROW(test_file.readRawBytes(-5), std::exception);
	
	test_file.seek(0);
	
	// read more than there is
	BOOST_CHECK_THROW(test_file.readRawBytes(buffer, 3), EOFException);
	BOOST_CHECK_THROW(test_file.readUInt32(), EOFException);
	BOOST_CHECK_THROW(test_file.readRawBytes(5), EOFException);
	BOOST_CHECK_THROW(test_file.readString(), EOFException);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( WriteFileTest )

BOOST_AUTO_TEST_CASE( default_constructor )
{
	FileWrite default_constructed;
	
	// no file is opened after default construction
	BOOST_CHECK( default_constructed.is_open() == false );
	BOOST_CHECK( default_constructed.getFileName() == "" );
	
	// all other operations should raise an assertion!
	BOOST_CHECK_THROW(default_constructed.length(), NoFileOpenedException);
	BOOST_CHECK_THROW(default_constructed.tell(), NoFileOpenedException);
	
	char target;
	BOOST_CHECK_THROW(default_constructed.writeByte('c'), NoFileOpenedException);
	BOOST_CHECK_THROW(default_constructed.write(std::string("c")), NoFileOpenedException);
	BOOST_CHECK_THROW(default_constructed.writeUInt32(5), NoFileOpenedException);
	BOOST_CHECK_THROW(default_constructed.writeNullTerminated(std::string("c")), NoFileOpenedException);
	BOOST_CHECK_THROW(default_constructed.write(&target, 1), NoFileOpenedException);
}


BOOST_AUTO_TEST_CASE( open_write_constructor )
{
	init_Physfs() ;
	
	FileWrite write_file("test_open_write_constructor.tmp");
	
	// now, a file is opened!
	BOOST_REQUIRE( write_file.is_open() == true );
	BOOST_CHECK( write_file.getFileName() == "test_open_write_constructor.tmp" );
	
	// this file is new, so length should be 0
	BOOST_CHECK( write_file.length() == 0 );
	
	write_file.close();
	BOOST_CHECK( write_file.is_open() == false );
	
	// make sure we delete this file after the test, so we can run the test a second time
	// under same circumstances
	PHYSFS_delete("test_open_write_constructor.tmp");
	
	try
	{
		FileWrite write_file2("this_file_surely_/cannot_exists?!@<|.tmp");
		BOOST_ERROR("opening files with invalid names should lead to an exception");
	} catch (std::exception& s) {
		// fine
	} 	
}


BOOST_AUTO_TEST_CASE( open_close_test )
{
	init_Physfs();
			
	FileWrite test_file("test_open_close.tmp");
	BOOST_REQUIRE( test_file.is_open() == true );
	test_file.close();
	BOOST_REQUIRE( test_file.is_open() == false );
	
	// now open another file
	test_file.open("test_open_close2.tmp");
	BOOST_REQUIRE( test_file.is_open() == true );
	test_file.close();
	BOOST_REQUIRE( test_file.is_open() == false );
	
	// and again the first file
	test_file.open("test_open_close.tmp");
	BOOST_REQUIRE( test_file.is_open() == true );
	test_file.close();
	BOOST_REQUIRE( test_file.is_open() == false );
	
	// cleanup
	PHYSFS_delete("test_open_close.tmp");
	PHYSFS_delete("test_open_close2.tmp");
}
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( FileWriteReadCycle )

BOOST_AUTO_TEST_CASE( raw_data_test )
{
	init_Physfs();
	
	char data[] = { 's', 'p', 'a', 'm', ' ', 't', 'e', 's', 't' };
			
	FileWrite writer("cycle.tmp");
	BOOST_REQUIRE( writer.is_open() == true );
	//FileRead reader_e("cycle.tmp");
	/// \todo we need to define what happens when we open a file for reading and writing simultaniuosly
	writer.write( data, sizeof(data) );
	writer.close();
	
	FileRead reader("cycle.tmp");
	char data2[sizeof(data)];
	reader.readRawBytes(data2, sizeof(data));
	
	BOOST_CHECK( std::memcmp(data, data2, sizeof(data)) == 0 );
	reader.seek(0);
	auto data3 = reader.readRawBytes(sizeof(data));
	BOOST_CHECK( std::memcmp(data, data3.data(), sizeof(data)) == 0 );
	
	PHYSFS_delete("cycle.tmp");
}

BOOST_AUTO_TEST_CASE( string_test )
{
	init_Physfs();
	
	std::string teststr = "hello world!";

	BOOST_TEST_CHECKPOINT( "string_test: writing test file" );
	FileWrite writer("cycle.tmp");
	BOOST_REQUIRE( writer.is_open() == true );

	writer.write( teststr );
	writer.writeNullTerminated( teststr );
	writer.write( teststr );
	writer.close();

	BOOST_TEST_CHECKPOINT( "string_test: reading test file" );
	FileRead reader("cycle.tmp");
	
	auto data = reader.readRawBytes(teststr.size());
	BOOST_CHECK_EQUAL (reader.tell(), teststr.size() );
	std::string str2 = reader.readString();
	BOOST_CHECK_EQUAL (reader.tell(), 2 * teststr.size() + 1 );
	
	BOOST_CHECK( std::memcmp(data.data(), teststr.data(), teststr.length()) == 0 );
	BOOST_CHECK_EQUAL( teststr, str2 );
	
	// now, try to read as null terminated when it isn't
	BOOST_CHECK_THROW( reader.readString(), EOFException);
	
	PHYSFS_delete("cycle.tmp");
}


BOOST_AUTO_TEST_CASE( int_test )
{
	init_Physfs();


	BOOST_TEST_CHECKPOINT( "int_test: writing test file" );
	FileWrite writer("cycle.tmp");
	BOOST_REQUIRE( writer.is_open() == true );

	const int TEST_INT_1 = 12;
	const int TEST_INT_2 = -8;
	const int TEST_INT_3 = 1275343;

	writer.writeUInt32( TEST_INT_1 );
	writer.writeUInt32( TEST_INT_2 );
	writer.writeUInt32( TEST_INT_3 );
	writer.writeByte( 5 );
	writer.close();

	BOOST_TEST_CHECKPOINT( "int_test: reading test file" );
	FileRead reader("cycle.tmp");
	
	int res = reader.readUInt32( );
	BOOST_CHECK_EQUAL (res, TEST_INT_1 );
	res = reader.readUInt32( );
	BOOST_CHECK_EQUAL (res, TEST_INT_2 );
	res = reader.readUInt32( );
	BOOST_CHECK_EQUAL (res, TEST_INT_3 );
	
	// try to read more
	BOOST_CHECK_THROW( reader.readUInt32(), EOFException);
	
	PHYSFS_delete("cycle.tmp");
}

BOOST_AUTO_TEST_SUITE_END()
