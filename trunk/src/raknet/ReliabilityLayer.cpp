/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Reliability Layer Implementation 
 * 
 * Copyright (c) 2003, Rakkarsoft LLC and Kevin Jenkins
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

#include "ReliabilityLayer.h"
#include <assert.h>
#include "GetTime.h"
#include "SocketLayer.h"

// alloca
#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif

// Defined in rand.cpp
extern void seedMT( unsigned int seed );
extern inline unsigned int randomMT( void );
extern inline float frandomMT( void );

static const int ACK_BIT_LENGTH = sizeof( PacketNumberType ) *8 + 1;
static const int MAXIMUM_WINDOW_SIZE = ( 8000 - UDP_HEADER_SIZE ) *8 / ACK_BIT_LENGTH; // Sanity check - the most ack packets that could ever (usually) fit into a frame.
static const int MINIMUM_WINDOW_SIZE = 5; // how many packets can be sent unacknowledged before waiting for an ack
static const int DEFAULT_RECEIVED_PACKETS_SIZE=128; // Divide by timeout time in seconds to get the max ave. packets per second before requiring reallocation

//-------------------------------------------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------------------------------------------
ReliabilityLayer::ReliabilityLayer() : updateBitStream( MAXIMUM_MTU_SIZE )   // preallocate the update bitstream so we can avoid a lot of reallocs at runtime
{
	InitializeVariables();
#ifdef __USE_IO_COMPLETION_PORTS
	readWriteSocket = INVALID_SOCKET;
#endif
	
	freeThreadedMemoryOnNextUpdate = false;
}

//-------------------------------------------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------------------------------------------
ReliabilityLayer::~ReliabilityLayer()
{
	FreeMemory( true ); // Free all memory immediately
#ifdef __USE_IO_COMPLETION_PORTS
	if ( readWriteSocket != INVALID_SOCKET )
		closesocket( readWriteSocket );
#endif
}

//-------------------------------------------------------------------------------------------------------
// Resets the layer for reuse
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::Reset( void )
{
	FreeMemory( true ); // true because making a memory reset pending in the update cycle causes resets after reconnects.  Instead, just call Reset from a single thread
	InitializeVariables();
}

//-------------------------------------------------------------------------------------------------------
// Sets up encryption
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SetEncryptionKey( const unsigned char* key )
{
	if ( key )
		encryptor.SetKey( key );
	else
		encryptor.UnsetKey();
}

//-------------------------------------------------------------------------------------------------------
// Assign a socket for the reliability layer to use for writing
//-------------------------------------------------------------------------------------------------------
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
void ReliabilityLayer::SetSocket( SOCKET s )
{
#ifdef __USE_IO_COMPLETION_PORTS
	// If this hits I am probably using sequential ports while doing IO completion ports
	assert( s != INVALID_SOCKET );
	readWriteSocket = s;
#endif
}

//-------------------------------------------------------------------------------------------------------
// Get the socket held by the reliability layer
//-------------------------------------------------------------------------------------------------------
SOCKET ReliabilityLayer::GetSocket( void )
{
#ifdef __USE_IO_COMPLETION_PORTS
	return readWriteSocket;
#else
	
	return INVALID_SOCKET;
#endif
}

//-------------------------------------------------------------------------------------------------------
// Initialize the variables
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::InitializeVariables( void )
{
	memset( waitingForOrderedPacketReadIndex, 0, NUMBER_OF_ORDERED_STREAMS * sizeof(OrderingIndexType));
	memset( waitingForSequencedPacketReadIndex, 0, NUMBER_OF_ORDERED_STREAMS * sizeof(OrderingIndexType) );
	memset( waitingForOrderedPacketWriteIndex, 0, NUMBER_OF_ORDERED_STREAMS * sizeof(OrderingIndexType) );
	memset( waitingForSequencedPacketWriteIndex, 0, NUMBER_OF_ORDERED_STREAMS * sizeof(OrderingIndexType) );
	memset( &statistics, 0, sizeof( statistics ) );
	statistics.connectionStartTime = RakNet::GetTime();
	splitPacketId = 0;
	packetNumber = 0;
	// lastPacketSendTime=retransmittedFrames=sentPackets=sentFrames=receivedPacketsCount=bytesSent=bytesReceived=0;
	SetLostPacketResendDelay( 1000 );
	deadConnection = cheater = false;
	lastAckTime = 0;
	blockWindowIncreaseUntilTime = 0;
	// Windowing
	windowSize = MINIMUM_WINDOW_SIZE;
	lossyWindowSize = MAXIMUM_WINDOW_SIZE + 1; // Infinite
	lastWindowIncreaseSizeTime = 0;
	// lastPacketReceivedTime=0;
	receivedPacketsBaseIndex=0;
	resetReceivedPackets=true;
}

//-------------------------------------------------------------------------------------------------------
// Frees all allocated memory
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::FreeMemory( bool freeAllImmediately )
{
	if ( freeAllImmediately )
	{
		FreeThreadedMemory();
		FreeThreadSafeMemory();
	}
	else
	{
		FreeThreadSafeMemory();
		freeThreadedMemoryOnNextUpdate = true;
	}
}

void ReliabilityLayer::FreeThreadedMemory( void )
{
}

void ReliabilityLayer::FreeThreadSafeMemory( void )
{
//	unsigned i, j;
	// InternalPacket *internalPacket;
	
	// if (bytesSent > 0 || bytesReceived > 0)
	// {
	
	/*
	for ( i = 0; i < NUMBER_OF_PRIORITIES; i++ )
	{
		j = 0;
		reliabilityLayerMutexes[ sendQueueSystemPriority_MUTEX + i ].Lock();
		
		for ( ; j < sendQueue[ i ].size(); j++ )
		{
			delete [] ( sendQueue[ i ] ) [ j ]->data;
			internalPacketPool.ReleasePointer( ( sendQueue[ i ] ) [ j ] );
		}
		
		sendQueue[ i ].clearAndForceAllocation( 512 ); // Preallocate the send lists so we don't do a bunch of reallocations unnecessarily
		reliabilityLayerMutexes[ sendQueueSystemPriority_MUTEX + i ].Unlock();
	}
	*/
	
	// }
	
	unsigned i;
	InternalPacket *internalPacket;

	for ( i = 0; i < splitPacketList.size(); i++ )
	{
		delete [] splitPacketList[ i ]->data;
		internalPacketPool.ReleasePointer( splitPacketList[ i ] );
	}

	splitPacketList.clear();
	//reliabilityLayerMutexes[splitPacketList_MUTEX].Unlock();
	// }


	// if (bytesSent > 0 || bytesReceived > 0)
	// {
	//reliabilityLayerMutexes[outputQueue_MUTEX].Lock();

	while ( outputQueue.size() > 0 )
	{
		internalPacket = outputQueue.pop();
		delete [] internalPacket->data;
		internalPacketPool.ReleasePointer( internalPacket );
	}

	outputQueue.clearAndForceAllocation( 512 );
	//reliabilityLayerMutexes[outputQueue_MUTEX].Unlock();
	// }

	// if (bytesSent > 0 || bytesReceived > 0)
	// {
	//reliabilityLayerMutexes[orderingList_MUTEX].Lock();

	for ( i = 0; i < orderingList.size(); i++ )
	{
		if ( orderingList[ i ] )
		{
			BasicDataStructures::LinkedList<InternalPacket*>* theList = orderingList[ i ];

			if ( theList )
			{
				while ( theList->size() )
				{
					internalPacket = orderingList[ i ]->pop();
					delete [] internalPacket->data;
					internalPacketPool.ReleasePointer( internalPacket );
				}

				delete theList;
			}
		}
	}

	orderingList.clear();
	//reliabilityLayerMutexes[orderingList_MUTEX].Unlock();
	// }

	// if (bytesSent > 0 || bytesReceived > 0)
	// {
	//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Lock();

	while ( acknowledgementQueue.size() > 0 )
		internalPacketPool.ReleasePointer( acknowledgementQueue.pop() );

	acknowledgementQueue.clearAndForceAllocation( 64 );

	//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Unlock();
	// }


	// if (bytesSent > 0 || bytesReceived > 0)
	// {
	//reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
	while ( resendQueue.size() )
	{
		// The resend Queue can have NULL pointer holes.  This is so we can deallocate blocks without having to compress the array
		internalPacket = resendQueue.pop();

		if ( internalPacket )
		{
			delete [] internalPacket->data;
			internalPacketPool.ReleasePointer( internalPacket );
		}
	}

	resendQueue.clearAndForceAllocation( DEFAULT_RECEIVED_PACKETS_SIZE );
	//reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
	// }

	unsigned j;
	for ( i = 0; i < NUMBER_OF_PRIORITIES; i++ )
	{
		j = 0;
		for ( ; j < sendPacketSet[ i ].size(); j++ )
		{
		delete [] ( sendPacketSet[ i ] ) [ j ]->data;
		internalPacketPool.ReleasePointer( ( sendPacketSet[ i ] ) [ j ] );
		}

		sendPacketSet[ i ].clearAndForceAllocation( 512 ); // Preallocate the send lists so we don't do a bunch of reallocations unnecessarily
	}
	
#ifdef _INTERNET_SIMULATOR
	#pragma message("-- RakNet _INTERNET_SIMULATOR is defined!! --")
	for (unsigned i = 0; i < delayList.size(); i++ )
		delete delayList[ i ];
		 
	delayList.clear();
#endif

	internalPacketPool.ClearPool();
}

