/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @ingroup RAKNET_AUTOPATCHER
 * @brief Autopatcher 
 *
 * Copyright (c) 2003, 2004, Rakkarsoft LLC and Kevin Jenkins
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @defgroup RAKNET_AUTOPATCHER Autopatcher 
 * @brief The Autopatcher provide client automatic patching of data file. 
 * 
 * The Autopatcher subsystem provide automatic file updates to the
 * client.  The server set a list of file available for patching. If
 * the files differs on the client, those file will be automatically
 * updated. It depends on the @em zLib library to compress data. The
 * latest version of this library is provided with the RakNet library.
 * 
 */
#ifndef __AUTO_PATCHER_H
#define __AUTO_PATCHER_H

class RakPeerInterface;

class RakServerInterface;

class RakClientInterface;

struct Packet;

struct DownloadableFileDescriptor;
#include "NetworkTypes.h"
#include "ArrayList.h"
#include "SHA1.h" 
/**
 * @ingroup RAKNET_AUTOPATCHER
 * @brief Result of action from the autopatcher 
 * 
 * This enum describe results for autopatcher operations. 
 */
enum SetFileDownloadableResult
{
	/**
	 * Operation failed 
	 */
	SET_FILE_DOWNLOADABLE_FAILED,
	/**
	 * Signature file is missing 
	 */
	SET_FILE_DOWNLOADABLE_FILE_NO_SIGNATURE_FILE,
	/**
	 * Signature check failed 
	 */
	SET_FILE_DOWNLOADABLE_FILE_SIGNATURE_CHECK_FAILED,
	/**
	 * Compression failed 
	 */
	SET_FILE_DOWNLOADABLE_COMPRESSION_FAILED,
	/**
	 * Success 
	 */
	SET_FILE_DOWNLOADABLE_SUCCESS,
};
/**
 * @ingroup RAKNET_AUTOPATCHER 
 * @brief Autopatcher Subsystem 
 * 
 * This class define the autopatcher facilities. It provide client and server 
 * side features to update file of the server side to the client. Missing files
 * are created while modified files are updated. This mecanism use UDP. It 
 * might be more efficient to use TCP for large file. 
 */

class AutoPatcher
{

public:
	/**
	 * Default Constructor
	 */
	AutoPatcher();
	/**
	 * Destructor
	 */
	~AutoPatcher();
	/**
	 * Frees all downloadable files.
	 * Frees the download directory prefix.
	 */
	void Clear( void );
	/**
	 * Set the ordering stream to send data on.  Defaults to 0.  Set
	 * this if you use ordered data for your game and don't want to hold
	 * up game data because of autopatcher file data
	 * @param streamIndex The stream to use for autopatcher file data 
	 */
	
	void SetOrderingStream( int streamIndex );
	/**
	 * Call this to  use RakPeer on sends. Mutually  exclusive with the
	 * other 2 overloads.
	 * @param localSystem The local communication end point to use 
	 */
	
