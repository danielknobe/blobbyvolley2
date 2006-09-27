/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @ingroup RAKNET_AUTOPATCHER
 * @brief Autopatcher Implementation. 
 *  
 * This file is part of RakNet Copyright 2003 Rakkarsoft LLC and Kevin Jenkins.
 *
 * Usage of Raknet is subject to the appropriate licence agreement.
 * "Shareware" Licensees with Rakkarsoft LLC are subject to the
 * shareware license found at
 * http://www.rakkarsoft.com/shareWareLicense.html which you agreed to
 * upon purchase of a "Shareware license" "Commercial" Licensees with
 * Rakkarsoft LLC are subject to the commercial license found at
 * http://www.rakkarsoft.com/sourceCodeLicense.html which you agreed
 * to upon purchase of a "Commercial license"
 * Custom license users are subject to the terms therein.
 * All other users are
 * subject to the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Refer to the appropriate license agreement for distribution,
 * modification, and warranty rights.
 */
#include <assert.h>
#include <stdio.h>
#include "Autopatcher.h"
#include "DownloadableFileDescriptor.h"
#include "RakPeerInterface.h"
#include "RakServerInterface.h"
#include "RakClientInterface.h"
#include "PacketEnumerations.h"
#include "BitStream.h"
#include "zlib.h"
#ifdef _WIN32 
// For mkdir
#include <direct.h>
#else
#include <sys/stat.h>
#endif

using namespace RakNet;
AutoPatcher::AutoPatcher()
{
	downloadPrefix = 0;
	compressionBoundary = 1024;
	downloadableFilesRequested = false;
	orderingStream = 0;
}

AutoPatcher::~AutoPatcher()
{
	Clear();
}

void AutoPatcher::SetOrderingStream( int streamIndex )
{
	assert( streamIndex >= 0 && streamIndex < 32 );
	orderingStream = streamIndex;
}

void AutoPatcher::Clear( void )
{
	unsigned index;
	
	if ( downloadPrefix )
		delete [] downloadPrefix;
		
	downloadPrefix = 0;
	
	for ( index = 0; index < downloadableFiles.size(); index++ )
		delete downloadableFiles[ index ];
		
	downloadableFiles.clear();
	
	for ( index = 0; index < downloadingFiles.size(); index++ )
		delete downloadingFiles[ index ];
		
	downloadingFiles.clear();
}

void AutoPatcher::SetNetworkingSystem( RakPeerInterface *localSystem )
{
	rakPeerInterface = localSystem;
	rakClientInterface = 0;
	rakServerInterface = 0;
}

void AutoPatcher::SetNetworkingSystem( RakClientInterface *localSystem )
{
	rakPeerInterface = 0;
	rakClientInterface = localSystem;
	rakServerInterface = 0;
}

void AutoPatcher::SetNetworkingSystem( RakServerInterface *localSystem )
{
	rakPeerInterface = 0;
	rakClientInterface = 0;
	rakServerInterface = localSystem;
}

void AutoPatcher::SetCompressionBoundary( unsigned boundary )
{
	compressionBoundary = boundary;
}

bool AutoPatcher::UnsetFileDownloadable( char *filename )
{
	unsigned index;
	DownloadableFileDescriptor *dfd;
	
	if ( filename == 0 || filename[ 0 ] == 0 )
	{
		assert( 0 );
		return false;
	}
	
	for ( index = 0; index < downloadableFiles.size(); index++ )
	{
		if ( strcmp( filename, downloadableFiles[ index ] ->filename ) == 0 )
		{
			dfd = downloadableFiles[ index ];
			downloadableFiles.del( index );
			delete dfd;
			return true;
		}
	}
	
	// Can't find that file
	return false;
}