//-------------------------------------------------------------------------------------------------------
// Packets are read directly from the socket layer and skip the reliability
//layer  because unconnected players do not use the reliability layer
// This function takes packet data after a player has been confirmed as
//connected.  The game should not use that data directly
// because some data is used internally, such as packet acknowledgement and
//split packets
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::HandleSocketReceiveFromConnectedPlayer( const char *buffer, int length )
{
#ifdef _DEBUG
	assert( !( length <= 0 || buffer == 0 ) );
#endif
	
	if ( length <= 1 || buffer == 0 )   // Length of 1 is a connection request resend that we just ignore
		return true;
		
	int numberOfAcksInFrame = 0;
	unsigned int time;
	bool indexFound;
	int count, size;
	PacketNumberType holeCount;
//	bool duplicatePacket;
	
	// bytesReceived+=length + UDP_HEADER_SIZE;

	UpdateThreadedMemory();
	
	// decode this whole chunk if the decoder is defined.
	if ( encryptor.IsKeySet() )
	{
		if ( encryptor.Decrypt( ( unsigned char* ) buffer, length, ( unsigned char* ) buffer, &length ) == false )
		{
			statistics.bitsWithBadCRCReceived += length * 8;
			statistics.packetsWithBadCRCReceived++;
			return false;
		}
	}
	
	statistics.bitsReceived += length * 8;
	statistics.packetsReceived++;
	
	RakNet::BitStream socketData( (char*)buffer, length, false ); // Convert the incoming data to a bitstream for easy parsing
	
	// time = lastPacketReceivedTime = RakNet::GetTime();
	time = RakNet::GetTime();
	
	
	// Parse the bitstream to create an internal packet
	InternalPacket* internalPacket = CreateInternalPacketFromBitStream( &socketData, time );

	while ( internalPacket )
	{
		if ( internalPacket->isAcknowledgement )
		{
			numberOfAcksInFrame++;
			//reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
			
			if ( resendQueue.size() == 0 )
			{
				//reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
				//reliabilityLayerMutexes[lastAckTime_MUTEX].Lock();
				lastAckTime = 0; // Not resending anything so clear this var so we don't drop the connection on not getting any more acks
				//reliabilityLayerMutexes[lastAckTime_MUTEX].Unlock();
			}
			
			else
			{
				//reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
				
				//reliabilityLayerMutexes[lastAckTime_MUTEX].Lock();
				lastAckTime = time; // Just got an ack.  Record when we got it so we know the connection is alive
				//reliabilityLayerMutexes[lastAckTime_MUTEX].Unlock();
			}
			
			// SHOW - ack received
			//printf("Got Ack for %i. resendQueue.size()=%i sendQueue[0].size() = %i\n",internalPacket->packetNumber, resendQueue.size(), sendQueue[0].size());
			RemovePacketFromResendQueueAndDeleteOlderReliableSequenced( internalPacket->packetNumber );
			
			internalPacketPool.ReleasePointer( internalPacket );
		}
		else
		{
			//   receivedPacketsCount++;
			if ( internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED || internalPacket->reliability == RELIABLE )
			{
				SendAcknowledgementPacket( internalPacket->packetNumber, time );
			}

			// Show all arrived packets
			// printf("Got %i.\n", internalPacket->packetNumber);

			// resetReceivedPackets is set from a non-threadsafe function.
			// We do the actual reset in this function so the data is not modified by multiple threads
			if (resetReceivedPackets)
			{
				receivedPackets.clearAndForceAllocation(DEFAULT_RECEIVED_PACKETS_SIZE);
				receivedPacketsBaseIndex=0;
				resetReceivedPackets=false;
			}

			//printf("internalPacket->packetNumber=%i receivedPacketsBaseIndex=%i\n", internalPacket->packetNumber,receivedPacketsBaseIndex);
			// If the following conditional is true then this either a duplicate packet
			// or an older out of order packet
			// The subtraction unsigned overflow is intentional
			holeCount = (PacketNumberType)(internalPacket->packetNumber-receivedPacketsBaseIndex);
			const int typeRange = (PacketNumberType)-1;

			if (holeCount==0)
			{
				// Got what we were expecting
				if (receivedPackets.size())
					receivedPackets.pop();
				++receivedPacketsBaseIndex;
			}
			else if (holeCount > typeRange-typeRange/2)
			{
				// Underflow - got a packet we have already counted past
#ifdef _DEBUG
				//printf( "Warning: Got old duplicate packet (%i) with ID %i.\n", internalPacket->packetNumber, (unsigned char)internalPacket->data[0] );
#endif
				statistics.duplicateMessagesReceived++;

				// Duplicate packet
				delete [] internalPacket->data;
				internalPacketPool.ReleasePointer( internalPacket );
				goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
			}
			else if (holeCount<receivedPackets.size())
			{
				// Got a higher count out of order packet that was missing in the sequence or we already got
				if (IsReceivedPacketHole(receivedPackets[holeCount], time ))
				{
					// Fill in the hole
					receivedPackets[holeCount]=time;
				}
				else
				{
#ifdef _DEBUG
					// If you see this many times, you forgot Sleep(0) in your main game loop so performance sucks and it is resending needlessly
				//	printf( "Warning: Got out of order duplicate packet (%i) with ID %i.\n", internalPacket->packetNumber, (unsigned char)internalPacket->data[0] );
#endif
					statistics.duplicateMessagesReceived++;

					// Duplicate packet
					delete [] internalPacket->data;
					internalPacketPool.ReleasePointer( internalPacket );
					goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
				}
			}
			else // holeCount>=receivedPackets.size()
			{
				// Got a higher count out of order packet whose packetNumber is higher than we have ever got

				// Add 0 times to the queue until (packetNumber - baseIndex) < queue size.
				while ((PacketNumberType)(holeCount) > receivedPackets.size())
					receivedPackets.push(MakeReceivedPacketHole(time));
				receivedPackets.push(time);
#ifdef _DEBUG
				// If this assert hits then PacketNumberType has overflowed
				assert(receivedPackets.size() < (unsigned int)((PacketNumberType)(-1)));
#endif
			}

			// Pop all expired times.  For each expired time popped, increase the baseIndex
			// Each time is either the actual time a packet was received or a time that is between
			// (time+TIMEOUT_TIME) to (time+TIMEOUT_TIME*2) in the future.  Future times are used as a flag
			// to indicate that we never got this packet so we don't mark it as a duplicate.
			while ( receivedPackets.size()>0 && IsExpiredTime(receivedPackets.peek(), time) )
			{
				receivedPackets.pop();
				++receivedPacketsBaseIndex;
			}

			statistics.messagesReceived++;

			// If the allocated buffer is > DEFAULT_RECEIVED_PACKETS_SIZE and it is 3x greater than the number of elements actually being used
			if (receivedPackets.AllocationSize() > DEFAULT_RECEIVED_PACKETS_SIZE && receivedPackets.AllocationSize() > receivedPackets.size() * 3)
				receivedPackets.compress();

			// Keep on top of deleting old unreliable split packets so they don't clog the list.
			if ( internalPacket->splitPacketCount > 0 )
				DeleteOldUnreliableSplitPackets( time );

			if ( internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == UNRELIABLE_SEQUENCED )
			{
#ifdef _DEBUG
				assert( internalPacket->orderingChannel < NUMBER_OF_ORDERED_STREAMS );
#endif
				
				if ( internalPacket->orderingChannel >= NUMBER_OF_ORDERED_STREAMS )
				{
					// Invalid packet
#ifdef _DEBUG
					printf( "Got invalid packet\n" );
#endif
					
					delete [] internalPacket->data;
					internalPacketPool.ReleasePointer( internalPacket );
					goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
				}
				
				if ( IsOlderOrderedPacket( internalPacket->orderingIndex, waitingForSequencedPacketReadIndex[ internalPacket->orderingChannel ] ) == false )
				{
					statistics.sequencedMessagesInOrder++;
					
					// Check for older packets in the output list. Delete any found
					// UPDATE:
					// Disabled.  We don't have enough info to consistently do this.  Sometimes newer data does supercede
					// older data such as with constantly declining health, but not in all cases.
					// For example, with sequenced unreliable sound packets just because you send a newer one doesn't mean you
					// don't need the older ones because the odds are they will still arrive in order
					/*
					  reliabilityLayerMutexes[outputQueue_MUTEX].Lock();
					  DeleteSequencedPacketsInList(internalPacket->orderingChannel, outputQueue);
					  reliabilityLayerMutexes[outputQueue_MUTEX].Unlock();
					
					  // Check for older packets in the split packet list. Delete any found
					  reliabilityLayerMutexes[splitPacketList_MUTEX].Lock();
					  DeleteSequencedPacketsInList(internalPacket->orderingChannel, splitPacketList, internalPacket->splitPacketId);
					  reliabilityLayerMutexes[splitPacketList_MUTEX].Unlock();
					*/ 
					// Is this a split packet?
					
					if ( internalPacket->splitPacketCount > 0 )
					{
						// Generate the split
						// Verify some parameters to make sure we don't get junk data
#ifdef _DEBUG
						assert( internalPacket->splitPacketIndex < internalPacket->splitPacketCount );
						assert( internalPacket->dataBitLength < MAXIMUM_MTU_SIZE * 8 );
#endif					
						//reliabilityLayerMutexes[splitPacketList_MUTEX].Lock();
						for ( unsigned cnt = 0; cnt < splitPacketList.size(); cnt++ )
						{
							// Make sure this is not a duplicate insertion.
							// If this hits then most likely splitPacketId overflowed into existing waiting split packets (i.e. more than rangeof(splitPacketId) waiting)
							if ( splitPacketList[ cnt ]->splitPacketIndex == internalPacket->splitPacketIndex && splitPacketList[ cnt ]->splitPacketId == splitPacketId )
							{
								#ifdef _DEBUG
								assert(0);
								#endif

								// Invalid packet
#ifdef _DEBUG
								printf( "Error: Split packet duplicate insertion (1)\n" );
#endif
								delete [] internalPacket->data;
								internalPacketPool.ReleasePointer( internalPacket );
								goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
							}
						}

#ifdef _DEBUG
						int splitPacketListSize = splitPacketList.size() + 1;
#endif
						//reliabilityLayerMutexes[splitPacketList_MUTEX].Unlock();

						
						// Check for a rebuilt packet
						InsertIntoSplitPacketList( internalPacket );
						
						// Sequenced
						internalPacket = BuildPacketFromSplitPacketList( internalPacket->splitPacketId, time );
						
						if ( internalPacket )
						{
#ifdef _DEBUG
							//reliabilityLayerMutexes[splitPacketList_MUTEX].Lock();
							assert( splitPacketList.size() == splitPacketListSize - internalPacket->splitPacketCount );
							//reliabilityLayerMutexes[splitPacketList_MUTEX].Unlock();
#endif
							// Update our index to the newest packet
							waitingForSequencedPacketReadIndex[ internalPacket->orderingChannel ] = internalPacket->orderingIndex + 1;
							
							// If there is a rebuilt packet, add it to the output queue
							//       reliabilityLayerMutexes[outputQueue_MUTEX].Lock();
							outputQueue.push( internalPacket );
							//       reliabilityLayerMutexes[outputQueue_MUTEX].Unlock();
							internalPacket = 0;
						}
						
#ifdef _DEBUG
						else
						{
							//reliabilityLayerMutexes[splitPacketList_MUTEX].Lock();
							assert( splitPacketList.size() == (unsigned int) splitPacketListSize );
							//reliabilityLayerMutexes[splitPacketList_MUTEX].Unlock();
						}
						
#endif
						// else don't have all the parts yet
					}
					
					else
					{
						// Update our index to the newest packet
						waitingForSequencedPacketReadIndex[ internalPacket->orderingChannel ] = internalPacket->orderingIndex + 1;
						
						// Not a split packet. Add the packet to the output queue
						//      reliabilityLayerMutexes[outputQueue_MUTEX].Lock();
						outputQueue.push( internalPacket );
						//      reliabilityLayerMutexes[outputQueue_MUTEX].Unlock();
						internalPacket = 0;
					}
				}
				else
				{
					statistics.sequencedMessagesOutOfOrder++;
					
					// Older sequenced packet. Discard it
					delete [] internalPacket->data;
					internalPacketPool.ReleasePointer( internalPacket );
				}
				
				goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
			}
			
			// Is this an unsequenced split packet?
			if ( internalPacket->splitPacketCount > 0 )
			{
				// An unsequenced split packet.  May be ordered though.
				
				// Verify some parameters to make sure we don't get junk data
#ifdef _DEBUG
				assert( internalPacket->splitPacketIndex < internalPacket->splitPacketCount );
				assert( internalPacket->dataBitLength < MAXIMUM_MTU_SIZE * 8 );
#endif
				
				// Check for a rebuilt packet
				if ( internalPacket->reliability != RELIABLE_ORDERED )
					internalPacket->orderingChannel = 255; // Use 255 to designate not sequenced and not ordered

				//reliabilityLayerMutexes[splitPacketList_MUTEX].Lock();
				// Make sure this is not a duplicate insertion.  If this assert hits then splitPacketId overflowed into existing waiting split packets (i.e. more than rangeof(splitPacketId) waiting)
				for ( unsigned cnt = 0; cnt < splitPacketList.size(); cnt++ )
				{
					if ( splitPacketList[ cnt ]->splitPacketIndex == internalPacket->splitPacketIndex && splitPacketList[ cnt ]->splitPacketId == internalPacket->splitPacketId )
					{
#ifdef _DEBUG
						assert(0);
#endif
						// Invalid packet
#ifdef _DEBUG
						printf( "Error: Split packet duplicate insertion (2)\n" );
#endif
						delete [] internalPacket->data;
						internalPacketPool.ReleasePointer( internalPacket );
						goto CONTINUE_SOCKET_DATA_PARSE_LOOP;

					}
				}

#ifdef _DEBUG
				int splitPacketListSize = splitPacketList.size() + 1;
				//reliabilityLayerMutexes[splitPacketList_MUTEX].Unlock();
#endif
				
				InsertIntoSplitPacketList( internalPacket );
				
				internalPacket = BuildPacketFromSplitPacketList( internalPacket->splitPacketId, time );
				
				if ( internalPacket == 0 )
				{
#ifdef _DEBUG
					//reliabilityLayerMutexes[splitPacketList_MUTEX].Lock();
					assert( splitPacketList.size() == (unsigned int) splitPacketListSize );
					//reliabilityLayerMutexes[splitPacketList_MUTEX].Unlock();
#endif
					// Don't have all the parts yet
					goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
				}
				
#ifdef _DEBUG
				else
				{
					//reliabilityLayerMutexes[splitPacketList_MUTEX].Lock();
					assert( splitPacketList.size() == splitPacketListSize - internalPacket->splitPacketCount );
					//reliabilityLayerMutexes[splitPacketList_MUTEX].Unlock();
				}
				
#endif
				// else continue down to handle RELIABLE_ORDERED
			}
			
			if ( internalPacket->reliability == RELIABLE_ORDERED )
			{
#ifdef _DEBUG
				assert( internalPacket->orderingChannel < NUMBER_OF_ORDERED_STREAMS );
#endif

				if ( internalPacket->orderingChannel >= NUMBER_OF_ORDERED_STREAMS )
				{
#ifdef _DEBUG
					printf("Got invalid ordering channel %i from packet %i\n", internalPacket->orderingChannel, internalPacket->packetNumber);
#endif
					// Invalid packet
					delete [] internalPacket->data;
					internalPacketPool.ReleasePointer( internalPacket );
					goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
				}
				
				if ( waitingForOrderedPacketReadIndex[ internalPacket->orderingChannel ] == internalPacket->orderingIndex )
				{
					// Get the list to hold ordered packets for this stream
					BasicDataStructures::LinkedList<InternalPacket*> *orderingListAtOrderingStream;
					unsigned char orderingChannelCopy = internalPacket->orderingChannel;
					
					statistics.orderedMessagesInOrder++;

					// Show ordering index increment
					//printf("Pushing immediate packet %i with ordering index %i\n", internalPacket->packetNumber, internalPacket->orderingIndex );

					// Push the packet for the user to read
					//     reliabilityLayerMutexes[outputQueue_MUTEX].Lock();
					outputQueue.push( internalPacket );
					//     reliabilityLayerMutexes[outputQueue_MUTEX].Unlock();
					internalPacket = 0; // Don't reference this any longer since other threads access it

					// Wait for the next ordered packet in sequence
					waitingForOrderedPacketReadIndex[ orderingChannelCopy ] ++; // This wraps
					
					//reliabilityLayerMutexes[orderingList_MUTEX].Lock();
					orderingListAtOrderingStream = GetOrderingListAtOrderingStream( orderingChannelCopy );
					
					if ( orderingListAtOrderingStream != 0)
					{
						while ( orderingListAtOrderingStream->size() > 0 )
						{
							// Cycle through the list until nothing is found
							orderingListAtOrderingStream->beginning();
							indexFound=false;
							size=orderingListAtOrderingStream->size();
							count=0;

							while (count++ < size)
							{
								if ( orderingListAtOrderingStream->peek()->orderingIndex == waitingForOrderedPacketReadIndex[ orderingChannelCopy ] )
								{
									//printf("Pushing delayed packet %i with ordering index %i. outputQueue.size()==%i\n", orderingListAtOrderingStream->peek()->packetNumber, orderingListAtOrderingStream->peek()->orderingIndex, outputQueue.size() );
									outputQueue.push( orderingListAtOrderingStream->pop() );
									waitingForOrderedPacketReadIndex[ orderingChannelCopy ]++; // This wraps at 255
									indexFound=true;
								}
								else
									(*orderingListAtOrderingStream)++;
							}

							if (indexFound==false)
								break;
						}
					}
					
					internalPacket = 0;
				}
				else
				{
				//	assert(waitingForOrderedPacketReadIndex[ internalPacket->orderingChannel ] < internalPacket->orderingIndex);
					statistics.orderedMessagesOutOfOrder++;

					// This is a newer ordered packet than we are waiting for. Store it for future use
					AddToOrderingList( internalPacket );
				}
				
				goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
			}
			
			// Nothing special about this packet.  Add it to the output queue
			//   reliabilityLayerMutexes[outputQueue_MUTEX].Lock();
			outputQueue.push( internalPacket );
			
			//   reliabilityLayerMutexes[outputQueue_MUTEX].Unlock();
			
			// Output queue fill rate test
			//   if (outputQueue.size()%50==0)
			//    printf("outputQueue.size()=%i Time=%i\n", outputQueue.size(), RakNet::GetTime());
			internalPacket = 0;
		}
		
		// Used for a goto to jump to the next packet immediately
		
	CONTINUE_SOCKET_DATA_PARSE_LOOP:
		// Parse the bitstream to create an internal packet
		internalPacket = CreateInternalPacketFromBitStream( &socketData, time );
	}
	
	// numberOfAcksInFrame>=windowSize means that all the packets we last sent from the resendQueue are cleared out
	// 11/17/05 - the problem with numberOfAcksInFrame >= windowSize is that if the entire frame is filled with resends but not all resends filled the frame
	// then the sender is limited by how many resends can fit in one frame
	if ( numberOfAcksInFrame >= windowSize && ( sendPacketSet[ SYSTEM_PRIORITY ].size() > 0 || sendPacketSet[ HIGH_PRIORITY ].size() > 0 || sendPacketSet[ MEDIUM_PRIORITY ].size() > 0 ) )
	{
		// reliabilityLayerMutexes[windowSize_MUTEX].Lock();
		//printf("windowSize=%i lossyWindowSize=%i\n", windowSize, lossyWindowSize);
		
		if ( windowSize < lossyWindowSize || (time>lastWindowIncreaseSizeTime && time-lastWindowIncreaseSizeTime>lostPacketResendDelay*2) )   // Increases the window size slowly, testing for packetloss
		{
			// If we get a frame which clears out the resend queue after handling one or more acks, and we have packets waiting to go out,
			// and we didn't recently lose a packet then increase the window size by 1
			windowSize++;
			
			if ( (time>lastWindowIncreaseSizeTime && time-lastWindowIncreaseSizeTime>lostPacketResendDelay*2) )   // The increase is to test for packetloss
				lastWindowIncreaseSizeTime = time;
				
			// If the window is so large that we couldn't possibly fit any more packets into the frame, then just leave it alone
			if ( windowSize > MAXIMUM_WINDOW_SIZE )
				windowSize = MAXIMUM_WINDOW_SIZE;
				
			// SHOW - WINDOWING
			//else
			//	printf("Increasing windowSize to %i.  Lossy window size = %i\n", windowSize, lossyWindowSize);
			
			// If we are more than 5 over the lossy window size, increase the lossy window size by 1
			if ( windowSize == MAXIMUM_WINDOW_SIZE || windowSize - lossyWindowSize > 5 )
				lossyWindowSize++;
		}
		// reliabilityLayerMutexes[windowSize_MUTEX].Unlock();
	}
	
	return true;
}