	void SetNetworkingSystem( RakPeerInterface *localSystem );
	/**
	 * Call this to use RakClientInterface on sends.  Mutually exclusive
	 * with the other 2 overloads.
	 * @param localSystem The local communication end point to use 
	 */
	void SetNetworkingSystem( RakClientInterface *localSystem );
	/**
	 * Call this to use RakServerInterface on sends.  Mutually exclusive
	 * with the other 2 overloads.
	 * @param localSystem The local communication end point to use 
	 */
	void SetNetworkingSystem( RakServerInterface *localSystem );
	/**
	 * Set the value at which files larger than this will be compressed.
	 * Files smaller than this will not be compressed. Defaults to
	 * 1024. Only changes files later passed to SetFileDownloadable,
	 * not files already processed
	 * @param boundary the new boundary for compression activation. 
	 */
	void SetCompressionBoundary( unsigned boundary );
	/**
	 * Creates a .sha file signature for a particular file.
	 * This is used by SetFileDownloadable with checkFileSignature as true
	 * 
	 * @param filename the name of the file 
	 * @return true on success, false on file does not exist.
	 */
	static bool CreateFileSignature( char *filename );
	/**
	 * Makes a file downloadable Returns checkFileSignature - if true
	 * then check the associated .sha1 to make sure it describes our
	 * file.  checkFileSignature is useful to make sure a file wasn't
	 * externally modified by a hacker or a virus 
	 * 
	 * @param filename the name of the file 
	 * @param checkFileSignature check if a file was externaly modified 
	 * @return true on success, false on can't open file
	 */
	SetFileDownloadableResult SetFileDownloadable( char *filename, bool checkFileSignature );
	/**
	 * Removes access to a file previously set as downloadable.
	 * @param filename the name of the file 
	 * @return true on success, false on failure. 
	 */
	bool UnsetFileDownloadable( char *filename );
	/**
	 * Returns how many files are still in the download list.
	 * Requires a previous call to OnAutopatcherFileList
	 * If returns >=1 filename and fileLength will be filled in to match
	 * the current file being downloaded
	 * A value of 0 for compressedFileLength indicates unknown
	 * @param filename the file currently downloaded 
	 * @param fileLength the size of the file currently downloaded 
	 * @param fileDataIsCompressed true if the file is compressed 
	 * @param compressedFileLength the size of the compressed file 
	 * @return the number of file still in the download list. 
	 */
	unsigned int GetDownloadStatus( char *filename, unsigned *fileLength, bool *fileDataIsCompressed, unsigned *compressedFileLength );
	/**
	 * Sets a base directory to look for and put all downloaded files in.
	 * For example, "Downloads"
	 * @param prefix the path to the source or the destination directory. 
	 */
	void SetDownloadedFileDirectoryPrefix( char *prefix );
	/**
	 * Requests that the remote system send the directory of files that
	 * are downloadable.  The remote system should get
	 * ID_AUTOPATCHER_REQUEST_FILE_LIST.  When it does, it should call
	 * SendDownloadableFileList with the playerID of the sender.  For
	 * the client, you can put UNASSIGNED_PLAYER_ID for remoteSystem
	 * @param remoteSystem The remoteSystem to ask for its downloadable
	 * file list.
	 */
	void RequestDownloadableFileList( PlayerID remoteSystem );
	/**
	 * If the packet identifier is ID_AUTOPATCHER_REQUEST_FILE_LIST, call this function with
	 * packet->playerID
	 * Sends a list of all downloadable files to the remote system
	 * The remote system should get a packet with ID ID_AUTOPATCHER_FILE_LIST.
	 * When it does, it should call
	 * OnAutopatcherFileList with the packet from the network
	 * @param remoteSystem the player to send the list to. 
	 */
	void SendDownloadableFileList( PlayerID remoteSystem );
	/**
	 * If the packet identifier is ID_AUTOPATCHER_FILE_LIST, call this
	 * function with the packet.  It will parse out all the files on the
	 * remote system and request to download the ones we don't have or
	 * don't match.  The remote system should get a packet with ID
	 * ID_AUTOPATCHER_REQUEST_FILES for each file requested
	 *
	 * We can specify to only accept files if we previously requested
	 * them by a call to RequestDownloadableFileList.  Set
	 * onlyAcceptFilesIfRequested to true to do this
	 *
	 * After this function is called you can call GetDownloadStatus(...)
	 * To find out which, if any, files are currently downloading
	 * 
	 * @param packet the packet with id ID_AUTOPATCHER_FILE_LIST 
	 * @param onlyAcceptFilesIfRequested true implies we previously ask for the
	 * requested file list.
	 */
	void OnAutopatcherFileList( Packet *packet, bool onlyAcceptFilesIfRequested );
	/**
	 * If the packet identifier is ID_AUTOPATCHER_REQUEST_FILES, call
	 * this function with the packet.  Reads the files from disk and
	 * sends them to the specified system
	 * @param packet the packet with id ID_AUTOPATCHER_REQUEST_FILES
	 */
	void OnAutopatcherRequestFiles( Packet *packet );
	/**
	 * If the packet identifier is ID_AUTOPATCHER_SET_DOWNLOAD_LIST,
	 * call this function with the packet. Finalizes the list of files
	 * that will be downloaded.
	 * @param packet the packet with id ID_AUTOPATCHER_SET_DOWNLOAD_LIST
	 */
	void OnAutopatcherSetDownloadList( Packet *packet );
	/**
	 * If the packet identifier is ID_AUTOPATCHER_WRITE_FILE, call this
	 * function with the packet.  Writes a file to disk.  There is
	 * security to prevent writing files we didn't ask for
	 * @param packet the packet with id ID_AUTOPATCHER_WRITE_FILE
	 * @return true on success
	 */
	bool OnAutopatcherWriteFile( Packet *packet );
	
	/**
	 * @internal 
	 * 
	 * @return true on success 
	 */
	static bool GenerateSHA1( char *filename, char SHA1Code[ SHA1_LENGTH ] );
	/**
	 * @internal 
	 * @return true on success 
	 */
	static int GetFileLength( char *filename );
	/**
	 * @internal 
	 * @return true on success 
	 */
	static bool WriteFileWithDirectories( const char *path, char *data, unsigned dataLength );
	
protected:
	//! One of these 3 must be set in order to send data
	RakPeerInterface *rakPeerInterface;
	//! One of these 3 must be set in order to send data
	RakClientInterface *rakClientInterface;
	//! One of these 3 must be set in order to send data
	RakServerInterface *rakServerInterface;
	//! The directory to download into basically
	char *downloadPrefix;
	//! Files of length >= this will be compressed
	unsigned compressionBoundary;
	//! true if the downloadableFiles have been requested
	bool downloadableFilesRequested;
	//! The ordering stream to use
	int orderingStream;
	//! Directory of files that are downloadable
	BasicDataStructures::List<DownloadableFileDescriptor*> downloadableFiles, downloadingFiles;
};

#endif