// Creates a .sha file signature for a particular file.
// This is used by SetFileDownloadable with checkFileSignature as true
bool AutoPatcher::CreateFileSignature( char *filename )
{
	FILE * fp;
	CSHA1 sha1;
	char SHA1Code[ SHA1_LENGTH ];
	char *sha1Path;
	char *fileData;
	unsigned fileLength;
	
	if ( filename == 0 || filename[ 0 ] == 0 )
	{
		assert( 0 );
		return false;
	}
	
	fileLength = AutoPatcher::GetFileLength( filename );
	
	if ( fileLength == 0 )
		return false;
		
	fileData = new char [ fileLength ];
	
	fp = fopen( filename, "rb" );
	
	fread( fileData, 1, fileLength, fp );
	
	fclose( fp );
	
	sha1.Reset();
	
	sha1.Update( ( unsigned char * ) fileData, fileLength );
	
	sha1.Final();
	
	memcpy( SHA1Code, sha1.GetHash(), SHA1_LENGTH );
	
	delete [] fileData;
	
	sha1Path = new char[ strlen( filename ) + 1 + 4 ];
	
	strcpy( sha1Path, filename );
	
	strcat( sha1Path, ".sha" );
	
	fp = fopen( sha1Path, "wb" );
	
	if ( fp == 0 )
	{
		assert( 0 ); // File system error
		delete [] sha1Path;
		return false;
	}
	
	delete [] sha1Path;
	fwrite( SHA1Code, 1, SHA1_LENGTH * sizeof( char ), fp );
	fclose( fp );
	return true;
}

SetFileDownloadableResult AutoPatcher::SetFileDownloadable( char *filename, bool checkFileSignature )
{
	unsigned index;
	DownloadableFileDescriptor *dfd;
	FILE *fp;
	CSHA1 sha1;
	char SHA1Code[ SHA1_LENGTH ];
	char *sha1Path;
	char *fileSource;
	
	if ( filename == 0 || filename[ 0 ] == 0 )
	{
		assert( 0 );
		return SET_FILE_DOWNLOADABLE_FAILED;
	}
	
	for ( index = 0; index < downloadableFiles.size(); index++ )
	{
		if ( strcmp( filename, downloadableFiles[ index ] ->filename ) == 0 )
		{
			// Already exists
			return SET_FILE_DOWNLOADABLE_SUCCESS;
		}
	}
	
	dfd = new DownloadableFileDescriptor;
	dfd->filename = new char [ strlen( filename ) + 1 ];
	strcpy( dfd->filename, filename );
	dfd->fileLength = AutoPatcher::GetFileLength( filename );
	
	if ( dfd->fileLength == 0 )
	{
		// Not a filename, or a bad filename
		delete dfd;
		return SET_FILE_DOWNLOADABLE_FAILED;
	}
	
	if ( dfd->fileLength >= compressionBoundary )
	{
		dfd->fileDataIsCompressed = true;
		// Can I use the same source and destination buffers to zlib compress?
		// Not sure - to be safe generate another array to hold the file source
		fileSource = new char [ dfd->fileLength ];
		fp = fopen( filename, "rb" );
		fread( fileSource, 1, dfd->fileLength, fp );
		fclose( fp );
		// Generate the SHA1 for the file
		sha1.Reset();
		sha1.Update( ( unsigned char * ) fileSource, dfd->fileLength );
		sha1.Final();
		memcpy( dfd->SHA1Code, sha1.GetHash(), SHA1_LENGTH );
		// According to zlib docs, 1.01% + 12 is the minimum output buffer size
		// We'll do 13 to account for rounding
		dfd->compressedFileLength = ( unsigned ) ( ( float ) dfd->fileLength * 1.01f + 13 );
		dfd->fileData = new char [ dfd->compressedFileLength ];
		// Compress should change dfd->compressedFileLength
		
		if ( compress( ( Bytef* ) dfd->fileData, ( uLongf* ) & ( dfd->compressedFileLength ), ( const Bytef * ) fileSource, ( uLong ) dfd->fileLength ) != Z_OK )
		{
#ifdef _DEBUG
			assert( 0 );
#endif
			
			delete [] fileSource;
			delete dfd;
			return SET_FILE_DOWNLOADABLE_COMPRESSION_FAILED;
		}
		
		delete [] fileSource;
	}
	
	else
	{
		dfd->fileDataIsCompressed = false;
		dfd->fileData = new char [ dfd->fileLength ];
		fp = fopen( filename, "rb" );
		fread( dfd->fileData, 1, dfd->fileLength, fp );
		fclose( fp );
		// Generate the SHA1 for the file
		sha1.Reset();
		sha1.Update( ( unsigned char * ) dfd->fileData, dfd->fileLength );
		sha1.Final();
		memcpy( dfd->SHA1Code, sha1.GetHash(), SHA1_LENGTH );
	}
	
	// Check the file signature
	if ( checkFileSignature )
	{
		// Try to open .sha
		sha1Path = new char[ strlen( filename ) + 1 + 4 ];
		strcpy( sha1Path, filename );
		strcat( sha1Path, ".sha" );
		fp = fopen( sha1Path, "rb" );
		
		if ( fp == 0 )
		{
			delete dfd;
			delete [] sha1Path;
			return SET_FILE_DOWNLOADABLE_FILE_NO_SIGNATURE_FILE;
		}
		
		if ( fread( SHA1Code, 1, SHA1_LENGTH * sizeof( char ), fp ) != SHA1_LENGTH )
		{
			delete dfd;
			delete [] sha1Path;
			return SET_FILE_DOWNLOADABLE_FILE_SIGNATURE_CHECK_FAILED;
		}
		
		fclose( fp );
		delete [] sha1Path;
		
		if ( memcmp( dfd->SHA1Code, SHA1Code, sizeof( char ) * SHA1_LENGTH ) != 0 )
		{
			delete dfd;
			return SET_FILE_DOWNLOADABLE_FILE_SIGNATURE_CHECK_FAILED;
		}
	}
	
	downloadableFiles.insert( dfd );
	return SET_FILE_DOWNLOADABLE_SUCCESS;
}

