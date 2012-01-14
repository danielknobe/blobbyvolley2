#define BOOST_TEST_MODULE FileAbstraction
#include <boost/test/unit_test.hpp>

#include "FileRead.h"
#include "FileWrite.h"
#include <iostream>
#include <cstring>
#include <physfs.h>

// helper
void init_Physfs() 
{
	static bool initialised = false;
	if(!initialised) 
	{
		BOOST_REQUIRE( PHYSFS_init("C:\\Dokumente und Einstellungen\\Erik\\Eigene Dateien\\Blobby Volley 2\\test\\bin\\debug\\") );
		PHYSFS_setWriteDir(".");
		PHYSFS_addToSearchPath(".", 1);
		initialised = true;
	}
}

#define CHECK_EXCEPTION_SAFETY(expr, excp) 	try { 	\
											BOOST_TEST_CHECKPOINT("trying " #expr);		\
											expr; \
											BOOST_ERROR(#expr " does not cause " #excp " to be thrown");\
											} \
											catch(excp& e) {\
												std::cout << "what: " << e.what() << std::endl; \
											};


BOOST_AUTO_TEST_SUITE( ReadFileTest )

BOOST_AUTO_TEST_CASE( default_constructor )
{
	FileRead default_constructed;
	
	// no file is opened after default construction
	BOOST_CHECK( default_constructed.is_open() == false );
	BOOST_CHECK( default_constructed.getPHYSFS_file() == 0 );
	BOOST_CHECK( default_constructed.getFileName() == "" );
	
	// all other operations should raise an assertion!
	CHECK_EXCEPTION_SAFETY (default_constructed.length(), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY (default_constructed.tell(), NoFileOpenedException);
	
	char target;
	CHECK_EXCEPTION_SAFETY (default_constructed.readRawBytes(&target, 1), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY (default_constructed.readRawBytes(1), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY (default_constructed.readUInt32(), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY (default_constructed.readString(), NoFileOpenedException);
}

BOOST_AUTO_TEST_CASE( open_read_constructor )
{
	init_Physfs();
	
	// create a temp file for the next check
	try {
		FileWrite write_file("test_open_read_constructor.tmp");
		write_file.write("test");
		write_file.close();
	} catch (std::exception& s) {
		BOOST_ERROR("this should never happen!");
	}
	
	// now this file exists
	try{
		FileRead read_file("test_open_read_constructor.tmp");
	
		// now, a file is opened!
		BOOST_REQUIRE( read_file.is_open() == true );
		BOOST_CHECK( read_file.getPHYSFS_file() != 0 );
		BOOST_CHECK( read_file.getFileName() == "test_open_read_constructor.tmp" );
		BOOST_CHECK( read_file.length() == 4);
	
		read_file.close();
		BOOST_CHECK( read_file.is_open() == false );
		BOOST_CHECK( read_file.getPHYSFS_file() == 0 );
		BOOST_CHECK( read_file.getFileName() == "" );
		
	} catch (std::exception& e)
	{
		BOOST_ERROR(e.what());
	}
	
	CHECK_EXCEPTION_SAFETY(FileRead read_file("this_file_surely_does_not_exists?!@<|.tmp"), FileLoadException);
	
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
	
	CHECK_EXCEPTION_SAFETY(test_file.tell(), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY(test_file.seek(2), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY(test_file.length(), NoFileOpenedException);
	
	char buffer[3];
	CHECK_EXCEPTION_SAFETY(test_file.readRawBytes(buffer, 3), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY(test_file.readRawBytes(1), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY(test_file.readUInt32(), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY(test_file.readString(), NoFileOpenedException);	
	
	CHECK_EXCEPTION_SAFETY(create_test_file.writeByte(5), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY(create_test_file.writeUInt32(5), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY(create_test_file.write( std::string("bye bye world;)") ), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY(create_test_file.writeNullTerminated( std::string("bye bye world;)") ), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY(create_test_file.write( "bye bye world;)", 8 ), NoFileOpenedException);
	
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
	
	// move reader in front of file beginning
	CHECK_EXCEPTION_SAFETY(test_file.seek(-1), PhysfsException);
	// move reader after file ending
	CHECK_EXCEPTION_SAFETY(test_file.seek(100), PhysfsException);
	
	char buffer[3];
	// read negative amounts of bytes
	CHECK_EXCEPTION_SAFETY(test_file.readRawBytes(buffer, -5), PhysfsException);
	CHECK_EXCEPTION_SAFETY(test_file.readRawBytes(-5), std::bad_alloc);
	
	test_file.seek(0);
	
	// read more than there is
	CHECK_EXCEPTION_SAFETY(test_file.readRawBytes(buffer, 3), EOFException);
	CHECK_EXCEPTION_SAFETY(test_file.readUInt32(), PhysfsException);		// FAIL
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( WriteFileTest )

BOOST_AUTO_TEST_CASE( default_constructor )
{
	FileWrite default_constructed;
	
	// no file is opened after default construction
	BOOST_CHECK( default_constructed.is_open() == false );
	BOOST_CHECK( default_constructed.getPHYSFS_file() == 0 );
	BOOST_CHECK( default_constructed.getFileName() == "" );
	
	// all other operations should raise an assertion!
	CHECK_EXCEPTION_SAFETY (default_constructed.length(), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY (default_constructed.tell(), NoFileOpenedException);
	
	char target;
	CHECK_EXCEPTION_SAFETY (default_constructed.writeByte('c'), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY (default_constructed.write(std::string("c")), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY (default_constructed.writeUInt32(5), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY (default_constructed.writeNullTerminated(std::string("c")), NoFileOpenedException);
	CHECK_EXCEPTION_SAFETY (default_constructed.write(&target, 1), NoFileOpenedException);
}


BOOST_AUTO_TEST_CASE( open_write_constructor )
{
	init_Physfs() ;
	
	FileWrite write_file("test_open_write_constructor.tmp");
	
	// now, a file is opened!
	BOOST_REQUIRE( write_file.is_open() == true );
	BOOST_CHECK( write_file.getPHYSFS_file() != 0 );
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
		FileWrite write_file2("this_file_surely_cannot_exists?!@<|.tmp");
		BOOST_ERROR("opening fiels with invalid names should lead to an exception");
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
	boost::shared_array<char> data3 = reader.readRawBytes(sizeof(data));
	BOOST_CHECK( std::memcmp(data, data3.get(), sizeof(data)) == 0 );
	
	PHYSFS_delete("cycle.tmp");
}

BOOST_AUTO_TEST_CASE( string_test )
{
	init_Physfs();
	
	std::string teststr = "hello world!";
			
	FileWrite writer("cycle.tmp");
	BOOST_REQUIRE( writer.is_open() == true );

	writer.write( teststr );
	writer.writeNullTerminated( teststr );
	writer.write( teststr );
	writer.close();
	
	FileRead reader("cycle.tmp");
	/// \todo convenience function for to reading null terminated strings
	boost::shared_array<char> data = reader.readRawBytes(teststr.size());
	BOOST_CHECK (reader.tell() == teststr.size() );
	std::string str2 = reader.readString();
	BOOST_CHECK (reader.tell() == 2 * teststr.size() + 1 );
	std::cout << "read: " << str2 << "\n";
	std::cout << "read: " << data.get() << "\n";
	
	BOOST_CHECK( std::memcmp(data.get(), teststr.data(), teststr.length()) == 0 );
	BOOST_CHECK( teststr == str2 );
	
	// now, try to read as null terminated when it isn't
	/// \todo we need a sensible check here
	std::string str3 = reader.readString();
	
	PHYSFS_delete("cycle.tmp");
}

BOOST_AUTO_TEST_SUITE_END()
