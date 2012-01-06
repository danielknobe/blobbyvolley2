#define BOOST_TEST_MODULE FileAbstraction
#include <boost/test/unit_test.hpp>

#include "File.h"
#include <iostream>
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


BOOST_AUTO_TEST_SUITE( FileAbstraction )

BOOST_AUTO_TEST_CASE( default_constructor )
{
	File default_constructed;
	// no file is opened after default construction
	BOOST_REQUIRE( default_constructed.is_open() == false );
	BOOST_REQUIRE( default_constructed.getPHYSFS_file() == 0 );
	
	// all other operations should raise an assertion!
}

BOOST_AUTO_TEST_CASE( open_write_constructor )
{
	init_Physfs() ;
	
	File write_file("test_open_write_constructor.tmp", File::OPEN_WRITE);
	
	// now, a file is opened!
	BOOST_REQUIRE( write_file.is_open() == true );
	BOOST_REQUIRE( write_file.getPHYSFS_file() != 0 );
	
	// this file is new, so length should be 0
	BOOST_REQUIRE( write_file.length() == 0 );
	
	write_file.close();
	BOOST_REQUIRE( write_file.is_open() == false );
	
	// make sure we delete this file after the test, so we can run the test a second time
	// under same circumstances
	PHYSFS_delete("test_open_write_constructor.tmp");
	
	try
	{
		File write_file2("this_file_surely_does_not_exists?!@<|.tmp", File::OPEN_WRITE);
		BOOST_ERROR("opening fiels with invalid names should lead to an exception");
	} catch (std::exception& s) {
		// fine
	} 	
}


BOOST_AUTO_TEST_CASE( open_read_constructor )
{
	init_Physfs();
	
	// create a temp file for the next check
	try {
		File write_file("test_open_read_constructor.tmp", File::OPEN_WRITE);
		write_file.write("test");
		write_file.close();
	} catch (std::exception& s) {
		BOOST_ERROR("this should never happen as we tested this behaviour in the test before!");
	}
	
	// now this file exists
	try{
		File read_file("test_open_read_constructor.tmp", File::OPEN_READ);
	
		// now, a file is opened!
		BOOST_REQUIRE( read_file.is_open() == true );
		BOOST_REQUIRE( read_file.getPHYSFS_file() != 0 );
	
		read_file.close();
		BOOST_REQUIRE( read_file.is_open() == false );
	} catch (std::exception& e)
	{
		BOOST_ERROR(e.what());
	}
	
	CHECK_EXCEPTION_SAFETY(File read_file("this_file_surely_does_not_exists?!@<|.tmp", File::OPEN_READ), 
							std::exception);
	
	PHYSFS_delete("test_open_read_constructor.tmp");
}

// !!!!!!!!!!!
// we don't have any tests for the functions close, getPHYSFS_file and is_open. 
// These are already tested together with the constructors (and other functions) 
//

BOOST_AUTO_TEST_CASE( open_close_test )
{
	init_Physfs();
			
	File test_file("test_open_close.tmp", File::OPEN_WRITE);
	BOOST_REQUIRE( test_file.is_open() == true );
	test_file.close();
	BOOST_REQUIRE( test_file.is_open() == false );
	
	// now open another file
	test_file.open("test_open_close2.tmp", File::OPEN_WRITE);
	BOOST_REQUIRE( test_file.is_open() == true );
	test_file.close();
	BOOST_REQUIRE( test_file.is_open() == false );
	
	// and again the first file
	test_file.open("test_open_close.tmp", File::OPEN_WRITE);
	BOOST_REQUIRE( test_file.is_open() == true );
	test_file.close();
	BOOST_REQUIRE( test_file.is_open() == false );
	
	// cleanup
	PHYSFS_delete("test_open_close.tmp");
	PHYSFS_delete("test_open_close2.tmp");
}

// wrongly closed file

BOOST_AUTO_TEST_CASE( wrongly_closed_file_test )
{	
	init_Physfs() ;

	File test_file("test_open_close.tmp", File::OPEN_WRITE);
	
	test_file.writeByte(1);
	// close the file, loughing wickedly ;)
	// don't ever do that in non-test code!
	PHYSFS_close( (PHYSFS_file*)test_file.getPHYSFS_file() );
	
	/// \todo this test can't work as we do it now because physfs just crashes when we 
	///			do this.
	// now, every action we try to perform on that file should yield an excpetion
	/*
	CHECK_EXCEPTION_SAFETY(test_file.tell(), 
							std::exception);
	
	//CHECK_EXCEPTION_SAFETY(test_file.length(), 	// FAIL, this crashes!
	//						std::exception);
	
	CHECK_EXCEPTION_SAFETY(test_file.writeByte(5), std::exception);
	*/
}

BOOST_AUTO_TEST_CASE( write_to_readonly_test )
{	
	init_Physfs() ;

	// create a temp helper file
	{
		File helper("readonly.tmp", File::OPEN_WRITE);
		helper.writeByte(5);
	}

	File test_file("readonly.tmp", File::OPEN_READ);
	
	BOOST_REQUIRE( test_file.is_open() == true );
	
	// now, every action we try to perform on that file should yield an excpetion
	CHECK_EXCEPTION_SAFETY(test_file.writeByte(5), PhysfsException);
	CHECK_EXCEPTION_SAFETY(test_file.write("abc", 3), PhysfsException);
	CHECK_EXCEPTION_SAFETY(test_file.writeUInt32(12), PhysfsException);
	CHECK_EXCEPTION_SAFETY(test_file.write(std::string("hello world")), PhysfsException);
	CHECK_EXCEPTION_SAFETY(test_file.writeNullTerminated(std::string("hello world")), PhysfsException);
}


BOOST_AUTO_TEST_CASE( exception_test )
{	
	init_Physfs() ;

	// create a temp helper file
	{
		File helper("read.tmp", File::OPEN_WRITE);
		helper.writeByte(5);
	}

	File test_file("read.tmp", File::OPEN_READ);
	
	BOOST_REQUIRE( test_file.is_open() == true );
	
	// move reader in front of file beginning
	CHECK_EXCEPTION_SAFETY(test_file.seek(-1), PhysfsException);
	// move reader after file ending
	CHECK_EXCEPTION_SAFETY(test_file.seek(100), PhysfsException);
	
	char buffer[3];
	CHECK_EXCEPTION_SAFETY(test_file.readRawBytes(buffer, 3), PhysfsException);		// FAIL
	
	// read negative amounts of bytes
	CHECK_EXCEPTION_SAFETY(test_file.readRawBytes(buffer, -5), PhysfsException);	// FAIL
	CHECK_EXCEPTION_SAFETY(test_file.readRawBytes(-5), PhysfsException);			// FAIL
	
	test_file.seek(0);
	
	// read more than there is
	CHECK_EXCEPTION_SAFETY(test_file.readRawBytes(buffer, 3), PhysfsException);
	CHECK_EXCEPTION_SAFETY(test_file.readUInt32(), PhysfsException);		// FAIL
}

BOOST_AUTO_TEST_SUITE_END()
