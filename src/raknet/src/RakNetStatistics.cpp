/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief Statistical Information Formatting Implementation
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

#include "RakNetStatistics.h"
#include <stdio.h> // sprintf
#include "BitStream.h" // BITS_TO_BYTES

// Verbosity level currently supports 0 (low), 1 (medium), 2 (high)
// Buffer must be hold enough to hold the output string.  See the source to get an idea of how many bytes will be output
void StatisticsToString( RakNetStatisticsStruct *s, char *buffer, int verbosityLevel )
{
	if ( s == 0 )
	{
		sprintf( buffer, "stats is a NULL pointer in statsToString\n" );
		return ;
	}

	if ( verbosityLevel == 0 )
	{
		// Verbosity level 0
		sprintf( buffer,
			"Total bytes sent: %u\n"
			"Total bytes received: %u\n"
			"Packetloss: %.1f%%\n",
			BITS_TO_BYTES( s->totalBitsSent ),
			BITS_TO_BYTES( s->bitsReceived + s->bitsWithBadCRCReceived ),
			100.0f * ( float ) s->messagesTotalBitsResent / ( float ) s->totalBitsSent );
	}

	else if ( verbosityLevel == 1 )
	{
		// Verbosity level 1

		sprintf( buffer,
			"Messages in Send buffer: %u\n"
			"Messages sent: %u\n"
			"Bytes sent: %u\n"
			"Acks sent: %u\n"
			"Acks in send buffer: %u\n"
			"Messages waiting for ack: %u\n"
			"Messages resent: %u\n"
			"Bytes resent: %u\n"
			"Packetloss: %.1f%%\n"
			"Messages received: %u\n"
			"Bytes received: %u\n"
			"Acks received: %u\n"
			"Duplicate acks received: %u\n"
			"Window size: %u\n",
			s->messageSendBuffer[ SYSTEM_PRIORITY ] + s->messageSendBuffer[ HIGH_PRIORITY ] + s->messageSendBuffer[ MEDIUM_PRIORITY ] + s->messageSendBuffer[ LOW_PRIORITY ],
			s->messagesSent[ SYSTEM_PRIORITY ] + s->messagesSent[ HIGH_PRIORITY ] + s->messagesSent[ MEDIUM_PRIORITY ] + s->messagesSent[ LOW_PRIORITY ],
			BITS_TO_BYTES( s->totalBitsSent ),
			s->acknowlegementsSent,
			s->acknowlegementsPending,
			s->messagesOnResendQueue,
			s->messageResends,
			BITS_TO_BYTES( s->messagesTotalBitsResent ),
			100.0f * ( float ) s->messagesTotalBitsResent / ( float ) s->totalBitsSent,
			s->duplicateMessagesReceived + s->invalidMessagesReceived + s->messagesReceived,
			BITS_TO_BYTES( s->bitsReceived + s->bitsWithBadCRCReceived ),
			s->acknowlegementsReceived,
			s->duplicateAcknowlegementsReceived,
			s->windowSize );
	}
	else
	{
		// Verbosity level 2.
		sprintf( buffer,
			"Bytes sent:\t\t\t\t%u\n"
			"Messages in send buffer:\t\tSP:%u HP:%u MP:%u LP:%u\n"
			"Messages sent:\t\t\t\tSP:%u HP:%u MP:%u LP:%u\n"
			"Message data bytes sent:\t\tSP:%u HP:%u MP:%u LP:%u\n"
			"Message header bytes sent:\t\tSP:%u HP:%u MP:%u LP:%u\n"
			"Message total bytes sent:\t\tSP:%u HP:%u MP:%u LP:%u\n"
			"Bytes received:\t\t\t\tTtl:%u Good:%u Bad:%u\n"
			"Packets received:\t\t\tTtl:%u Good:%u Bad:%u\n"
			"Acks received:\t\t\t\tTtl:%u Good:%u Dup:%u\n"
			"Messages received:\t\t\tTotal:%u Valid:%u Invalid:%u Dup:%u\n"
			"Packetloss:\t\t\t\t%.1f%%\n"
			"Packets sent:\t\t\t\t%u\n"
			"Acks sent:\t\t\t\t%u\n"
			"Acks in send buffer:\t\t\t%u\n"
			"Messages waiting for ack:\t\t%u\n"
			"Ack bytes sent:\t\t\t\t%u\n"
			"Sent packets containing only acks:\t%u\n"
			"Sent packets w/only acks and resends:\t%u\n"
			"Reliable messages resent:\t\t%u\n"
			"Reliable message data bytes resent:\t%u\n"
			"Reliable message header bytes resent:\t%u\n"
			"Reliable message total bytes resent:\t%u\n"
			"Number of messages split:\t\t%u\n"
			"Number of messages unsplit:\t\t%u\n"
			"Message splits performed:\t\t%u\n"
			"Sequenced messages out of order:\t%u\n"
			"Sequenced messages in order:\t\t%u\n"
			"Ordered messages out of order:\t\t%u\n"
			"Ordered messages in of order:\t\t%u\n"
			"Split messages waiting for reassembly:\t%u\n"
			"Messages in internal output queue:\t%u\n"
			"Window size:\t\t\t\t%u\n"
			"Lossy window size\t\t\t%u\n"
			"Connection start time:\t\t\t%u\n",
			BITS_TO_BYTES( s->totalBitsSent ),
			s->messageSendBuffer[ SYSTEM_PRIORITY ], s->messageSendBuffer[ HIGH_PRIORITY ], s->messageSendBuffer[ MEDIUM_PRIORITY ], s->messageSendBuffer[ LOW_PRIORITY ],
			s->messagesSent[ SYSTEM_PRIORITY ], s->messagesSent[ HIGH_PRIORITY ], s->messagesSent[ MEDIUM_PRIORITY ], s->messagesSent[ LOW_PRIORITY ],
			BITS_TO_BYTES( s->messageDataBitsSent[ SYSTEM_PRIORITY ] ), BITS_TO_BYTES( s->messageDataBitsSent[ HIGH_PRIORITY ] ), BITS_TO_BYTES( s->messageDataBitsSent[ MEDIUM_PRIORITY ] ), BITS_TO_BYTES( s->messageDataBitsSent[ LOW_PRIORITY ] ),
			BITS_TO_BYTES( s->messageTotalBitsSent[ SYSTEM_PRIORITY ] - s->messageDataBitsSent[ SYSTEM_PRIORITY ] ), BITS_TO_BYTES( s->messageTotalBitsSent[ HIGH_PRIORITY ] - s->messageDataBitsSent[ HIGH_PRIORITY ] ), BITS_TO_BYTES( s->messageTotalBitsSent[ MEDIUM_PRIORITY ] - s->messageDataBitsSent[ MEDIUM_PRIORITY ] ), BITS_TO_BYTES( s->messageTotalBitsSent[ LOW_PRIORITY ] - s->messageDataBitsSent[ LOW_PRIORITY ] ),
			BITS_TO_BYTES( s->messageTotalBitsSent[ SYSTEM_PRIORITY ] ), BITS_TO_BYTES( s->messageTotalBitsSent[ HIGH_PRIORITY ] ), BITS_TO_BYTES( s->messageTotalBitsSent[ MEDIUM_PRIORITY ] ), BITS_TO_BYTES( s->messageTotalBitsSent[ LOW_PRIORITY ] ),
			BITS_TO_BYTES( s->bitsReceived + s->bitsWithBadCRCReceived ), BITS_TO_BYTES( s->bitsReceived ), BITS_TO_BYTES( s->bitsWithBadCRCReceived ),
			s->packetsReceived + s->packetsWithBadCRCReceived, s->packetsReceived, s->packetsWithBadCRCReceived,
			s->acknowlegementsReceived + s->duplicateAcknowlegementsReceived, s->acknowlegementsReceived, s->duplicateAcknowlegementsReceived,
			s->messagesReceived + s->invalidMessagesReceived + s->duplicateMessagesReceived, s->messagesReceived, s->invalidMessagesReceived, s->duplicateMessagesReceived,
			100.0f * ( float ) s->messagesTotalBitsResent / ( float ) s->totalBitsSent,
			s->packetsSent,
			s->acknowlegementsSent,
			s->acknowlegementsPending,
			s->messagesOnResendQueue,
			BITS_TO_BYTES( s->acknowlegementBitsSent ),
			s->packetsContainingOnlyAcknowlegements,
			s->packetsContainingOnlyAcknowlegementsAndResends,
			s->messageResends,
			BITS_TO_BYTES( s->messageDataBitsResent ),
			BITS_TO_BYTES( s->messagesTotalBitsResent - s->messageDataBitsResent ),
			BITS_TO_BYTES( s->messagesTotalBitsResent ),
			s->numberOfSplitMessages,
			s->numberOfUnsplitMessages,
			s->totalSplits,
			s->sequencedMessagesOutOfOrder,
			s->sequencedMessagesInOrder,
			s->orderedMessagesOutOfOrder,
			s->orderedMessagesInOrder,
			s->messagesWaitingForReassembly,
			s->internalOutputQueueSize,
			s->windowSize,
			s->lossySize,
			s->connectionStartTime );
	}
}
