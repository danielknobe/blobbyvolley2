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

#include "ReplaySavePoint.h"

const char validHeader[4] = { 'B', 'V', '2', 'R' };	//!< header of replay file

const unsigned char REPLAY_FILE_VERSION_MAJOR = 1;
const unsigned char REPLAY_FILE_VERSION_MINOR = 1;

// 10 secs for normal gamespeed
const int REPLAY_SAVEPOINT_PERIOD = 750;


/*! \class ChecksumException
	\brief thrown when actual and expected file checksum mismatch
*/
struct ChecksumException : public std::exception
{
	ChecksumException(std::string filename, uint32_t expected, uint32_t real);
	~ChecksumException() throw();

	virtual const char* what() const throw();

	std::string error;
};

/**
	\page replay_system Replay System
	\section rep_file_spec Replay File Specification
	
		Blobby Volley Replay files have the following structure:
		<dl>
		<dt>Replay File V 1.x: </dt>
		<dd>
		<table>
			<tr><th>Offset</th><th>Size</th><th>Values</th><th>Description</th></tr>

			<tr><td colspan=4>File Header</td></tr>
			<tr><td>0</td><td>4 bytes</td><td>bv2r</td><td>Replay file header</td></tr>
			<tr><td>4</td><td>4 bytes</td><td>01xp</td><td>File version information. First byte ist always 0, 
												second byte contains major version (1), x is minor version 
												and p contains additional versioning information. \sa rep_versioning</td></tr>
			<tr><td>8</td><td>4 bytes</td><td>checksum</td><td>contains a checksum of the whole file</td></tr>

			<tr><td colspan=4>Replay Header</td></tr>
			<tr><td>12</td><td>4 bytes</td><td>int</td><td>length of this replay header in bytes</td></tr>
			<tr><td>16</td><td>4 bytes</td><td>pointer</td><td>Points to the starting position of the attributes section</td></tr>
			<tr><td>20</td><td>4 bytes</td><td>int</td><td>Length of attributes section</td></tr>
			<tr><td>24</td><td>4 bytes</td><td>pointer</td><td>Points to the starting position of the jump table</td></tr>
			<tr><td>28</td><td>4 bytes</td><td>int</td><td>Length of jump table section</td></tr>
			<tr><td>32</td><td>4 bytes</td><td>pointer</td><td>Points to the starting position of the data section</td></tr>
			<tr><td>36</td><td>4 bytes</td><td>int</td><td>Length of data section (bytes)</td></tr>

			<tr><td colspan=4>Attributes Section (AS)</td></tr>
			<tr><td>AS+0</td><td>4 bytes</td><td>atr\n</td><td>Attribute beginning indicator</td></tr>
			<tr><td>AS+4</td><td>4 bytes</td><td>int</td><td>Gamespeed</td></tr>
			<tr><td>AS+8</td><td>4 bytes</td><td>int</td><td>Game duration [sec]</td></tr>
			<tr><td>AS+12</td><td>4 bytes</td><td>int</td><td>Game duration [steps]</td></tr>
			<tr><td>AS+16</td><td>4 bytes</td><td>int</td><td>Date of match</td></tr>
			<tr><td>AS+20</td><td>4 bytes</td><td>int</td><td>Left player color</td></tr>
			<tr><td>AS+24</td><td>4 bytes</td><td>int</td><td>Right player color</td></tr>
			<tr><td>AS+28</td><td>4 bytes</td><td>int</td><td>Left player score</td></tr>
			<tr><td>AS+32</td><td>4 bytes</td><td>int</td><td>Right player score</td></tr>
			<tr><td>AS+36</td><td>string</td><td>string</td><td>Left player name</td></tr>
			<tr><td>...</td><td>string</td><td>string</td><td>Right player name</td></tr>
			<tr><td colspan=4>...</td></tr>
			
			<tr><td colspan=4>Jump Table (JT)</td></tr>
			<tr><td>JT+0</td><td>4 bytes</td><td>jpt\n</td><td>Jump table beginning indicator</td></tr>
			<tr><td colspan=4>[currently undefined]</td></tr>
			
			<tr><td colspan=4>Data Section (DS)</td></tr>
			<tr><td>DS+0</td><td>4 bytes</td><td>jpt\n</td><td>Data beginning indicator</td></tr>
			<tr><td>DS+4</td><td>4 bytes</td><td>int</td><td>Length of data section [steps]</td></tr>
			<tr><td>DS+8</td><td>...</td><td>*Packet</td><td>game data</td></tr>
			
		</table></dd></dl>
**/