void AutoPatcher::SetDownloadedFileDirectoryPrefix( char *prefix )
{
	int len;
	
	if ( downloadPrefix )
	{
		if ( prefix && strcmp( downloadPrefix, prefix ) == 0 )
			return ;
			
		delete [] downloadPrefix;
	}
	
	downloadPrefix = 0;
	
	if ( prefix && prefix[ 0 ] )
	{
		downloadPrefix = new char [ strlen( prefix ) + 2 ];
		strcpy( downloadPrefix, prefix );
		len = ( int ) strlen( downloadPrefix );
		// Append a / if it doesn't have one
		// so it's easier to strcat stuff to downloadPrefix in other places
		
		if ( downloadPrefix[ len - 1 ] != '/' && downloadPrefix[ len - 1 ] != '\\' )
		{
			downloadPrefix[ len ] = '/';
			downloadPrefix[ len + 1 ] = 0;
		}
	}
}

void AutoPatcher::RequestDownloadableFileList( PlayerID remoteSystem )
{
	assert( rakPeerInterface || rakClientInterface || rakServerInterface );
	downloadableFilesRequested = true;
	unsigned char packetID = ID_AUTOPATCHER_REQUEST_FILE_LIST;
	
	if ( rakServerInterface )
		rakServerInterface->Send( ( char* ) & packetID, sizeof( unsigned char ), HIGH_PRIORITY, RELIABLE, orderingStream, remoteSystem, false );
	else
		if ( rakPeerInterface )
			rakPeerInterface->Send( ( char* ) & packetID, sizeof( unsigned char ), HIGH_PRIORITY, RELIABLE, orderingStream, remoteSystem, false );
		else
			rakClientInterface->Send( ( char* ) & packetID, sizeof( unsigned char ), HIGH_PRIORITY, RELIABLE, orderingStream );
			
}

void AutoPatcher::SendDownloadableFileList( PlayerID remoteSystem )
{
	BitStream serializedFileDescriptor( 65536 );
	unsigned index;
	unsigned char packetID;
	assert( rakPeerInterface || rakClientInterface || rakServerInterface );
	packetID = ID_AUTOPATCHER_FILE_LIST;
	serializedFileDescriptor.Write( packetID );
	serializedFileDescriptor.WriteCompressed( downloadableFiles.size() );
	// Serialize the data for each of the downloadableFiles
	
	for ( index = 0; index < downloadableFiles.size(); index++ )
	{
		downloadableFiles[ index ] ->SerializeHeader( &serializedFileDescriptor );
		downloadableFiles[ index ] ->SerializeSHA1( &serializedFileDescriptor );
	}
	
	if ( rakServerInterface )
		rakServerInterface->Send( &serializedFileDescriptor, MEDIUM_PRIORITY, RELIABLE_ORDERED, orderingStream, remoteSystem, false );
	else
		if ( rakPeerInterface )
			rakPeerInterface->Send( &serializedFileDescriptor, MEDIUM_PRIORITY, RELIABLE_ORDERED, orderingStream, remoteSystem, false );
		else
			rakClientInterface->Send( &serializedFileDescriptor, MEDIUM_PRIORITY, RELIABLE_ORDERED, orderingStream );
}

