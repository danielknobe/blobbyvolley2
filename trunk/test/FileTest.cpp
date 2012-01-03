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
	
	try
	{
		File read_file("this_file_surely_does_not_exists?!@<|.tmp", File::OPEN_READ);
		BOOST_ERROR("openeing unexisting file should lead to an exception!");
	} catch (std::exception& s) {
		// fine
	} 
	
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
	PHYSFS_deinit();
}

BOOST_AUTO_TEST_SUITE_END()
