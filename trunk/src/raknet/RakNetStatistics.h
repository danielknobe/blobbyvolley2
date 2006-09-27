/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief Store Statistics concerning Network Usage. 
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
	
	//!  Number of bits added by encryption
	unsigned encryptionBitsSent;
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