void AutoPatcher::OnAutopatcherFileList( Packet *packet, bool onlyAcceptFilesIfRequested )
{
	BitStream serializedFileDescriptor( ( char* ) packet->data, packet->length, false );
	BitStream outputBitStream;
	DownloadableFileDescriptor *dfd;
	char *filePath;
	char SHA1Code[ SHA1_LENGTH ];
	bool allocatedFilePath;
	unsigned int numberOfDownloadableFiles;
	unsigned index;
	unsigned char packetID;
	assert( rakPeerInterface || rakClientInterface || rakServerInterface );
	assert( packet );
	
	if ( onlyAcceptFilesIfRequested && downloadableFilesRequested == false )
		return ;
		
	if ( packet == 0 )
		return ;
		
	downloadableFilesRequested = false;
	
	// Ignore ID_AUTOPATCHER_FILE_LIST
	serializedFileDescriptor.IgnoreBits( sizeof( unsigned char ) * 8 );
	
	if ( serializedFileDescriptor.ReadCompressed( numberOfDownloadableFiles ) == false )
	{
		// Invalid packet format. Should never get this unless it's a bug or someone is hacking
#ifdef _DEBUG
		assert( 0 );
#endif
		
		return ;
	}
	
	dfd = 0;
	
	for ( index = 0; index < numberOfDownloadableFiles; index++ )
	{
		if ( dfd == 0 )
			dfd = new DownloadableFileDescriptor;
		else
			dfd->Clear();
			
		if ( dfd->DeserializeHeader( &serializedFileDescriptor ) == false )
		{
			assert( 0 ); // Error in packet header.  Should only get this from hackers or bugs
			delete dfd;
			
			for ( index = 0; index < downloadingFiles.size(); index++ )
				delete downloadingFiles[ index ];
				
			downloadingFiles.clear();
			
			return ;
		}
		
		if ( dfd->DeserializeSHA1( &serializedFileDescriptor ) == false )
		{
			assert( 0 ); // Error in packet header.  Should only get this from hackers or bugs
			delete dfd;
			
			for ( index = 0; index < downloadingFiles.size(); index++ )
				delete downloadingFiles[ index ];
				
			downloadingFiles.clear();
			
			return ;
		}
		
		// Check to see if we have the file specified in the file descriptor.
		// If we don't have it, or the SHA1 doesn't match, then request to download it
		if ( downloadPrefix )
		{
			filePath = new char[ strlen( downloadPrefix ) + strlen( dfd->filename ) + 1 ];
			strcpy( filePath, downloadPrefix );
			strcat( filePath, dfd->filename );
			allocatedFilePath = true;
		}
		
		else
		{
			filePath = dfd->filename;
			allocatedFilePath = false;
		}
		
		// Just a guess - if the server uses a different compressionBoundary
		// then it will be a wrong guess
		dfd->fileDataIsCompressed = dfd->fileLength >= compressionBoundary ? true : false;
		
		if ( GenerateSHA1( filePath, SHA1Code ) == false ||
			memcmp( SHA1Code, dfd->SHA1Code, SHA1_LENGTH * sizeof( char ) ) != 0 )
		{
			// Don't have the file, or SHA1 doesn't match.
			// Add dfd to the list of files to download
			downloadingFiles.insert( dfd );
			dfd = 0;
		}
		
		if ( allocatedFilePath )
			delete [] filePath;
	}
	
	if ( dfd )
		delete dfd;
		
	// At this point downloadingFiles is probably what we will get back, in that order.
	// However, if the server rejects to send something then it will be changed by a later packet.
	if ( downloadingFiles.size() > 0 )
	{
		packetID = ID_AUTOPATCHER_REQUEST_FILES;
		outputBitStream.Write( packetID );
		outputBitStream.WriteCompressed( downloadingFiles.size() );
		
		for ( index = 0; index < downloadingFiles.size(); index++ )
			downloadingFiles[ index ] ->SerializeHeader( &outputBitStream );
			
		if ( rakServerInterface )
			rakServerInterface->Send( &outputBitStream, MEDIUM_PRIORITY, RELIABLE_ORDERED, orderingStream, packet->playerId, false );
		else
			if ( rakPeerInterface )
				rakPeerInterface->Send( &outputBitStream, MEDIUM_PRIORITY, RELIABLE_ORDERED, orderingStream, packet->playerId, false );
			else
				rakClientInterface->Send( &outputBitStream, MEDIUM_PRIORITY, RELIABLE_ORDERED, orderingStream );
	}
}