//-------------------------------------------------------------------------------------------------------
// This gets an end-user packet already parsed out. Returns number of BITS put into the buffer
//-------------------------------------------------------------------------------------------------------
int ReliabilityLayer::Receive( char **data )
{
	// Wait until the clear occurs
	if (freeThreadedMemoryOnNextUpdate)
		return 0;

	InternalPacket * internalPacket;
	
	// reliabilityLayerMutexes[outputQueue_MUTEX].Lock();
	
	if ( outputQueue.size() > 0 )
	{
		//  #ifdef _DEBUG
		//  assert(bitStream->GetNumberOfBitsUsed()==0);
		//  #endif
		internalPacket = outputQueue.pop();
		//  reliabilityLayerMutexes[outputQueue_MUTEX].Unlock();
		
		int bitLength;
		*data = internalPacket->data;
		bitLength = internalPacket->dataBitLength;
		internalPacketPool.ReleasePointer( internalPacket );
		return bitLength;
	}
	
	else
	{
		// reliabilityLayerMutexes[outputQueue_MUTEX].Unlock();
		return 0;
	}
	
}

//-------------------------------------------------------------------------------------------------------
// Puts data on the send queue
// bitStream contains the data to send
// priority is what priority to send the data at
// reliability is what reliability to use
// ordering channel is from 0 to 255 and specifies what stream to use
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::Send( char *data, int numberOfBitsToSend, PacketPriority priority, PacketReliability reliability, unsigned char orderingChannel, bool makeDataCopy, int MTUSize, unsigned int currentTime )
{
#ifdef _DEBUG
	assert( !( reliability > RELIABLE_SEQUENCED || reliability < 0 ) );
	assert( !( priority > NUMBER_OF_PRIORITIES || priority < 0 ) );
	assert( !( orderingChannel < 0 || orderingChannel >= NUMBER_OF_ORDERED_STREAMS ) );
	assert( numberOfBitsToSend > 0 );
#endif
	
#ifdef __USE_IO_COMPLETION_PORTS
	
	if ( readWriteSocket == INVALID_SOCKET )
		return false;
		
#endif
		
	// Fix any bad parameters
	if ( reliability > RELIABLE_SEQUENCED || reliability < 0 )
		reliability = RELIABLE;
		
	if ( priority > NUMBER_OF_PRIORITIES || priority < 0 )
		priority = HIGH_PRIORITY;
		
	if ( orderingChannel >= NUMBER_OF_ORDERED_STREAMS )
		orderingChannel = 0;

	int numberOfBytesToSend=BITS_TO_BYTES(numberOfBitsToSend);
	if ( numberOfBitsToSend == 0 )
	{
#ifdef _DEBUG
		printf( "Error!! ReliabilityLayer::Send bitStream->GetNumberOfBytesUsed()==0\n" );
#endif
		
		return false;
	}
	
	InternalPacket * internalPacket = internalPacketPool.GetPointer();
	//InternalPacket * internalPacket = sendPacketSet[priority].WriteLock();
#ifdef _DEBUG
	// Remove accessing undefined memory warning
	memset( internalPacket, 255, sizeof( InternalPacket ) );
#endif
	
	internalPacket->creationTime = currentTime;
	
	if ( makeDataCopy )
	{
		internalPacket->data = new char [ numberOfBytesToSend ];
		memcpy( internalPacket->data, data, numberOfBytesToSend );
//		printf("Allocated %i\n", internalPacket->data);
	}
	else
	{
		// Allocated the data elsewhere, delete it in here
		internalPacket->data = ( char* ) data;
//		printf("Using Pre-Allocated %i\n", internalPacket->data);
	}
		
	internalPacket->dataBitLength = numberOfBitsToSend;
	internalPacket->isAcknowledgement = false;
	internalPacket->nextActionTime = 0;
	
	//reliabilityLayerMutexes[ packetNumber_MUTEX ].Lock();
	internalPacket->packetNumber = packetNumber;
	//reliabilityLayerMutexes[ packetNumber_MUTEX ].Unlock();
	
	internalPacket->priority = priority;
	internalPacket->reliability = reliability;
	internalPacket->splitPacketCount = 0;
	
	// Calculate if I need to split the packet
	int headerLength = BITS_TO_BYTES( GetBitStreamHeaderLength( internalPacket ) );
	
	int maxDataSize = MTUSize - UDP_HEADER_SIZE - headerLength;
	
	if ( encryptor.IsKeySet() )
		maxDataSize -= 16; // Extra data for the encryptor
		
	bool splitPacket = numberOfBytesToSend > maxDataSize;
	
	// If a split packet, we might have to upgrade the reliability
	if ( splitPacket )
		statistics.numberOfSplitMessages++;
	else
		statistics.numberOfUnsplitMessages++;
		
	// Increment the cyclical receivedPacketsIndex for use by the next packet.
	// This variable is used as the identifier of the packet on the remote machine.
	// When it cycles it will reuse older numbers but that is ok because by the time it
	// cycles those older packets will be pretty much guaranteed to arrive by then
	//reliabilityLayerMutexes[ packetNumber_MUTEX ].Lock();
	
//	if ( ++packetNumber == RECEIVED_PACKET_LOG_LENGTH )
//		packetNumber = 0;

	++packetNumber;
		
	//reliabilityLayerMutexes[ packetNumber_MUTEX ].Unlock();
	
	if ( internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == UNRELIABLE_SEQUENCED )
	{
		// Assign the sequence stream and index
		internalPacket->orderingChannel = orderingChannel;
		//reliabilityLayerMutexes[ waitingForSequencedPacketWriteIndex_MUTEX ].Lock();
		internalPacket->orderingIndex = waitingForSequencedPacketWriteIndex[ orderingChannel ] ++;
		//reliabilityLayerMutexes[ waitingForSequencedPacketWriteIndex_MUTEX ].Unlock();
		
		// This packet supersedes all other sequenced packets on the same ordering channel
		// Delete all packets in all send lists that are sequenced and on the same ordering channel
		// UPDATE:
		// Disabled.  We don't have enough info to consistently do this.  Sometimes newer data does supercede
		// older data such as with constantly declining health, but not in all cases.
		// For example, with sequenced unreliable sound packets just because you send a newer one doesn't mean you
		// don't need the older ones because the odds are they will still arrive in order
		/*
		  for (int i=0; i < NUMBER_OF_PRIORITIES; i++)
		  {
		  reliabilityLayerMutexes[sendQueue_MUTEX].Lock();
		  DeleteSequencedPacketsInList(orderingChannel, sendQueue[i]);
		  reliabilityLayerMutexes[sendQueue_MUTEX].Unlock();
		  }
		*/
	}
	
	else
		if ( internalPacket->reliability == RELIABLE_ORDERED )
		{
			// Assign the ordering channel and index
			internalPacket->orderingChannel = orderingChannel;
			//reliabilityLayerMutexes[ waitingForOrderedPacketWriteIndex_MUTEX ].Lock();
			internalPacket->orderingIndex = waitingForOrderedPacketWriteIndex[ orderingChannel ] ++;
			//reliabilityLayerMutexes[ waitingForOrderedPacketWriteIndex_MUTEX ].Unlock();
		}
		
	if ( splitPacket )   // If it uses a secure header it will be generated here
	{
		// Must split the packet.  This will also generate the SHA1 if it is required. It also adds it to the send list.
		//InternalPacket packetCopy;
		//memcpy(&packetCopy, internalPacket, sizeof(InternalPacket));
		//sendPacketSet[priority].CancelWriteLock(internalPacket);
		//SplitPacket( &packetCopy, MTUSize );
		SplitPacket( internalPacket, MTUSize );
		//delete [] packetCopy.data;
		return true;
	}
	
//	reliabilityLayerMutexes[ sendQueueSystemPriority_MUTEX + internalPacket->priority ].Lock();
	sendPacketSet[ internalPacket->priority ].push( internalPacket );
//	reliabilityLayerMutexes[ sendQueueSystemPriority_MUTEX + internalPacket->priority ].Unlock();

//	sendPacketSet[priority].WriteUnlock();

	return true;
}

