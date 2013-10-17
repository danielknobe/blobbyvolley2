/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief Store Statistics concerning Network Usage.
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

#ifndef __RAK_NET_STATISTICS_H
#define __RAK_NET_STATISTICS_H

#include "PacketPriority.h"
/**
 * @brief Network Statisics Usage
 *
 * Store Statistics information related to network usage
 */

struct RakNetStatisticsStruct
{
	//!  Number of Messages in the send Buffer (high, medium, low priority)
	unsigned messageSendBuffer[ NUMBER_OF_PRIORITIES ];
	//!  Number of messages sent (high, medium, low priority)
	unsigned messagesSent[ NUMBER_OF_PRIORITIES ];
	//!  Number of data bits used for user messages
	unsigned messageDataBitsSent[ NUMBER_OF_PRIORITIES ];
	//!  Number of total bits used for user messages, including headers
	unsigned messageTotalBitsSent[ NUMBER_OF_PRIORITIES ];

	//!  Number of packets sent containing only acknowledgements
	unsigned packetsContainingOnlyAcknowlegements;
	//!  Number of acknowledgements sent
	unsigned acknowlegementsSent;
	//!  Number of acknowledgements waiting to be sent
	unsigned acknowlegementsPending;
	//!  Number of acknowledgements bits sent
	unsigned acknowlegementBitsSent;

	//!  Number of packets containing only acknowledgements and resends
	unsigned packetsContainingOnlyAcknowlegementsAndResends;

	//!  Number of messages resent
	unsigned messageResends;
	//!  Number of bits resent of actual data
	unsigned messageDataBitsResent;
	//!  Total number of bits resent, including headers
	unsigned messagesTotalBitsResent;
	//!  Number of messages waiting for ack
	unsigned messagesOnResendQueue;

	//!  Number of messages not split for sending
	unsigned numberOfUnsplitMessages;
	//!  Number of messages split for sending
	unsigned numberOfSplitMessages;
	//!  Total number of splits done for sending
	unsigned totalSplits;

	//!  Total packets sent
	unsigned packetsSent;

	//!  total bits sent
	unsigned totalBitsSent;

	//!  Number of sequenced messages arrived out of order
	unsigned sequencedMessagesOutOfOrder;
	//!  Number of sequenced messages arrived in order
	unsigned sequencedMessagesInOrder;

	//!  Number of ordered messages arrived out of order
	unsigned orderedMessagesOutOfOrder;
	//!  Number of ordered messages arrived in order
	unsigned orderedMessagesInOrder;

	//!  Packets with a good CRC received
	unsigned packetsReceived;
	//!  Packets with a bad CRC received
	unsigned packetsWithBadCRCReceived;
	//!  Bits with a good CRC received
	unsigned bitsReceived;
	//!  Bits with a bad CRC received
	unsigned bitsWithBadCRCReceived;
	//!  Number of acknowledgement messages received for packets we are resending
	unsigned acknowlegementsReceived;
	//!  Number of acknowledgement messages received for packets we are not resending
	unsigned duplicateAcknowlegementsReceived;
	//!  Number of data messages (anything other than an ack) received that are valid and not duplicate
	unsigned messagesReceived;
	//!  Number of data messages (anything other than an ack) received that are invalid
	unsigned invalidMessagesReceived;
	//!  Number of data messages (anything other than an ack) received that are duplicate
	unsigned duplicateMessagesReceived;
	//!  Number of messages waiting for reassembly
	unsigned messagesWaitingForReassembly;
	//!  Number of messages in reliability output queue
	unsigned internalOutputQueueSize;
	//!  Current window size
	unsigned windowSize;
	//!  lossy window size
	unsigned lossySize;
	//!  connection start time
	unsigned int connectionStartTime;
};


/**
 * Verbosity level currently supports 0 (low), 1 (medium), 2 (high)
 * @param s The Statistical information to format out
 * @param buffer The buffer containing a formated report
 * @param verbosityLevel
 *  - 0 low
 *  - 1 medium
 *  - 2 high
 */
void StatisticsToString( RakNetStatisticsStruct *s, char *buffer, int verbosityLevel );

#endif