void AutoPatcher::OnAutopatcherRequestFiles( Packet *packet )
{
	assert( rakPeerInterface || rakClientInterface || rakServerInterface );
	
	BitStream serializedFileDescriptor( ( char* ) packet->data, packet->length, false );
	BitStream outputBitStream;
	DownloadableFileDescriptor dfd;
	unsigned int numberOfFilesRequested;
	unsigned index, downloadableFilesIndex;
	unsigned char packetID;
	// Holds a copy of the pointers.  Don't deallocate them!
	BasicDataStructures::List<DownloadableFileDescriptor*> sendList;
	
	// Ignore ID_AUTOPATCHER_REQUEST_FILES
	serializedFileDescriptor.IgnoreBits( sizeof( unsigned char ) * 8 );
	
	if ( serializedFileDescriptor.ReadCompressed( numberOfFilesRequested ) == false )
	{
		// Invalid packet format. Should never get this unless it's a bug or someone is hacking
#ifdef _DEBUG
		assert( 0 );
#endif
		
		return ;
	}
	
	// Go through all the files requested in the packet.
	// If we allow download of it, add the descriptor to a send list which we
	// serialize and send back to the sender telling them what files they will get.
	// This is necessary because it is possible that cheaters will request files
	// not in the list or that files will be removed from downloadable status after an initial
	// successful request for it
	for ( index = 0; index < numberOfFilesRequested; index++ )
	{
		dfd.Clear();
		
		if ( dfd.DeserializeHeader( &serializedFileDescriptor ) == false )
		{
			assert( 0 ); // Error in packet header.  Should only get this from hackers or bugs
			return ;
		}
		
		for ( downloadableFilesIndex = 0; downloadableFilesIndex < downloadableFiles.size(); downloadableFilesIndex++ )
		{
			if ( strcmp( downloadableFiles[ downloadableFilesIndex ] ->filename, dfd.filename ) == 0 )
			{
				// Record that we are going to send this file to system requesting it
				sendList.insert( downloadableFiles[ downloadableFilesIndex ] );
				break;
			}
		}
	}
	
	packetID = ID_AUTOPATCHER_SET_DOWNLOAD_LIST;
	// Serialize the list of files we will send
	outputBitStream.Write( packetID );
	outputBitStream.WriteCompressed( sendList.size() );
	
	for ( index = 0; index < sendList.size(); index++ )
		sendList[ index ] ->SerializeHeader( &outputBitStream );
		
	// Send the list of files
	if ( rakServerInterface )
		rakServerInterface->Send( &outputBitStream, MEDIUM_PRIORITY, RELIABLE_ORDERED, orderingStream, packet->playerId, false );
	else
		if ( rakPeerInterface )
			rakPeerInterface->Send( &outputBitStream, MEDIUM_PRIORITY, RELIABLE_ORDERED, orderingStream, packet->playerId, false );
		else
			rakClientInterface->Send( &outputBitStream, MEDIUM_PRIORITY, RELIABLE_ORDERED, orderingStream );
			
	// The next step is to send the actual files.  We already know what files need to be sent -
	// The files specified by the descriptors in sendList.
	packetID = ID_AUTOPATCHER_WRITE_FILE;
	
	for ( index = 0; index < sendList.size(); index++ )
	{
		// We used outputBitStream earlier so don't forget to reset it
		outputBitStream.Reset();
		outputBitStream.Write( packetID );
		sendList[ index ] ->SerializeHeader( &outputBitStream );
		sendList[ index ] ->SerializeFileData( &outputBitStream );
		
		if ( rakServerInterface )
			rakServerInterface->Send( &outputBitStream, LOW_PRIORITY, RELIABLE_ORDERED, orderingStream, packet->playerId, false );
		else
			if ( rakPeerInterface )
				rakPeerInterface->Send( &outputBitStream, LOW_PRIORITY, RELIABLE_ORDERED, orderingStream, packet->playerId, false );
			else
				rakClientInterface->Send( &outputBitStream, LOW_PRIORITY, RELIABLE_ORDERED, orderingStream );
	}
}