//-------------------------------------------------------------------------------------------------------
// Run this once per game cycle.  Handles internal lists and actually does the send
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::Update( SOCKET s, PlayerID playerId, int MTUSize, unsigned int time )
{
#ifdef __USE_IO_COMPLETION_PORTS

	if ( readWriteSocket == INVALID_SOCKET )
		return;

	if (deadConnection)
		return;

#endif
	// unsigned resendQueueSize;
	bool reliableDataSent;
	
	unsigned int lastAck;

	UpdateThreadedMemory();

	// Accuracy isn't important on this value, and since this is called so often the mutex is sometimes causing deadlock problems.
	// So it is presently disabled
	// reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
	// resendQueueSize=resendQueue.size();
	// reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
	
	// reliabilityLayerMutexes[lastAckTime_MUTEX].Lock();
	lastAck = lastAckTime;
	
	// reliabilityLayerMutexes[lastAckTime_MUTEX].Unlock();
	
	// Due to thread vagarities and the way I store the time to avoid slow calls to RakNet::GetTime
	// time may be less than lastAck
	if ( resendQueue.size() > 0 && time > lastAck && lastAck && time - lastAck > TIMEOUT_TIME )
	{
		// SHOW - dead connection
		// printf("The connection has been lost.\n");
		// We've waited a very long time for a reliable packet to get an ack and it never has
		deadConnection = true;
		return;
	}

	//if (outputWindowFullTime && RakNet::GetTime() > TIMEOUT_TIME + outputWindowFullTime)
	//{
	// // We've waited a long time with no data from the other system.  Assume the connection is lost
	// deadConnection=true;
	// return;
	//}
	
	// Not a frame but a packet actually.
	// However, in a sense it is a frame because we are filling multiple logical packets into one datagram
	//reliabilityLayerMutexes[updateBitStream_MUTEX].Lock();
	
	// Keep sending to available bandwidth
	while ( IsFrameReady( time ) )
	{
		updateBitStream.Reset();
		GenerateFrame( &updateBitStream, MTUSize, &reliableDataSent, time );
		
		if ( updateBitStream.GetNumberOfBitsUsed() > 0 )
		{
#ifndef _INTERNET_SIMULATOR
			SendBitStream( s, playerId, &updateBitStream );
#else
			// Delay the send to simulate lag
			DataAndTime *dt;
			dt = new DataAndTime;
			memcpy( dt->data, updateBitStream.GetData(), updateBitStream.GetNumberOfBytesUsed() );
			dt->length = updateBitStream.GetNumberOfBytesUsed();
			dt->sendTime = time + 1 + ( randomMT() % 100 );
			delayList.insert( dt );
#endif
			
		}
		else
			break;
	}	

#ifdef _INTERNET_SIMULATOR
	// Do any lagged sends
	unsigned i = 0;
	
	while ( i < delayList.size() )
	{
		if ( delayList[ i ]->sendTime < time )
		{
			updateBitStream.Reset();
			updateBitStream.Write( delayList[ i ]->data, delayList[ i ]->length );
			// Send it now
			SendBitStream( s, playerId, &updateBitStream );
			
			delete delayList[ i ];
			if (i != delayList.size() - 1)
				delayList[ i ] = delayList[ delayList.size() - 1 ];
			delayList.del();
		}
		
		else
			i++;
	}
	
#endif
	
	//reliabilityLayerMutexes[updateBitStream_MUTEX].Unlock();
}

//-------------------------------------------------------------------------------------------------------
// Writes a bitstream to the socket
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SendBitStream( SOCKET s, PlayerID playerId, RakNet::BitStream *bitStream )
{
	// SHOW - showing reliable flow
	// if (bitStream->GetNumberOfBytesUsed()>50)
	//  printf("Sending %i bytes. sendQueue[0].size()=%i, resendQueue.size()=%i\n", bitStream->GetNumberOfBytesUsed(), sendQueue[0].size(),resendQueue.size());
	
	int oldLength, length;
	
	// sentFrames++;
	
#ifdef _INTERNET_SIMULATOR
	
	// packetloss
	//if (windowSize>MINIMUM_WINDOW_SIZE && frandomMT() <= (float)(windowSize-MINIMUM_WINDOW_SIZE)/(float)(MAXIMUM_WINDOW_SIZE-MINIMUM_WINDOW_SIZE))
	if (frandomMT() <= .05f)
	{
	// printf("Frame %i lost\n", sentFrames);
	return;
	}
#endif
	
	
	// Encode the whole bitstream if the encoder is defined.
	
	if ( encryptor.IsKeySet() )
	{
		length = bitStream->GetNumberOfBytesUsed();
		oldLength = length;
		
		encryptor.Encrypt( ( unsigned char* ) bitStream->GetData(), length, ( unsigned char* ) bitStream->GetData(), &length );
		statistics.encryptionBitsSent = ( length - oldLength ) * 8;
		
		assert( ( length % 16 ) == 0 );
	}
	
	else
	{
		length = bitStream->GetNumberOfBytesUsed();
	}
	
#ifdef __USE_IO_COMPLETION_PORTS
	if ( readWriteSocket == INVALID_SOCKET )
	{
		assert( 0 );
		return ;
	}
	
	statistics.packetsSent++;
	statistics.totalBitsSent += length * 8;
	SocketLayer::Instance()->Write( readWriteSocket, ( const char* ) bitStream->GetData(), length );
#else
	
	statistics.packetsSent++;
	statistics.totalBitsSent += length * 8;
	//printf("total bits=%i length=%i\n", BITS_TO_BYTES(statistics.totalBitsSent), length);

	SocketLayer::Instance()->SendTo( s, ( char* ) bitStream->GetData(), length, playerId.binaryAddress, playerId.port );
#endif // __USE_IO_COMPLETION_PORTS
	
	// lastPacketSendTime=time;
}

//-------------------------------------------------------------------------------------------------------
// Returns true if we can or should send a frame.  False if we should not
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsFrameReady( unsigned int time )
{
	if ( IsSendThrottled() == false )
	{
		// Show send throttled
		// printf("Send is throttled. resendQueue.size=%i windowSize=%i\n", resendQueue.size(), windowSize);
		return true;
	}
		
	//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Lock();
	
	// Any acknowledgement packets waiting?  We will send these even if the send is throttled.
	// Otherwise the throttle may never end
	if ( acknowledgementQueue.size() >= MINIMUM_WINDOW_SIZE
		// Try not waiting to send acks - will take more bandwidth but maybe less packetloss
		// || acknowledgementQueue.peek()->nextActionTime < time
	   )
	{
		//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Unlock();
		return true;
	}
	
	// reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Unlock();
	
	// Does the oldest packet need to be resent?  If so, send it.
	// Otherwise the throttle may never end
	// reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
	if ( resendQueue.size() > 0 && resendQueue.peek() && resendQueue.peek()->nextActionTime < time )
	{
		//  reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
		return true;
	}
	
	// reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
	
	// Send is throttled.  Don't send.
	return false;
}