bool AutoPatcher::OnAutopatcherWriteFile( Packet *packet )
{
	DownloadableFileDescriptor dfd;
	unsigned downloadingFilesIndex;
	char *prefixedPath;
	bool prefixedPathAllocated;
	char *uncompressedData;
	BitStream serializedFileDescriptor( ( char* ) packet->data, packet->length, false );
	assert( rakPeerInterface || rakClientInterface || rakServerInterface );
	// Deserialize the header
	// Ignore ID_AUTOPATCHER_WRITE_FILE
	serializedFileDescriptor.IgnoreBits( sizeof( unsigned char ) * 8 );
	dfd.DeserializeHeader( &serializedFileDescriptor );
	dfd.DeserializeFileData( &serializedFileDescriptor );
	// Security - Make sure this file was originally requested
	downloadingFilesIndex = 0;
	
	while ( downloadingFilesIndex < downloadingFiles.size() )
	{
		if ( strcmp( downloadingFiles[ downloadingFilesIndex ] ->filename, dfd.filename ) == 0 )
		{
			// Remove the file from the list of files that we are downloading
			delete downloadingFiles[ downloadingFilesIndex ]; // Destructor takes care of internal arrays
			downloadingFiles.del( downloadingFilesIndex );
			
			if ( downloadPrefix )
			{
				prefixedPath = new char [ strlen( dfd.filename ) + strlen( downloadPrefix ) + 1 ];
				strcpy( prefixedPath, downloadPrefix );
				strcat( prefixedPath, dfd.filename );
				prefixedPathAllocated = true;
			}
			
			else
			{
				prefixedPath = dfd.filename;
				prefixedPathAllocated = false;
			}
			
			// Uncompress the data if it was compressed
			if ( dfd.fileDataIsCompressed )
			{
				uncompressedData = new char [ dfd.fileLength ];
				
				if ( uncompress( ( Bytef * ) uncompressedData, ( uLongf * ) & dfd.fileLength, ( const Bytef * ) dfd.fileData, ( uLong ) dfd.compressedFileLength ) != Z_OK )
				{
					delete [] uncompressedData;
					
					if ( prefixedPath )
						delete [] prefixedPath;
						
					return false;
				}
			}
			
			else
				uncompressedData = dfd.fileData;
				
			// Write the file, with the downloaded file prefix if there is one
			if ( WriteFileWithDirectories( prefixedPath, uncompressedData, dfd.fileLength ) == false )
			{
				assert( 0 ); // File system error?
				
				if ( dfd.fileDataIsCompressed )
					delete [] uncompressedData;
					
				if ( prefixedPathAllocated )
					delete [] prefixedPath;
					
				return false;
			}
			
			if ( dfd.fileDataIsCompressed )
				delete [] uncompressedData;
				
			if ( prefixedPathAllocated )
				delete [] prefixedPath;
				
			return true;
		}
		
		else
			downloadingFilesIndex++;
	}
	
	return false;
}

bool AutoPatcher::WriteFileWithDirectories( const char *path, char *data, unsigned dataLength )
{
	int index;
	FILE *fp;
	char *pathCopy;
#ifndef _WIN32
	
	char *systemCommand;
#endif
	
	if ( path == 0 || path[ 0 ] == 0 || data == 0 || dataLength <= 0 )
		return false;
		
#ifndef _WIN32
		
	systemCommand = new char [ strlen( path ) + 1 + 6 ];
	
#endif
	
	pathCopy = new char [ strlen( path ) + 1 ];
	
	strcpy( pathCopy, path );
	
	index = 0;
	
	while ( pathCopy[ index ] )
	{
		if ( pathCopy[ index ] == '/' || pathCopy[ index ] == '\\' )
		{
			pathCopy[ index ] = 0;
#ifdef _WIN32
			
			mkdir( pathCopy );
#else
			
			mkdir( pathCopy, 0644 );
			//   strcpy(systemCommand, "mkdir ");
			//   strcat(systemCommand, pathCopy);
			//   system(systemCommand);
#endif
			
			pathCopy[ index ] = '/';
		}
		
		index++;
	}
	
	delete [] pathCopy;
#ifndef _WIN32
	
	delete [] systemCommand;
#endif
	
	fp = fopen( path, "wb" );
	
	if ( fp == 0 )
		return false;
		
	fwrite( data, 1, dataLength, fp );
	
	fclose( fp );
	
	return true;
}

// If the packet identifier is ID_AUTOPATCHER_SET_DOWNLOAD_LIST, call this function with
// the packet.
// Finalizes the list of files that will be downloaded.
void AutoPatcher::OnAutopatcherSetDownloadList( Packet *packet )
{
	unsigned numberOfFilesToDownload, index, downloadingFilesIndex;
	DownloadableFileDescriptor dfd;
	BitStream serializedFileDescriptor( ( char* ) packet->data, packet->length, false );
	// Deserialize the packet header
	// Ignore ID_AUTOPATCHER_SET_DOWNLOAD_LIST
	serializedFileDescriptor.IgnoreBits( sizeof( unsigned char ) * 8 );
	
	if ( serializedFileDescriptor.ReadCompressed( numberOfFilesToDownload ) == false )
	{
		// Invalid packet format. Should never get this unless it's a bug or someone is hacking
#ifdef _DEBUG
		assert( 0 );
#endif
		
		return ;
	}
	
	// Flag all existing descriptors so that we know they are invalid
	for ( index = 0; index < downloadingFiles.size(); index++ )
	{
		downloadingFiles[ index ] ->fileLength = 0;
	}
	
	// For each file descriptor
	for ( index = 0; index < numberOfFilesToDownload; index++ )
	{
		// Parse out the info from the packet
		dfd.Clear();
		dfd.DeserializeHeader( &serializedFileDescriptor );
		// Make sure this file was originally requested.
		
		for ( downloadingFilesIndex = 0; downloadingFilesIndex < downloadingFiles.size(); downloadingFilesIndex++ )
		{
			if ( strcmp( downloadingFiles[ downloadingFilesIndex ] ->filename, dfd.filename ) == 0 )
			{
				// Copy over the descriptive data
				downloadingFiles[ downloadingFilesIndex ] ->compressedFileLength = dfd.compressedFileLength;
				downloadingFiles[ downloadingFilesIndex ] ->fileDataIsCompressed = dfd.fileDataIsCompressed;
				downloadingFiles[ downloadingFilesIndex ] ->fileLength = dfd.fileLength;
				break;
			}
		}
		
		// If it was not requested, ignore it.
	}
	
	// If an existing descriptor has the invalid flag set, then remove it from the list
	downloadingFilesIndex = 0;
	
	while ( downloadingFilesIndex < downloadingFiles.size() )
	{
		if ( downloadingFiles[ downloadingFilesIndex ] ->fileLength == 0 )
		{
			delete downloadingFiles[ downloadingFilesIndex ];
			downloadingFiles.del( downloadingFilesIndex );
		}
		
		else
			downloadingFilesIndex++;
	}
}

bool AutoPatcher::GenerateSHA1( char *filename, char SHA1Code[ SHA1_LENGTH ] )
{
	CSHA1 sha1;
	sha1.Reset();
	
	if ( sha1.HashFile( filename ) == false )
		return false;
		
	// Output the code
	sha1.Final();
	
	memcpy( SHA1Code, sha1.GetHash(), SHA1_LENGTH * sizeof( char ) );
	
	return true;
}

int AutoPatcher::GetFileLength( char *filename )
{
	int filesize;
	FILE *fp;
	fp = fopen( filename, "rb" );
	
	if ( fp == 0 )
		return 0;
		
	fseek( fp, 0, SEEK_END );
	
	filesize = ftell( fp );
	
	fclose( fp );
	
	return filesize;
}

unsigned int AutoPatcher::GetDownloadStatus( char *filename, unsigned *fileLength, bool *fileDataIsCompressed, unsigned *compressedFileLength )
{
	if ( downloadingFiles.size() > 0 )
	{
		if ( filename )
			strcpy( filename, downloadingFiles[ 0 ] ->filename );
			
		if ( fileLength )
			* fileLength = downloadingFiles[ 0 ] ->fileLength;
			
		if ( fileDataIsCompressed )
			* fileDataIsCompressed = downloadingFiles[ 0 ] ->fileDataIsCompressed;
			
		if ( compressedFileLength )
			* compressedFileLength = downloadingFiles[ 0 ] ->compressedFileLength;
			
		return downloadingFiles.size();
	}
	
	return 0;
}