//-------------------------------------------------------------------------------------------------------
// Generates a frame (coalesced packets)
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::GenerateFrame( RakNet::BitStream *output, int MTUSize, bool *reliableDataSent, unsigned int time )
{
	InternalPacket * internalPacket;
	int maxDataBitSize;
	int reliableBits = 0;
	int nextPacketBitLength;
	unsigned i;
	bool isReliable, onlySendUnreliable;
	bool acknowledgementPacketsSent;
	bool anyPacketsLost = false;
	
	maxDataBitSize = MTUSize - UDP_HEADER_SIZE;
	
	if ( encryptor.IsKeySet() )
		maxDataBitSize -= 16; // Extra data for the encryptor
		
	maxDataBitSize <<= 3;
	
	acknowledgementPacketsSent = false;
	
	*reliableDataSent = false;
	
	
	// Packet acknowledgements always go out first if they are overdue or if there are a lot of them
	//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Lock();
	// reliabilityLayerMutexes[remoteFramesAwaitingAck_MUTEX].Lock();
	if ( acknowledgementQueue.size() > 0 &&
		( acknowledgementQueue.size() >= MINIMUM_WINDOW_SIZE ||
		  acknowledgementQueue.peek()->nextActionTime < time ) )
	{
		do
		{
			// reliabilityLayerMutexes[remoteFramesAwaitingAck_MUTEX].Unlock();
			internalPacket = acknowledgementQueue.pop();
			//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Unlock();
			
			// Write the acknowledgement to the output bitstream
			statistics.acknowlegementsSent++;
			statistics.acknowlegementBitsSent += WriteToBitStreamFromInternalPacket( output, internalPacket );
			acknowledgementPacketsSent = true;
			
			// Delete the acknowledgement
			internalPacketPool.ReleasePointer( internalPacket );
			
			if ( output->GetNumberOfBitsUsed() + ACK_BIT_LENGTH > maxDataBitSize )
			{
				// SHOW - show ack
				// printf("Sending FULL ack (%i) at time %i. acknowledgementQueue.size()=%i\n", output->GetNumberOfBytesUsed(), RakNet::GetTime(),acknowledgementQueue.size());
				
				statistics.packetsContainingOnlyAcknowlegements++;
				// Show - Frame full
//				printf("Frame full in sending acks\n");
				goto END_OF_GENERATE_FRAME;
			}
			
			//  reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Lock();
		}
		
		while ( acknowledgementQueue.size() > 0 );
	}
	
	//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Unlock();
	
	
	
	// SHOW - show ack
	//if (output->GetNumberOfBitsUsed()>0)
	// printf("Sending ack (%i) at time %i. acknowledgementQueue.size()=%i\n", output->GetNumberOfBytesUsed(), RakNet::GetTime(),acknowledgementQueue.size());
	
	//reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
	
	// The resend Queue can have NULL pointer holes.  This is so we can deallocate blocks without having to compress the array
	while ( resendQueue.size() > 0 )
	{
		if ( resendQueue.peek() == 0 )
		{
			resendQueue.pop();
			continue; // This was a hole
		}
		
		if ( resendQueue.peek()->nextActionTime < time )
		{
			internalPacket = resendQueue.pop();
			//reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
			// Testing
			//printf("Resending %i. queue size = %i\n", internalPacket->packetNumber, resendQueue.size());
			
			nextPacketBitLength = GetBitStreamHeaderLength( internalPacket ) + internalPacket->dataBitLength;
			
			if ( output->GetNumberOfBitsUsed() + nextPacketBitLength > maxDataBitSize )
			{
				//reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
				resendQueue.pushAtHead( internalPacket ); // Not enough room to use this packet after all!
				//reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
				
				if ( anyPacketsLost )
				{
					UpdatePacketloss( time );
				}
				
				// Show - Frame full
				//printf("Frame full in sending resends\n");
				goto END_OF_GENERATE_FRAME;
			}
			
#ifdef _DEBUG
			assert( internalPacket->priority >= 0 );
			
			assert( internalPacket->reliability >= 0 );
			
#endif
			
			// SHOW - show resends
			//printf("Resending packet. resendQueue.size()=%i. Data=%s\n",resendQueue.size(), internalPacket->data);
			
			// Write to the output bitstream
			//   sentPackets++;
			statistics.messageResends++;
			statistics.messageDataBitsResent += internalPacket->dataBitLength;
			statistics.messagesTotalBitsResent += WriteToBitStreamFromInternalPacket( output, internalPacket );
			
			*reliableDataSent = true;
			
			//  if (output->GetNumberOfBitsUsed() + ACK_BIT_LENGTH > maxDataBitSize)
			 // printf("Frame full of just acks and resends at time %i.\n", RakNet::GetTime());
			
			statistics.packetsContainingOnlyAcknowlegementsAndResends++;
			anyPacketsLost = true;
			internalPacket->nextActionTime = time + lostPacketResendDelay;

			// Put the packet back into the resend list at the correct spot
			// Don't make a copy since I'm reinserting an allocated struct
			InsertPacketIntoResendQueue( internalPacket, time, false, false );
			
			//reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
		}
		else
		{
			break;
		}
	}
	
	//reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
	
	if ( anyPacketsLost )
	{
		// Update packetloss
		UpdatePacketloss( time );
	}
	
	onlySendUnreliable = false;
	
	if ( IsSendThrottled() )
		return ; // Don't send regular data if we are supposed to be waiting on the window
		
	// From highest to lowest priority, fill up the output bitstream from the send lists
	for ( i = 0; i < NUMBER_OF_PRIORITIES; i++ )
	{
		// if (i==LOW_PRIORITY && sendQueue[LOW_PRIORITY].size() > 0 && (sendQueue[LOW_PRIORITY].size()%100)==0)
		// {
		//  printf("%i\n", sendQueue[LOW_PRIORITY].size());
		// }
		
		// Not mutexed - may give a wrong value if another thread is inserting something but it's ok
		// Because we can avoid a slow mutex call a lot of the time
		
		//if ( sendQueue[ i ].size() == 0 )
		//	continue;

//		reliabilityLayerMutexes[ sendQueueSystemPriority_MUTEX + i ].Lock();
		
		
		while ( sendPacketSet[ i ].size() )
		//while ( (internalPacket=sendPacketSet[i].ReadLock())!=0 )
		{
			internalPacket = sendPacketSet[ i ].pop();
//			reliabilityLayerMutexes[ sendQueueSystemPriority_MUTEX + i ].Unlock();
			
			nextPacketBitLength = GetBitStreamHeaderLength( internalPacket ) + internalPacket->dataBitLength;
			
			if ( output->GetNumberOfBitsUsed() + nextPacketBitLength > maxDataBitSize )
			{
				// This output won't fit.
//				reliabilityLayerMutexes[ sendQueueSystemPriority_MUTEX + i ].Lock();
				sendPacketSet[ i ].pushAtHead( internalPacket ); // Push this back at the head so it is the next thing to go out
//				sendPacketSet[i].CancelReadLock(internalPacket);
				break;
			}
			
			if ( internalPacket->reliability == RELIABLE || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
				isReliable = true;
			else
				isReliable = false;
				
			// Write to the output bitstream
			//   sentPackets++;
			statistics.messagesSent[ i ] ++;
			statistics.messageDataBitsSent[ i ] += internalPacket->dataBitLength;

			// printf("Writing send packet %i to bitstream.\n", internalPacket->packetNumber);

			if (internalPacket->data[0]==49)
			{
				unsigned short packetPort;
				memcpy((char*)&packetPort, internalPacket->data+1, sizeof(unsigned short));
			}
			statistics.messageTotalBitsSent[ i ] += WriteToBitStreamFromInternalPacket( output, internalPacket );

			if ( isReliable )
			{
				// Reliable packets are saved to resend later
				reliableBits += internalPacket->dataBitLength;
				internalPacket->nextActionTime = time + lostPacketResendDelay;

				// Third param is true to make a copy because this data is from a producer consumer pool and can't be stored out of order
				//InsertPacketIntoResendQueue( internalPacket, time, true, true );

				InsertPacketIntoResendQueue( internalPacket, time, false, true );
				*reliableDataSent = true;
			}
			else
			{
				// Unreliable packets are deleted
				delete [] internalPacket->data;
				internalPacketPool.ReleasePointer( internalPacket );
			}
			//sendPacketSet[i].ReadUnlock();
			
	//		reliabilityLayerMutexes[ sendQueueSystemPriority_MUTEX + i ].Lock();
		}
	//	reliabilityLayerMutexes[ sendQueueSystemPriority_MUTEX + i ].Unlock();
	}

	// Optimization - if we sent data but didn't send an acknowledgement packet previously then send them now
	if ( acknowledgementPacketsSent == false && output->GetNumberOfBitsUsed() > 0 )
	{
		if ( acknowledgementQueue.size() > 0 )
		{
			//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Lock();
			
			while ( output->GetNumberOfBitsUsed() + ACK_BIT_LENGTH < maxDataBitSize && acknowledgementQueue.size() > 0 )
			{
				internalPacket = acknowledgementQueue.pop();
#ifdef _DEBUG
				internalPacket->data=0;
				assert(internalPacket->isAcknowledgement==true);
#endif
				//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Unlock();
				
				// Write the acknowledgement to the output bitstream
				WriteToBitStreamFromInternalPacket( output, internalPacket );

#ifdef _DEBUG
				assert(internalPacket->data==0);
#endif
				
				// Delete the acknowledgement
				internalPacketPool.ReleasePointer( internalPacket );
				
				//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Lock();
			}
			
			//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Unlock();
		}
	}
	
END_OF_GENERATE_FRAME:
	;
	
	// if (output->GetNumberOfBitsUsed()>0)
	// {
	// Update the throttle with the header
	//  bytesSent+=output->GetNumberOfBytesUsed() + UDP_HEADER_SIZE;
	//}
}

//-------------------------------------------------------------------------------------------------------
// Are we waiting for any data to be sent out or be processed by the player?
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsDataWaiting(void)
{
	unsigned i;
	for ( i = 0; i < NUMBER_OF_PRIORITIES; i++ )
	{
		if (sendPacketSet[ i ].size() > 0)
			return true;
	}

	return acknowledgementQueue.size() > 0 || resendQueue.size() > 0 || outputQueue.size() > 0 || orderingList.size() > 0 || splitPacketList.size() > 0;
}

//-------------------------------------------------------------------------------------------------------
// This will return true if we should not send at this time
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsSendThrottled( void )
{
	return ( int ) GetResendQueueDataSize() >= windowSize;
}

//-------------------------------------------------------------------------------------------------------
// We lost a packet
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::UpdatePacketloss( unsigned int time )
{
	// unsigned int time = RakNet::GetTime();
	/*
	  maximumWindowSize = (unsigned int)((double)maximumWindowSize * DECREASE_THROUGHPUT_DELTA);
	  if (maximumWindowSize < MINIMUM_THROUGHPUT)
	  {
	  maximumWindowSize = MINIMUM_THROUGHPUT;
	  }
	*/
	
	//printf("Lost packet. resendQueue.size()=%i sendQueue[0].size() = %i\n",resendQueue.size(), sendQueue[0].size());
	
	// reliabilityLayerMutexes[windowSize_MUTEX].Lock();
	
	
	// reliabilityLayerMutexes[windowSize_MUTEX].Unlock();
	// retransmittedFrames++;
	
	// The window size will decrease everytime we have to retransmit a frame
	//reliabilityLayerMutexes[windowSize_MUTEX].Lock();
	
	if ( --windowSize < MINIMUM_WINDOW_SIZE )
		windowSize = MINIMUM_WINDOW_SIZE;
		
	//reliabilityLayerMutexes[windowSize_MUTEX].Unlock();
	lossyWindowSize = windowSize;
	
	lastWindowIncreaseSizeTime = time; // This will block the window size from increasing immediately
	
	// SHOW - windowing
	//if (resendQueue.size()>0)
	  //printf("Frame lost.  New window size = %i.  Lossy window size = %i. Time=%i. Next send time=%i\n", windowSize, lossyWindowSize, RakNet::GetTime(),resendQueue.peek()->nextActionTime);
}

//-------------------------------------------------------------------------------------------------------
// Does what the function name says
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::RemovePacketFromResendQueueAndDeleteOlderReliableSequenced( PacketNumberType packetNumber )
{
	InternalPacket * internalPacket;
	PacketReliability reliability; // What type of reliability algorithm to use with this packet
	unsigned char orderingChannel; // What ordering channel this packet is on, if the reliability type uses ordering channels
	OrderingIndexType orderingIndex; // The ID used as identification for ordering channels
	
	// reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
	
	for ( unsigned i = 0; i < resendQueue.size(); i ++ )
	{
		if ( resendQueue[i] && packetNumber == resendQueue[i]->packetNumber )
		{
			// Found what we wanted to ack
			statistics.acknowlegementsReceived++;
			
			if ( i == 0 )
				internalPacket = resendQueue.pop();
			else
			{
			
				// Generate a hole
				internalPacket = resendQueue[ i ];
				// testing
				// printf("Removing packet %i from resend\n", internalPacket->packetNumber);
				resendQueue[ i ] = 0;
			}
			
			//reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
			
			// Save some of the data of the packet
			reliability = internalPacket->reliability;
			orderingChannel = internalPacket->orderingChannel;
			orderingIndex = internalPacket->orderingIndex;
			
			// Delete the packet
			//printf("Deleting %i\n", internalPacket->data);
			delete [] internalPacket->data;
			internalPacketPool.ReleasePointer( internalPacket );
			
			// If the deleted packet was reliable sequenced, also delete all older reliable sequenced resends on the same ordering channel.
			// This is because we no longer need to send these.
			if ( reliability == RELIABLE_SEQUENCED )
			{
				unsigned j = 0;
				
				//reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
				
				while ( j < resendQueue.size() )
				{
					internalPacket = resendQueue[ j ];
					
					if ( internalPacket && internalPacket->reliability == RELIABLE_SEQUENCED && internalPacket->orderingChannel == orderingChannel && IsOlderOrderedPacket( internalPacket->orderingIndex, orderingIndex ) )
					{
						// Delete the packet
						delete [] internalPacket->data;
						internalPacketPool.ReleasePointer( internalPacket );
						resendQueue[ j ] = 0; // Generate a hole
					}
					
					j++;
				}
				
				//    reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
			}
			
			//reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
			return ;
		}
	}
	
	//reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
	
	// Didn't find what we wanted to ack
	statistics.duplicateAcknowlegementsReceived++;
}

//-------------------------------------------------------------------------------------------------------
// Acknowledge receipt of the packet with the specified packetNumber
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SendAcknowledgementPacket( PacketNumberType packetNumber, unsigned int time )
{
	InternalPacket * internalPacket;
	
	// Disabled - never gets called anyway so just wastes CPU cycles
	/*
	// High load optimization - if there are over 100 acks waiting scan the list to make sure what we are adding isn't already scheduled to go out
	reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Lock();
	size = acknowledgementQueue.size();
	if (size>100)
	{
	for (i=0; i < size; i++)
	{
	internalPacket=acknowledgementQueue[i];
	if (internalPacket && internalPacket->packetNumber==packetNumber)
	{
	reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Unlock();
	//printf("Eliminating duplicate ack. acknowledgementQueue.size()=%i\n",acknowledgementQueue.size());
	return; // No need to add it - it is already here
	}
	}
	}
	reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Unlock();
	*/
	
	internalPacket = internalPacketPool.GetPointer();
#ifdef _DEBUG
	// Remove boundschecker accessing undefined memory error
	memset( internalPacket, 255, sizeof( InternalPacket ) );
#endif
	
	internalPacket->packetNumber = packetNumber;
	internalPacket->isAcknowledgement = true;
	
	internalPacket->creationTime = time;
	// We send this acknowledgement no later than 1/4 the time the remote
	//machine would send the original packet again
	// DEBUG
	internalPacket->nextActionTime = internalPacket->creationTime + ( lostPacketResendDelay >> 2 );
	//internalPacket->nextActionTime = internalPacket->creationTime;
	//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Lock();
	acknowledgementQueue.push( internalPacket );
	// printf("<Server>Adding ack at time %i. acknowledgementQueue.size=%i\n",RakNet::GetTime(), acknowledgementQueue.size());
	//reliabilityLayerMutexes[acknowledgementQueue_MUTEX].Unlock();
}

//-------------------------------------------------------------------------------------------------------
// Parse an internalPacket and figure out how many header bits would be
// written.  Returns that number
//-------------------------------------------------------------------------------------------------------
int ReliabilityLayer::GetBitStreamHeaderLength( const InternalPacket *const internalPacket )
{
#ifdef _DEBUG
	assert( internalPacket );
#endif
	
	int bitLength;
	
	if ( internalPacket->isAcknowledgement )
		return ACK_BIT_LENGTH;
		
	// Write if this packet has a security header (1 bit)
	//bitStream->Write(internalPacket->hasSecurityHeader);
	// -- bitLength+=1;
	bitLength = ACK_BIT_LENGTH;
	
	// Write the PacketReliability.  This is encoded in 3 bits
	//bitStream->WriteBits((unsigned char*)&(internalPacket->reliability), 3, true);
	bitLength += 3;
	
	// If the reliability requires an ordering channel and ordering index, we Write those.
	if ( internalPacket->reliability == UNRELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
	{
		// ordering channel encoded in 5 bits (from 0 to 31)
		//bitStream->WriteBits((unsigned char*)&(internalPacket->orderingChannel), 5, true);
		bitLength+=5;
		
		// ordering index is one byte
		//bitStream->WriteCompressed(internalPacket->orderingIndex);
		bitLength+=sizeof(OrderingIndexType)*8;
	}
	
	// Write if this is a split packet (1 bit)
	bool isSplitPacket = internalPacket->splitPacketCount > 0;
	
	//bitStream->Write(isSplitPacket);
	bitLength += 1;
	
	if ( isSplitPacket )
	{
		// split packet indices are two bytes (so one packet can be split up to 65535
		// times - maximum packet size would be about 500 * 65535)
		//bitStream->Write(internalPacket->splitPacketId);
		//bitStream->WriteCompressed(internalPacket->splitPacketIndex);
		//bitStream->WriteCompressed(internalPacket->splitPacketCount);
		bitLength += 3 * 8 * 2;
	}
	
	// Write how many bits the packet data is. Stored in an unsigned short and
	// read from 16 bits
	//bitStream->WriteBits((unsigned char*)&(internalPacket->dataBitLength), 16, true);
	
	// Read how many bits the packet data is.  Stored in 16 bits
	bitLength += 16;
	
	// Byte alignment
	//bitLength += 8 - ((bitLength -1) %8 + 1);
	
	return bitLength;
}

//-------------------------------------------------------------------------------------------------------
// Parse an internalPacket and create a bitstream to represent this data
//-------------------------------------------------------------------------------------------------------
int ReliabilityLayer::WriteToBitStreamFromInternalPacket( RakNet::BitStream *bitStream, const InternalPacket *const internalPacket )
{
#ifdef _DEBUG
	assert( bitStream && internalPacket );
#endif
	
	int start = bitStream->GetNumberOfBitsUsed();
	
	// testing
	//if (internalPacket->reliability==UNRELIABLE)
	//  printf("Sending unreliable packet %i\n", internalPacket->packetNumber);
	//else if (internalPacket->reliability==RELIABLE_SEQUENCED || internalPacket->reliability==RELIABLE_ORDERED || internalPacket->reliability==RELIABLE)
//	  printf("Sending reliable packet number %i\n", internalPacket->packetNumber);
	
	//bitStream->AlignWriteToByteBoundary();
	
	// Write the packet number (2 bytes)
	bitStream->Write( internalPacket->packetNumber );
	
	// Write if this packet is an acknowledgement (1 bit)
	bitStream->Write( internalPacket->isAcknowledgement );
	// Acknowledgement packets have no more data than the packetnumber and whether it is an acknowledgement
	
	if ( internalPacket->isAcknowledgement )
	{
		return bitStream->GetNumberOfBitsUsed() - start;
	}
	
#ifdef _DEBUG
	assert( internalPacket->dataBitLength > 0 );
#endif
	
	// Write the PacketReliability.  This is encoded in 3 bits
	unsigned char reliability = (unsigned char) internalPacket->reliability;
	
	bitStream->WriteBits( ( unsigned char* ) ( &( reliability ) ), 3, true );
	
	// If the reliability requires an ordering channel and ordering index, we Write those.
	if ( internalPacket->reliability == UNRELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
	{
		// ordering channel encoded in 5 bits (from 0 to 31)
		bitStream->WriteBits( ( unsigned char* ) & ( internalPacket->orderingChannel ), 5, true );
		
		// One or two bytes
		bitStream->Write( internalPacket->orderingIndex );
	}
	
	// Write if this is a split packet (1 bit)
	bool isSplitPacket = internalPacket->splitPacketCount > 0;
	
	bitStream->Write( isSplitPacket );
	
	if ( isSplitPacket )
	{
		// split packet indices are two bytes (so one packet can be split up to 65535 times - maximum packet size would be about 500 * 65535)
		bitStream->Write( internalPacket->splitPacketId );
		bitStream->WriteCompressed( internalPacket->splitPacketIndex );
		bitStream->WriteCompressed( internalPacket->splitPacketCount );
	}
	
	// Write how many bits the packet data is. Stored in 13 bits
#ifdef _DEBUG
	assert( BITS_TO_BYTES( internalPacket->dataBitLength ) < MAXIMUM_MTU_SIZE ); // I never send more than MTU_SIZE bytes
	
#endif
	
	unsigned short length = ( unsigned short ) internalPacket->dataBitLength; // Ignore the 2 high bytes for WriteBits
	
	bitStream->WriteCompressed( length );
	
	// Write the actual data.
	bitStream->WriteAlignedBytes( ( unsigned char* ) internalPacket->data, BITS_TO_BYTES( internalPacket->dataBitLength ) );
	
	//bitStream->WriteBits((unsigned char*)internalPacket->data, internalPacket->dataBitLength);
	
	return bitStream->GetNumberOfBitsUsed() - start;
}

//-------------------------------------------------------------------------------------------------------
// Parse a bitstream and create an internal packet to represent this data
//-------------------------------------------------------------------------------------------------------
InternalPacket* ReliabilityLayer::CreateInternalPacketFromBitStream( RakNet::BitStream *bitStream, unsigned int time )
{
	bool bitStreamSucceeded;
	InternalPacket* internalPacket;
	
	if ( bitStream->GetNumberOfUnreadBits() < sizeof( internalPacket->packetNumber ) * 8 )
		return 0; // leftover bits
		
	internalPacket = internalPacketPool.GetPointer();
	
#ifdef _DEBUG
	// Remove accessing undefined memory error
	memset( internalPacket, 255, sizeof( InternalPacket ) );
#endif
	
	internalPacket->creationTime = time;
	
	//bitStream->AlignReadToByteBoundary();
	
	// Read the packet number (2 bytes)
	bitStreamSucceeded = bitStream->Read( internalPacket->packetNumber );
	
#ifdef _DEBUG
	// 10/08/05 - Disabled assert since this hits from offline packets
	//assert( bitStreamSucceeded );
#endif
	
	if ( bitStreamSucceeded == false )
	{
		internalPacketPool.ReleasePointer( internalPacket );
		return 0;
	}
	
	// Read if this packet is an acknowledgement (1 bit)
	bitStreamSucceeded = bitStream->Read( internalPacket->isAcknowledgement );
	
#ifdef _DEBUG
	// 10/08/05 - Disabled assert since this hits from offline packets
	//assert( bitStreamSucceeded );
#endif
	
	if ( bitStreamSucceeded == false )
	{
		internalPacketPool.ReleasePointer( internalPacket );
		return 0;
	}
	
	// Acknowledgement packets have no more data than the packetnumber and whether it is an acknowledgement
	if ( internalPacket->isAcknowledgement )
		return internalPacket;
		
	// Read the PacketReliability. This is encoded in 3 bits
	unsigned char reliability;
	
	bitStreamSucceeded = bitStream->ReadBits( ( unsigned char* ) ( &( reliability ) ), 3 );
	
	internalPacket->reliability = ( PacketReliability ) reliability;
	
#ifdef _DEBUG
	// 10/08/05 - Disabled assert since this hits from offline packets
	// assert( bitStreamSucceeded );
#endif
	
	if ( bitStreamSucceeded == false )
	{
		internalPacketPool.ReleasePointer( internalPacket );
		return 0;
	}
	
	// If the reliability requires an ordering channel and ordering index, we read those.
	if ( internalPacket->reliability == UNRELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
	{
		// ordering channel encoded in 5 bits (from 0 to 31)
		bitStreamSucceeded = bitStream->ReadBits( ( unsigned char* ) & ( internalPacket->orderingChannel ), 5 );
#ifdef _DEBUG
		// 10/08/05 - Disabled assert since this hits from offline packets
		//assert( bitStreamSucceeded );
#endif
		
		if ( bitStreamSucceeded == false )
		{
			internalPacketPool.ReleasePointer( internalPacket );
			return 0;
		}
		
		bitStreamSucceeded = bitStream->Read( internalPacket->orderingIndex );
		
#ifdef _DEBUG
		// 10/08/05 - Disabled assert since this hits from offline packets
		//assert( bitStreamSucceeded );		
#endif
		
		if ( bitStreamSucceeded == false )
		{
			internalPacketPool.ReleasePointer( internalPacket );
			return 0;
		}
	}
	
	// Read if this is a split packet (1 bit)
	bool isSplitPacket;
	
	bitStreamSucceeded = bitStream->Read( isSplitPacket );
	
#ifdef _DEBUG
	// 10/08/05 - Disabled assert since this hits from offline packets
	//assert( bitStreamSucceeded );
#endif
	
	if ( bitStreamSucceeded == false )
	{
		internalPacketPool.ReleasePointer( internalPacket );
		return 0;
	}
	
	if ( isSplitPacket )
	{
		// split packet indices are one byte (so one packet can be split up to 65535 times - maximum packet size would be about 500 * 65535)
		bitStreamSucceeded = bitStream->Read( internalPacket->splitPacketId );
#ifdef _DEBUG
		// 10/08/05 - Disabled assert since this hits from offline packets
		// assert( bitStreamSucceeded );
#endif
		
		if ( bitStreamSucceeded == false )
		{
			internalPacketPool.ReleasePointer( internalPacket );
			return 0;
		}
		
		bitStreamSucceeded = bitStream->ReadCompressed( internalPacket->splitPacketIndex );
#ifdef _DEBUG
		// 10/08/05 - Disabled assert since this hits from offline packets
		//assert( bitStreamSucceeded );
#endif
		
		if ( bitStreamSucceeded == false )
		{
			internalPacketPool.ReleasePointer( internalPacket );
			return 0;
		}
		
		bitStreamSucceeded = bitStream->ReadCompressed( internalPacket->splitPacketCount );
#ifdef _DEBUG
		// 10/08/05 - Disabled assert since this hits from offline packets
		//assert( bitStreamSucceeded );
#endif
		
		if ( bitStreamSucceeded == false )
		{
			internalPacketPool.ReleasePointer( internalPacket );
			return 0;
		}
	}
	
	else
		internalPacket->splitPacketIndex = internalPacket->splitPacketCount = 0;
		
	// Optimization - do byte alignment here
	//unsigned char zero;
	//bitStream->ReadBits(&zero, 8 - (bitStream->GetNumberOfBitsUsed() %8));
	//assert(zero==0);
	
	
	unsigned short length;
	
	bitStreamSucceeded = bitStream->ReadCompressed( length );
	
	// Read into an unsigned short.  Otherwise the data would be offset too high by two bytes
#ifdef _DEBUG
	// 10/08/05 - Disabled assert since this hits from offline packets
	//assert( bitStreamSucceeded );
#endif
	
	if ( bitStreamSucceeded == false )
	{
		internalPacketPool.ReleasePointer( internalPacket );
		return 0;
	}
	
	internalPacket->dataBitLength = length;
#ifdef _DEBUG
// 10/08/05 - Disabled assert since this hits from offline packets arriving when the sender does not know we just connected, which is an unavoidable condition sometimes
//	assert( internalPacket->dataBitLength > 0 && BITS_TO_BYTES( internalPacket->dataBitLength ) < MAXIMUM_MTU_SIZE );
#endif
	if ( ! ( internalPacket->dataBitLength > 0 && BITS_TO_BYTES( internalPacket->dataBitLength ) < MAXIMUM_MTU_SIZE ) )
	{
		// 10/08/05 - internalPacket->data wasn't allocated yet
	//	delete [] internalPacket->data;
		internalPacketPool.ReleasePointer( internalPacket );
		return 0;
	}
	
	// Allocate memory to hold our data
	internalPacket->data = new char [ BITS_TO_BYTES( internalPacket->dataBitLength ) ];
	//printf("Allocating %i\n",  internalPacket->data);
	
	// Set the last byte to 0 so if ReadBits does not read a multiple of 8 the last bits are 0'ed out
	internalPacket->data[ BITS_TO_BYTES( internalPacket->dataBitLength ) - 1 ] = 0;
	
	// Read the data the packet holds
	bitStreamSucceeded = bitStream->ReadAlignedBytes( ( unsigned char* ) internalPacket->data, BITS_TO_BYTES( internalPacket->dataBitLength ) );

	//bitStreamSucceeded = bitStream->ReadBits((unsigned char*)internalPacket->data, internalPacket->dataBitLength);
#ifdef _DEBUG
	
	// 10/08/05 - Disabled assert since this hits from offline packets
	//assert( bitStreamSucceeded );
	
	if ( bitStreamSucceeded == false )
	{
		delete [] internalPacket->data;
		internalPacketPool.ReleasePointer( internalPacket );
		return 0;
	}
	
#endif
	
	// PRINTING UNRELIABLE STRINGS
	// if (internalPacket->data && internalPacket->dataBitLength>5*8)
	//  printf("Received %s\n",internalPacket->data);
	
	return internalPacket;
}

//-------------------------------------------------------------------------------------------------------
// Get the SHA1 code
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::GetSHA1( unsigned char * const buffer, unsigned int
				nbytes, char code[ SHA1_LENGTH ] )
{
	CSHA1 sha1;
	
	sha1.Reset();
	sha1.Update( ( unsigned char* ) buffer, nbytes );
	sha1.Final();
	memcpy( code, sha1.GetHash(), SHA1_LENGTH );
}

//-------------------------------------------------------------------------------------------------------
// Check the SHA1 code
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::CheckSHA1( char code[ SHA1_LENGTH ], unsigned char *
				  const buffer, unsigned int nbytes )
{
	char code2[ SHA1_LENGTH ];
	GetSHA1( buffer, nbytes, code2 );
	
	for ( int i = 0; i < SHA1_LENGTH; i++ )
		if ( code[ i ] != code2[ i ] )
			return false;
			
	return true;
}

//-------------------------------------------------------------------------------------------------------
// Search the specified list for sequenced packets on the specified ordering
// stream, optionally skipping those with splitPacketId, and delete them
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::DeleteSequencedPacketsInList( unsigned char orderingChannel, BasicDataStructures::List<InternalPacket*>&theList, int splitPacketId )
{
	unsigned i = 0;
	
	while ( i < theList.size() )
	{
		if ( ( theList[ i ]->reliability == RELIABLE_SEQUENCED || theList[ i ]->reliability == UNRELIABLE_SEQUENCED ) &&
			theList[ i ]->orderingChannel == orderingChannel && ( splitPacketId == -1 || theList[ i ]->splitPacketId != (unsigned int) splitPacketId ) )
		{
			InternalPacket * internalPacket = theList[ i ];
			theList.del( i );
			delete [] internalPacket->data;
			internalPacketPool.ReleasePointer( internalPacket );
		}
		
		else
			i++;
	}
}

//-------------------------------------------------------------------------------------------------------
// Search the specified list for sequenced packets with a value less than orderingIndex and delete them
// Note - I added functionality so you can use the Queue as a list (in this case for searching) but it is less efficient to do so than a regular list
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::DeleteSequencedPacketsInList( unsigned char orderingChannel, BasicDataStructures::Queue<InternalPacket*>&theList )
{
	InternalPacket * internalPacket;
	int listSize = theList.size();
	int i = 0;
	
	while ( i < listSize )
	{
		if ( ( theList[ i ]->reliability == RELIABLE_SEQUENCED || theList[ i ]->reliability == UNRELIABLE_SEQUENCED ) && theList[ i ]->orderingChannel == orderingChannel )
		{
			internalPacket = theList[ i ];
			theList.del( i );
			delete [] internalPacket->data;
			internalPacketPool.ReleasePointer( internalPacket );
			listSize--;
		}
		
		else
			i++;
	}
}

//-------------------------------------------------------------------------------------------------------
// Returns true if newPacketOrderingIndex is older than the waitingForPacketOrderingIndex
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsOlderOrderedPacket( OrderingIndexType newPacketOrderingIndex, OrderingIndexType waitingForPacketOrderingIndex )
{
	// This should give me 255 or 65535
	OrderingIndexType maxRange = (OrderingIndexType) -1;

	if ( waitingForPacketOrderingIndex > maxRange/2 )
	{
		if ( newPacketOrderingIndex >= waitingForPacketOrderingIndex - maxRange/2+1 && newPacketOrderingIndex < waitingForPacketOrderingIndex )
		{
			return true;
		}
	}
	
	else
		if ( newPacketOrderingIndex >= ( OrderingIndexType ) ( waitingForPacketOrderingIndex - (( OrderingIndexType ) maxRange/2+1) ) ||
			newPacketOrderingIndex < waitingForPacketOrderingIndex )
		{
			return true;
		}
		
	// Old packet
	return false;
}

//-------------------------------------------------------------------------------------------------------
// Split the passed packet into chunks under MTU_SIZEbytes (including headers) and save those new chunks
// Optimized version
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SplitPacket( InternalPacket *internalPacket, int MTUSize )
{
	// Doing all sizes in bytes in this function so I don't write partial bytes with split packets
	internalPacket->splitPacketCount = 1; // This causes GetBitStreamHeaderLength to account for the split packet header
	int headerLength = BITS_TO_BYTES( GetBitStreamHeaderLength( internalPacket ) );
	int dataByteLength = BITS_TO_BYTES( internalPacket->dataBitLength );
	int maxDataSize;
	int maximumSendBlock, byteOffset, bytesToSend;
	unsigned short splitPacketIndex;
	int i;
	InternalPacket **internalPacketArray;
	
	maxDataSize = MTUSize - UDP_HEADER_SIZE;
	
	if ( encryptor.IsKeySet() )
		maxDataSize -= 16; // Extra data for the encryptor
		
#ifdef _DEBUG
	// Make sure we need to split the packet to begin with
	assert( dataByteLength > maxDataSize - headerLength );
	
	// If this assert is hit the packet is so tremendous we need to widen the split packet types.  You should never send something that big anyway
	assert( ( dataByteLength - 1 ) / ( maxDataSize - headerLength ) + 1 < 65535 );
	
#endif
	
	// How much to send in the largest block
	maximumSendBlock = maxDataSize - headerLength;
	
	// Calculate how many packets we need to create
	internalPacket->splitPacketCount = ( unsigned short ) ( ( dataByteLength - 1 ) / ( maximumSendBlock ) + 1 );
	
	statistics.totalSplits += internalPacket->splitPacketCount;
	
	// Optimization
	// internalPacketArray = new InternalPacket*[internalPacket->splitPacketCount];
	internalPacketArray = ( InternalPacket** ) alloca( sizeof( InternalPacket* ) * internalPacket->splitPacketCount );
	
	for ( i = 0; i < ( int ) internalPacket->splitPacketCount; i++ )
	{
		internalPacketArray[ i ] = internalPacketPool.GetPointer();
		//internalPacketArray[ i ] = (InternalPacket*) alloca( sizeof( InternalPacket ) );
//		internalPacketArray[ i ] = sendPacketSet[internalPacket->priority].WriteLock();
		memcpy( internalPacketArray[ i ], internalPacket, sizeof( InternalPacket ) );
	}
	
	// This identifies which packet this is in the set
	splitPacketIndex = 0;
	
	// Do a loop to send out all the packets
	do
	{
		byteOffset = splitPacketIndex * maximumSendBlock;
		bytesToSend = dataByteLength - byteOffset;
		
		if ( bytesToSend > maximumSendBlock )
			bytesToSend = maximumSendBlock;
			
		// Copy over our chunk of data
		internalPacketArray[ splitPacketIndex ]->data = new char[ bytesToSend ];
		
		memcpy( internalPacketArray[ splitPacketIndex ]->data, internalPacket->data + byteOffset, bytesToSend );
		
		if ( bytesToSend != maximumSendBlock )
			internalPacketArray[ splitPacketIndex ]->dataBitLength = internalPacket->dataBitLength - splitPacketIndex * ( maximumSendBlock << 3 );
		else
			internalPacketArray[ splitPacketIndex ]->dataBitLength = bytesToSend << 3;
			
		internalPacketArray[ splitPacketIndex ]->splitPacketIndex = splitPacketIndex;
		internalPacketArray[ splitPacketIndex ]->splitPacketId = splitPacketId;
		internalPacketArray[ splitPacketIndex ]->splitPacketCount = internalPacket->splitPacketCount;
		
		if ( splitPacketIndex > 0 )   // For the first split packet index we keep the packetNumber already assigned
		{
			// For every further packet we use a new packetNumber.
			// Note that all split packets are reliable
			//reliabilityLayerMutexes[ packetNumber_MUTEX ].Lock();
			internalPacketArray[ splitPacketIndex ]->packetNumber = packetNumber;
			
			//if ( ++packetNumber == RECEIVED_PACKET_LOG_LENGTH )
			//	packetNumber = 0;
			++packetNumber;
				
			//reliabilityLayerMutexes[ packetNumber_MUTEX ].Unlock();
		}
		
		// Add the new packet to send list at the correct priority
		//  reliabilityLayerMutexes[sendQueue_MUTEX].Lock();
		//  sendQueue[internalPacket->priority].insert(newInternalPacket);
		//  reliabilityLayerMutexes[sendQueue_MUTEX].Unlock();
		// SHOW SPLIT PACKET GENERATION
		// if (splitPacketIndex % 100 == 0)
		//  printf("splitPacketIndex=%i\n",splitPacketIndex);
		//} while(++splitPacketIndex < internalPacket->splitPacketCount);
	}
	
	while ( ++splitPacketIndex < internalPacket->splitPacketCount );
	
	splitPacketId++; // It's ok if this wraps to 0

//	InternalPacket *workingPacket;
	
	// Copy all the new packets into the split packet list
//	reliabilityLayerMutexes[ sendQueueSystemPriority_MUTEX + internalPacket->priority ].Lock();
	for ( i = 0; i < ( int ) internalPacket->splitPacketCount; i++ )
	{
		sendPacketSet[ internalPacket->priority ].push( internalPacketArray[ i ] );
//		workingPacket=sendPacketSet[internalPacket->priority].WriteLock();
//		memcpy(workingPacket, internalPacketArray[ i ], sizeof(InternalPacket));
//		sendPacketSet[internalPacket->priority].WriteUnlock();
	}
//	reliabilityLayerMutexes[ sendQueueSystemPriority_MUTEX + internalPacket->priority ].Unlock();
	
	// Delete the original
	delete [] internalPacket->data;
	internalPacketPool.ReleasePointer( internalPacket );
	
	//delete [] internalPacketArray;
}

//-------------------------------------------------------------------------------------------------------
// Insert a packet into the split packet list
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::InsertIntoSplitPacketList( InternalPacket * internalPacket )
{
	//reliabilityLayerMutexes[splitPacketList_MUTEX].Lock();
	splitPacketList.insert( internalPacket );
	//reliabilityLayerMutexes[splitPacketList_MUTEX].Unlock();
}

//-------------------------------------------------------------------------------------------------------
// Take all split chunks with the specified splitPacketId and try to
//reconstruct a packet.  If we can, allocate and return it.  Otherwise return 0
// Optimized version
//-------------------------------------------------------------------------------------------------------
InternalPacket * ReliabilityLayer::BuildPacketFromSplitPacketList( unsigned int splitPacketId, unsigned int time )
{
	int i, j, size;
	// How much data all blocks but the last hold
	int maxDataSize;
	int numParts;
	int bitlength;
	int *indexList;
	int indexListIndex;
	
	//reliabilityLayerMutexes[splitPacketList_MUTEX].Lock();
	size = splitPacketList.size();
	
	for ( i = 0; i < size; i++ )
	{
		if ( splitPacketList[ i ]->splitPacketId == splitPacketId )
		{
			// Is there enough elements in the list to have all the parts?
			
			if ( splitPacketList[ i ]->splitPacketCount > splitPacketList.size() - i )
			{
				//   if (splitPacketList.size() % 100 == 0 || splitPacketList[i]->splitPacketCount-splitPacketList.size()<100)
				//    printf("%i out of %i\n", splitPacketList.size(), splitPacketList[i]->splitPacketCount);
				//reliabilityLayerMutexes[splitPacketList_MUTEX].Unlock();
				return 0;
			}
			
			//  printf("%i out of %i\n", splitPacketList.size(), splitPacketList[i]->splitPacketCount);
			// Keep track of the indices of the elements through our first scan so we don't have to rescan to find them
			indexListIndex = 0;
			
			numParts = 1;
			
			bitlength = splitPacketList[ i ]->dataBitLength;
			
			// indexList = new int[splitPacketList[i]->splitPacketCount];
			indexList = ( int* ) alloca( sizeof( int ) * splitPacketList[ i ]->splitPacketCount );
			
			indexList[ indexListIndex++ ] = i;
			
			maxDataSize = BITS_TO_BYTES( splitPacketList[ i ]->dataBitLength );
			
			// Are all the parts there?
			for ( j = i + 1; j < size; j++ )
			{
				if ( splitPacketList[ j ]->splitPacketId == splitPacketId )
				{
					indexList[ indexListIndex++ ] = j;
					numParts++;
					bitlength += splitPacketList[ j ]->dataBitLength;

					// Verify that we are dealing with the same splitPacketId
#ifdef _DEBUG
					assert(splitPacketList[ j ]->splitPacketCount==splitPacketList[ i ]->splitPacketCount);
#endif
					
					if ( ( int ) BITS_TO_BYTES( splitPacketList[ j ]->dataBitLength ) > maxDataSize )
						maxDataSize = BITS_TO_BYTES( splitPacketList[ j ]->dataBitLength );
				}
			}
			
			if ( (unsigned int)numParts == splitPacketList[ i ]->splitPacketCount )
			{
				int allocatedLength;
				// All the parts are here
				InternalPacket * internalPacket = CreateInternalPacketCopy( splitPacketList[ i ], 0, 0, time );
				allocatedLength=BITS_TO_BYTES( bitlength );
				internalPacket->data = new char[ allocatedLength ];
#ifdef _DEBUG
				internalPacket->splitPacketCount = splitPacketList[ i ]->splitPacketCount;
#endif
				
				// Add each part to internalPacket
				j = 0;
				
				while ( j < indexListIndex )
				{
					if ( splitPacketList[ indexList[ j ] ]->splitPacketCount-1 == splitPacketList[ indexList[ j ] ]->splitPacketIndex )
					{
						// Last split packet
						// If this assert fails,
						// then the total bit length calculated by adding the last block to the maximum block size * the number of blocks that are not the last block
						// doesn't match the amount calculated from traversing the list
#ifdef _DEBUG
						assert( BITS_TO_BYTES( splitPacketList[ indexList[ j ] ]->dataBitLength ) + splitPacketList[ indexList[ j ] ]->splitPacketIndex * (unsigned)maxDataSize == ( (unsigned)bitlength - 1 ) / 8 + 1 );
#endif

						if (splitPacketList[ indexList[ j ] ]->splitPacketIndex * (unsigned int) maxDataSize + (unsigned int) BITS_TO_BYTES( splitPacketList[ indexList[ j ] ]->dataBitLength ) > (unsigned int) allocatedLength )
						{
							// Watch for buffer overruns
#ifdef _DEBUG
							assert(0);
#endif
							delete internalPacket->data;
							internalPacketPool.ReleasePointer(internalPacket);
							return 0;
						}

						memcpy( internalPacket->data + splitPacketList[ indexList[ j ] ]->splitPacketIndex * maxDataSize, splitPacketList[ indexList[ j ] ]->data, BITS_TO_BYTES( splitPacketList[ indexList[ j ] ]->dataBitLength ) );
					}
					
					else
					{
                        if (splitPacketList[ indexList[ j ] ]->splitPacketIndex * (unsigned int) maxDataSize + (unsigned int) maxDataSize > (unsigned int) allocatedLength )
						{
							// Watch for buffer overruns
#ifdef _DEBUG
							assert(0);
#endif
							delete internalPacket->data;
							internalPacketPool.ReleasePointer(internalPacket);
							return 0;
						}
						// Not last split packet
						memcpy( internalPacket->data + splitPacketList[ indexList[ j ] ]->splitPacketIndex * maxDataSize, splitPacketList[ indexList[ j ] ]->data, maxDataSize );
					}
					
					internalPacket->dataBitLength += splitPacketList[ indexList[ j ] ]->dataBitLength;
					InternalPacket *temp;
					
					temp = splitPacketList[ indexList[ j ] ];
					delete [] temp->data;
					internalPacketPool.ReleasePointer( temp );
					splitPacketList[ indexList[ j ] ] = 0;
					
#ifdef _DEBUG
					
					numParts--;
#endif
					
					j++;
					size--;
				}
				
#ifdef _DEBUG
				assert( numParts == 0 ); // Make sure the correct # of elements was removed from the list
#endif
				
				j = 0;
				
				while ( ( unsigned ) j < splitPacketList.size() )
					if ( splitPacketList[ j ] == 0 )
					{
						// Since order doesn't matter, swap from the tail to the current element.
						splitPacketList[ j ] = splitPacketList[ splitPacketList.size() - 1 ];
						splitPacketList[ splitPacketList.size() - 1 ] = 0;
						// Then just delete the tail (just changes a counter)
						splitPacketList.del( splitPacketList.size() - 1 );
					}
					
					else
						j++;
						
				//reliabilityLayerMutexes[splitPacketList_MUTEX].Unlock();
				
				// delete [] indexList;
				
				return internalPacket;
			}
			
			// delete [] indexList;
			break;
		}
	}
	
	//reliabilityLayerMutexes[splitPacketList_MUTEX].Unlock();
	
	return 0;
}

// Delete any unreliable split packets that have long since expired
void ReliabilityLayer::DeleteOldUnreliableSplitPackets( unsigned int time )
{
	unsigned size, i, orderingIndexToDelete;
	unsigned int newestUnreliableSplitPacket;
	bool found;
	InternalPacket *temp;
	
	// Scan through the list for split packets that were sent unreliably.
	// If the newest unreliable split packet for a particular ID is more than 3000 ms old, then
	// delete all of them of that id
	
	size = splitPacketList.size();
	newestUnreliableSplitPacket = 0;
	found = false;
	
	for ( i = 0; i < size; i++ )
	{
		if ( ( splitPacketList[ i ]->reliability == UNRELIABLE || splitPacketList[ i ]->reliability == UNRELIABLE_SEQUENCED ) &&
			splitPacketList[ i ]->creationTime >= newestUnreliableSplitPacket )
		{
			orderingIndexToDelete = splitPacketList[ i ]->orderingIndex;
			newestUnreliableSplitPacket = splitPacketList[ i ]->creationTime;
			found = true;
		}
	}
	
	if ( found && time>newestUnreliableSplitPacket && time-newestUnreliableSplitPacket > 5000 )
	{
		// Delete all split packets that use orderingIndexToDelete
		i = 0;
		
		while ( i < splitPacketList.size() )
		{
#pragma warning( disable : 4701 ) //  warning C4701: local variable 'orderingIndexToDelete' may be used without having been initialized
			if ( splitPacketList[ i ]->orderingIndex == orderingIndexToDelete )
			{
				temp = splitPacketList[ i ];
				splitPacketList[ i ] = splitPacketList[ splitPacketList.size() - 1 ];
				splitPacketList.del(); // Removes the last element
				delete [] temp->data;
				internalPacketPool.ReleasePointer( temp );
			}
			
			else
				i++;
		}
	}
}

//-------------------------------------------------------------------------------------------------------
// Creates a copy of the specified internal packet with data copied from the original starting at dataByteOffset for dataByteLength bytes.
// Does not copy any split data parameters as that information is always generated does not have any reason to be copied
//-------------------------------------------------------------------------------------------------------
InternalPacket * ReliabilityLayer::CreateInternalPacketCopy( InternalPacket *original, int dataByteOffset, int dataByteLength, unsigned int time )
{
	InternalPacket * copy = internalPacketPool.GetPointer();
#ifdef _DEBUG
	// Remove boundschecker accessing undefined memory error
	memset( copy, 255, sizeof( InternalPacket ) );
#endif
	// Copy over our chunk of data
	
	if ( dataByteLength > 0 )
	{
		copy->data = new char[ dataByteLength ];
		memcpy( copy->data, original->data + dataByteOffset, dataByteLength );
	}
	else
		copy->data = 0;
		
	copy->dataBitLength = dataByteLength << 3;
	copy->creationTime = time;
	copy->isAcknowledgement = original->isAcknowledgement;
	copy->nextActionTime = 0;
	copy->orderingIndex = original->orderingIndex;
	copy->orderingChannel = original->orderingChannel;
	copy->packetNumber = original->packetNumber;
	copy->priority = original->priority;
	copy->reliability = original->reliability;
	
	return copy;
}

//-------------------------------------------------------------------------------------------------------
// Get the specified ordering list
// LOCK THIS WHOLE BLOCK WITH reliabilityLayerMutexes[orderingList_MUTEX].Unlock();
//-------------------------------------------------------------------------------------------------------
BasicDataStructures::LinkedList<InternalPacket*> *ReliabilityLayer::GetOrderingListAtOrderingStream( unsigned char orderingChannel )
{
	if ( orderingChannel >= orderingList.size() )
		return 0;
		
	return orderingList[ orderingChannel ];
}

//-------------------------------------------------------------------------------------------------------
// Add the internal packet to the ordering list in order based on order index
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::AddToOrderingList( InternalPacket * internalPacket )
{
#ifdef _DEBUG
	assert( internalPacket->orderingChannel < NUMBER_OF_ORDERED_STREAMS );
#endif
	
	if ( internalPacket->orderingChannel >= NUMBER_OF_ORDERED_STREAMS )
	{
		return;
	}

	BasicDataStructures::LinkedList<InternalPacket*> *theList;
		
	if ( internalPacket->orderingChannel >= orderingList.size() || orderingList[ internalPacket->orderingChannel ] == 0 )
	{
		// Need a linked list in this index
		orderingList.replace( new BasicDataStructures::LinkedList<InternalPacket*>, 0, internalPacket->orderingChannel );
		theList=orderingList[ internalPacket->orderingChannel ];
	}
	else
	{
		// Have a linked list in this index
		if ( orderingList[ internalPacket->orderingChannel ]->size() == 0 )
		{
			theList=orderingList[ internalPacket->orderingChannel ];
		}
		else
		{
			theList = GetOrderingListAtOrderingStream( internalPacket->orderingChannel );
		}
	}

	theList->end();
	theList->add(internalPacket);
}

//-------------------------------------------------------------------------------------------------------
// Inserts a packet into the resend list in order
// THIS WHOLE FUNCTION SHOULD BE LOCKED WITH
// reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::InsertPacketIntoResendQueue( InternalPacket *internalPacket, unsigned int time, bool makeCopyOfInternalPacket, bool resetAckTimer )
{
	//reliabilityLayerMutexes[lastAckTime_MUTEX].Lock();
	if ( lastAckTime == 0 || resetAckTimer )
		lastAckTime = time; // Start the timer for the ack of this packet if we aren't already waiting for an ack
		
	//reliabilityLayerMutexes[lastAckTime_MUTEX].Unlock();
	
	//reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
	if (makeCopyOfInternalPacket)
	{
		InternalPacket *pool=internalPacketPool.GetPointer();
		//printf("Adding %i\n", internalPacket->data);
		memcpy(pool, internalPacket, sizeof(InternalPacket));
		resendQueue.push( pool );
	}
	else
	{
		resendQueue.push( internalPacket );
	}	
	
	//reliabilityLayerMutexes[resendQueue_MUTEX].Unlock();
}

//-------------------------------------------------------------------------------------------------------
// If Read returns -1 and this returns true then a modified packet was detected
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsCheater( void ) const
{
	return cheater;
}

//-------------------------------------------------------------------------------------------------------
//  Were you ever unable to deliver a packet despite retries?
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsDeadConnection( void ) const
{
	return deadConnection;
}

//-------------------------------------------------------------------------------------------------------
//  Causes IsDeadConnection to return true
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::KillConnection( void )
{
	deadConnection=true;
}

//-------------------------------------------------------------------------------------------------------
// How long to wait between packet resends
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SetLostPacketResendDelay( unsigned int i )
{
	if ( i > 0 )
		lostPacketResendDelay = i;
		
	if ( lostPacketResendDelay < 150 )   // To avoid unnecessary packetloss, this value should be UPDATE_THREAD_UPDATE_TIME + UPDATE_THREAD_POLL_TIME at a minimum
		lostPacketResendDelay = 150;
}

//-------------------------------------------------------------------------------------------------------
// Statistics
//-------------------------------------------------------------------------------------------------------
RakNetStatisticsStruct * const ReliabilityLayer::GetStatistics( void )
{
	int i;
	
	for ( i = 0; i < NUMBER_OF_PRIORITIES; i++ )
	{
		statistics.messageSendBuffer[i] = sendPacketSet[i].size();
	//	statistics.messageSendBuffer[i] = sendPacketSet[i].Size();
	}
	
	statistics.acknowlegementsPending = acknowledgementQueue.size();
	statistics.messagesWaitingForReassembly = splitPacketList.size();
	statistics.internalOutputQueueSize = outputQueue.size();
	statistics.windowSize = windowSize;
	statistics.lossySize = lossyWindowSize == MAXIMUM_WINDOW_SIZE + 1 ? 0 : lossyWindowSize;
	statistics.messagesOnResendQueue = GetResendQueueDataSize();
	
	return &statistics;
}

//-------------------------------------------------------------------------------------------------------
// Decodes the time given and returns if that time should be removed from the recieved packets list
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsExpiredTime(unsigned int input, unsigned int currentTime) const
{
	// A time in the future is just a flag that this was a packet we never got (a hole).  We still expire these
	// after shifting the value to normal time
	if (IsReceivedPacketHole(input, currentTime)) 
		input -= TIMEOUT_TIME*100;

	if (input < currentTime - TIMEOUT_TIME)
		return true;

	return false;
}

//-------------------------------------------------------------------------------------------------------
// Returns if this packet time is encoded to mean we never got this packet
//-------------------------------------------------------------------------------------------------------
unsigned int ReliabilityLayer::IsReceivedPacketHole(unsigned int input, unsigned int currentTime) const
{
	return input > currentTime+TIMEOUT_TIME;
}

//-------------------------------------------------------------------------------------------------------
// Gets the time used to indicate that this received packet never arrived
//-------------------------------------------------------------------------------------------------------
unsigned int ReliabilityLayer::MakeReceivedPacketHole(unsigned int input) const
{
	return input + TIMEOUT_TIME*100; // * 2 is enough but there is no reason to take that chance
}

//-------------------------------------------------------------------------------------------------------
// Returns the number of packets in the resend queue, not counting holes
//-------------------------------------------------------------------------------------------------------
unsigned int ReliabilityLayer::GetResendQueueDataSize(void) const
{
	/*
	unsigned int i, count;
	for (count=0, i=0; i < resendQueue.size(); i++)
		if (resendQueue[i]!=0)
			count++;
	return count;
	*/

	// Not accurate but thread-safe.  The commented version might crash if the queue is cleared while we loop through it
	return resendQueue.size();
}

//-------------------------------------------------------------------------------------------------------
// Process threaded commands
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::UpdateThreadedMemory(void)
{
	if ( freeThreadedMemoryOnNextUpdate )
	{
		freeThreadedMemoryOnNextUpdate = false;
		FreeThreadedMemory();
	}
}
