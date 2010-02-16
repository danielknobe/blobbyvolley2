/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @ingroup RAKNET_DNO 
 * @file 
 * @brief Provide Macro for the Distributed Object System
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

#include "DistributedNetworkObjectStub.h"
#include "DistributedNetworkObject.h"
#include "BitStream.h"
#include "GetTime.h"
#include <memory.h>
#include <assert.h>

/**
 * @ingroup RAKNET_DNO
 * This define should be put after the class definition, once per
 * class that you want to be distributed
 */
#define REGISTER_DISTRIBUTED_CLASS(className) \
	DistributedNetworkObjectStub<className> distributedNetworkObjectStub_##className(#className);
/**
 * @ingroup RAKNET_DNO 
 * DOM stands for distributed object member -
 * COPY will send data without interpolation. When the packet arrives the remote value will immediately take the new value.
 * INTERPOLATE will send data and use interpolation.
 * When the packet arrives the remote value will gradually take the new value according to DistributedNetworkObject::maximumUpdateFrequency
 * Uncompressed should be used for non-native types, such as structures, or native types whose values take the range of the entire variable.
 * At this time Uncompressed will not work with arrays or pointers.  However, you can enclose arrays in a struct or register each element
 * Compressed can only be used for native types, and will result in data compression if the variable value is less than half of its maximum range
 */
#define DOM_COPY_UNCOMPRESSED
#define DOM_COPY_COMPRESSED
#define DOM_INTERPOLATE_UNCOMPRESSED
#define DOM_INTERPOLATE_COMPRESSED

#define DOM_SERVER_AUTHORITATIVE (isServerAuthoritative==true)
#define DOM_CLIENT_AUTHORITATIVE (isServerAuthoritative==false)

/**
 * @ingroup RAKNET_DNO 
 * REGISTER_DISTRIBUTED_OBJECT_MEMBERS is optional.  If you use it, it must be put in the public section of the class that you want to have
 * automatically synchronized member variables.
 * Base class should be the class the enclosing class immediately derives from.  You can pass DistributedNetworkObject if it derives from no other class.
 * AuthoritativeNetwork should be DOM_SERVER_AUTHORITATIVE or DOM_CLIENT_AUTHORITATIVE depending on which system is the authority for an object
 * SynchronizationMethod should be any of the values immediately above
 * Variable type can be any type other than a pointer. However, for interpolation it must also have *, +, -, and = defined.
 * Variable name should be the name of the variable to synchronize and its type must match the type specified immediately prior.
 */
#pragma deprecated(REGISTER_1_DISTRIBUTED_OBJECT_MEMBERS) // This whole file is depreciated.  Use the new object replication system instead
#define REGISTER_1_DISTRIBUTED_OBJECT_MEMBERS(BaseClass, SynchronizationMethod1, AuthoritativeNetwork1, VariableType1, VariableName1)\
	SynchronizationMethod1##_INTERPOLATION_MEMBERS(VariableType1, VariableName1)  \
	VariableType1 VariableName1##_LastReadValue, VariableName1##_LastWriteValue; \
	\
	virtual void DistributedMemoryInit(bool isServerAuthoritative) \
	{ \
		BaseClass::DistributedMemoryInit(isServerAuthoritative); \
		SynchronizationMethod1##_INTERPOLATION_MEMORY_INIT(VariableName1) \
	} \
	virtual bool ProcessDistributedMemoryStack(RakNet::BitStream *bitStream, bool isWrite, bool forceWrite, bool isServerAuthoritative) \
	{ \
		bool anyDataWritten=false; \
		bool dataChanged=false; \
		if (distributedMemoryInitialized==false) \
		{ \
			DistributedMemoryInit(isServerAuthoritative); \
			SynchronizationMethod1##_EXPANDED(AuthoritativeNetwork1, VariableType1, VariableName1) \
		} \
		else \
		{ \
			SynchronizationMethod1##_EXPANDED(AuthoritativeNetwork1, VariableType1, VariableName1) \
		} \
		\
		if (BaseClass::ProcessDistributedMemoryStack(bitStream, isWrite, forceWrite,isServerAuthoritative)==true) \
			anyDataWritten=true;\
		return anyDataWritten; \
	} \
	\
	virtual void InterpolateDistributedMemory(bool isServerAuthoritative) \
	{ \
		unsigned int currentTime=RakNet::GetTime(); \
		if (distributedMemoryInitialized==false) \
			DistributedMemoryInit(isServerAuthoritative); \
		SynchronizationMethod1##_INTERPOLATION_CODE(AuthoritativeNetwork1, VariableName1) \
	}

#define DOM_COPY_UNCOMPRESSED_INTERPOLATION_CODE(AuthoritativeNetwork,VariableName)
#define DOM_COPY_COMPRESSED_INTERPOLATION_CODE(AuthoritativeNetwork,VariableName)
#define DOM_INTERPOLATE_UNCOMPRESSED_INTERPOLATION_CODE(AuthoritativeNetwork,VariableName) DOM_INTERPOLATION_CODE(AuthoritativeNetwork,VariableName)
#define DOM_INTERPOLATE_COMPRESSED_INTERPOLATION_CODE(AuthoritativeNetwork,VariableName) DOM_INTERPOLATION_CODE(AuthoritativeNetwork,VariableName) 
/**
 * @ingroup RAKNET_DNO 
 * This is called when you call DistributedNetworkObject::InterpolateDistributedMemory, one per interpolated member
 */
#define DOM_INTERPOLATION_CODE(AuthoritativeNetwork, VariableName) \
	if (VariableName!=VariableName##_FinalValue && VariableName##_LastKnownValue!=VariableName##_LastReadValue && VariableName##_LastKnownValue==VariableName) \
	{ \
		if (currentTime >= VariableName##_InterpolationEndTime) \
			VariableName=VariableName##_FinalValue; \
		else \
		{ \
			VariableName = VariableName##_InitialValue + (currentTime - VariableName##_InterpolationStartTime) / (float)(VariableName##_InterpolationEndTime - VariableName##_InterpolationStartTime) * (VariableName##_FinalValue - VariableName##_InitialValue); \
		} \
		VariableName##_LastKnownValue = VariableName; \
	} \

#define DOM_COPY_UNCOMPRESSED_INTERPOLATION_MEMBERS(VariableType, VariableName)
#define DOM_COPY_COMPRESSED_INTERPOLATION_MEMBERS(VariableType, VariableName)
#define DOM_INTERPOLATE_UNCOMPRESSED_INTERPOLATION_MEMBERS(VariableType, VariableName) unsigned int VariableName##_InterpolationStartTime,VariableName##_InterpolationEndTime; VariableType VariableName##_InitialValue, VariableName##_FinalValue, VariableName##_LastKnownValue;
#define DOM_INTERPOLATE_COMPRESSED_INTERPOLATION_MEMBERS(VariableType, VariableName) unsigned int VariableName##_InterpolationStartTime,VariableName##_InterpolationEndTime; VariableType VariableName##_InitialValue, VariableName##_FinalValue, VariableName##_LastKnownValue;

#define DOM_COPY_UNCOMPRESSED_INTERPOLATION_MEMORY_INIT(VariableName)
#define DOM_COPY_COMPRESSED_INTERPOLATION_MEMORY_INIT(VariableName)
#define DOM_INTERPOLATE_UNCOMPRESSED_INTERPOLATION_MEMORY_INIT(VariableName) memcpy((char*)&VariableName##_LastReadValue,(char*)&VariableName, sizeof(VariableName)); memcpy((char*)&VariableName##_LastKnownValue,(char*)&VariableName, sizeof(VariableName)); memcpy((char*)&VariableName##_FinalValue,(char*)&VariableName, sizeof(VariableName));
#define DOM_INTERPOLATE_COMPRESSED_INTERPOLATION_MEMORY_INIT(VariableName) memcpy((char*)&VariableName##_LastReadValue,(char*)&VariableName, sizeof(VariableName)); memcpy((char*)&VariableName##_LastKnownValue,(char*)&VariableName, sizeof(VariableName)); memcpy((char*)&VariableName##_FinalValue,(char*)&VariableName, sizeof(VariableName));

#define DOM_COPY_UNCOMPRESSED_EXPANDED(AuthoritativeNetwork,VariableType, VariableName)\
	DOM_COPY_EXPANDED2(AuthoritativeNetwork,VariableType,VariableName, readSuccess=bitStream->Read((char*)&VariableName##_LastReadValue, sizeof(VariableName));,\
	                   readSuccess=bitStream->Read((char*)&dummy, sizeof(VariableName));,\
	                   bitStream->Write((char*)&VariableName, sizeof(VariableName));)
#define DOM_COPY_COMPRESSED_EXPANDED(AuthoritativeNetwork,VariableType, VariableName)\
	DOM_COPY_EXPANDED2(AuthoritativeNetwork,VariableType,VariableName, readSuccess=bitStream->ReadCompressed(VariableName##_LastReadValue);,\
	                   readSuccess=bitStream->ReadCompressed(dummy);,\
	                   bitStream->WriteCompressed(VariableName);)
#define DOM_INTERPOLATE_UNCOMPRESSED_EXPANDED(AuthoritativeNetwork,VariableType, VariableName)\
	DOM_INTERPOLATE_EXPANDED2(AuthoritativeNetwork,VariableType,VariableName, readSuccess=bitStream->Read((char*)&VariableName##_LastReadValue, sizeof(VariableName));,\
	                          readSuccess=bitStream->Read((char*)&dummy, sizeof(VariableName));,\
	                          if (VariableName##_LastKnownValue!=VariableName##_LastReadValue && VariableName##_LastKnownValue==VariableName) bitStream->Write((char*)&VariableName##_FinalValue, sizeof(VariableName));\
	                          else bitStream->Write((char*)&VariableName, sizeof(VariableName));)
#define DOM_INTERPOLATE_COMPRESSED_EXPANDED(AuthoritativeNetwork,VariableType, VariableName)\
	DOM_INTERPOLATE_EXPANDED2(AuthoritativeNetwork,VariableType,VariableName, readSuccess=bitStream->ReadCompressed(VariableName##_LastReadValue);,\
	                          readSuccess=bitStream->ReadCompressed(dummy);,\
	                          if (VariableName##_LastKnownValue!=VariableName##_LastReadValue && VariableName##_LastKnownValue==VariableName) bitStream->WriteCompressed(VariableName##_FinalValue);\
	                          else bitStream->WriteCompressed(VariableName);)

#define DOM_COPY_EXPANDED2(AuthoritativeNetwork,VariableType,VariableName, ReadCode, ReadDummyCode, WriteCode) \
	DOM_CORE_EXPANDED(AuthoritativeNetwork,VariableType,VariableName, ReadCode, ReadDummyCode, WriteCode, true) \
	if (isWrite==false && dataChanged==true) \
		VariableName=VariableName##_LastReadValue;

#define DOM_INTERPOLATE_EXPANDED2(AuthoritativeNetwork,VariableType,VariableName, ReadCode, ReadDummyCode, WriteCode) \
	DOM_CORE_EXPANDED(AuthoritativeNetwork,VariableType,VariableName, ReadCode, ReadDummyCode, WriteCode, VariableName!=VariableName##_LastKnownValue) \
	if (isWrite==false && dataChanged==true) \
	{ \
		VariableName##_InterpolationStartTime=RakNet::GetTime(); \
		VariableName##_InterpolationEndTime=VariableName##_InterpolationStartTime+maximumUpdateFrequency; \
		VariableName##_InitialValue=VariableName; \
		VariableName##_FinalValue=VariableName##_LastReadValue; \
		VariableName##_LastKnownValue=VariableName; \
	}

#define DOM_CORE_EXPANDED(AuthoritativeNetwork,VariableType,VariableName, ReadCode, ReadDummyCode, WriteCode, InterpolationWriteCondition) \
	assert(sizeof(VariableName)==sizeof(VariableName##_LastWriteValue)); \
	if (isWrite) \
	{ \
		if (forceWrite) \
		{ \
			bitStream->Write(true); \
			WriteCode \
			memcpy(&VariableName##_LastWriteValue, &VariableName, sizeof(VariableName)); \
			anyDataWritten=true; \
		} \
		else \
		{ \
			if (AuthoritativeNetwork && memcmp(&VariableName, &VariableName##_LastWriteValue, sizeof(VariableName))!=0 && InterpolationWriteCondition)\
			{ \
				bitStream->Write(true); \
				WriteCode \
				memcpy(&VariableName##_LastWriteValue, &VariableName, sizeof(VariableName)); \
				anyDataWritten=true; \
			} \
			else \
			{ \
				bitStream->Write(false); \
			} \
		} \
	} \
	else \
	{ \
		if (bitStream->Read(dataChanged)==false) \
		{ \
			assert(0==1); \
			return false; \
		} \
		if (dataChanged) \
		{ \
			if (isServerAuthoritative==false || AuthoritativeNetwork==0) \
			{ \
				bool readSuccess; \
				ReadCode\
				if (readSuccess==false) \
				{ \
					assert(0==2); \
					return false; \
				} \
				if (isServerAuthoritative==false) \
					memcpy((char*)&VariableName##_LastWriteValue, (char*)&VariableName##_LastReadValue, sizeof(VariableName##_LastWriteValue));  \
			} \
			else \
			{ \
				VariableType dummy; \
				bool readSuccess; \
				ReadDummyCode\
				if (readSuccess==false) \
				{ \
					assert(0==2); \
					return false; \
				} \
			} \
		} \
	}
// -------------------------------------------------------------------------------------------------------------------------------------------
#define REGISTER_2_DISTRIBUTED_OBJECT_MEMBERS(BaseClass, SynchronizationMethod1, AuthoritativeNetwork1, VariableType1, VariableName1 \
                , SynchronizationMethod2, AuthoritativeNetwork2, VariableType2, VariableName2 \
                                             )\
SynchronizationMethod1##_INTERPOLATION_MEMBERS(VariableType1, VariableName1)  \
SynchronizationMethod2##_INTERPOLATION_MEMBERS(VariableType2, VariableName2)  \
VariableType1 VariableName1##_LastReadValue, VariableName1##_LastWriteValue; \
VariableType2 VariableName2##_LastReadValue, VariableName2##_LastWriteValue; \
\
virtual void DistributedMemoryInit(bool isServerAuthoritative) \
{ \
	BaseClass::DistributedMemoryInit(isServerAuthoritative); \
	SynchronizationMethod1##_INTERPOLATION_MEMORY_INIT(VariableName1) \
	SynchronizationMethod2##_INTERPOLATION_MEMORY_INIT(VariableName2) \
} \
virtual bool ProcessDistributedMemoryStack(RakNet::BitStream *bitStream, bool isWrite, bool forceWrite, bool isServerAuthoritative) \
{ \
	bool anyDataWritten=false; \
	bool dataChanged=false; \
	if (distributedMemoryInitialized==false) \
	{ \
		DistributedMemoryInit(isServerAuthoritative); \
		SynchronizationMethod1##_EXPANDED(AuthoritativeNetwork1, VariableType1, VariableName1) \
		SynchronizationMethod2##_EXPANDED(AuthoritativeNetwork2, VariableType2, VariableName2) \
	} \
	else \
		SynchronizationMethod1##_EXPANDED(AuthoritativeNetwork1, VariableType1, VariableName1) \
		SynchronizationMethod2##_EXPANDED(AuthoritativeNetwork2, VariableType2, VariableName2) \
		if (BaseClass::ProcessDistributedMemoryStack(bitStream, isWrite, forceWrite,isServerAuthoritative)==true) \
			anyDataWritten=true;\
	return anyDataWritten; \
} \
\
virtual void InterpolateDistributedMemory(bool isServerAuthoritative) \
{ \
	unsigned int currentTime=RakNet::GetTime(); \
	float percentageOfTimeElapsed; \
	if (distributedMemoryInitialized==false) \
		DistributedMemoryInit(isServerAuthoritative); \
	SynchronizationMethod1##_INTERPOLATION_CODE(AuthoritativeNetwork1, VariableName1) \
	SynchronizationMethod2##_INTERPOLATION_CODE(AuthoritativeNetwork2, VariableName2) \
}
// -------------------------------------------------------------------------------------------------------------------------------------------
#define REGISTER_3_DISTRIBUTED_OBJECT_MEMBERS(BaseClass, SynchronizationMethod1, AuthoritativeNetwork1, VariableType1, VariableName1 \
                , SynchronizationMethod2, AuthoritativeNetwork2, VariableType2, VariableName2 \
                , SynchronizationMethod3, AuthoritativeNetwork3, VariableType3, VariableName3 \
                                             )\
SynchronizationMethod1##_INTERPOLATION_MEMBERS(VariableType1, VariableName1)  \
SynchronizationMethod2##_INTERPOLATION_MEMBERS(VariableType2, VariableName2)  \
SynchronizationMethod3##_INTERPOLATION_MEMBERS(VariableType3, VariableName3)  \
VariableType1 VariableName1##_LastReadValue, VariableName1##_LastWriteValue; \
VariableType2 VariableName2##_LastReadValue, VariableName2##_LastWriteValue; \
VariableType3 VariableName3##_LastReadValue, VariableName3##_LastWriteValue; \
\
virtual void DistributedMemoryInit(bool isServerAuthoritative) \
{ \
	BaseClass::DistributedMemoryInit(isServerAuthoritative); \
	SynchronizationMethod1##_INTERPOLATION_MEMORY_INIT(VariableName1) \
	SynchronizationMethod2##_INTERPOLATION_MEMORY_INIT(VariableName2) \
	SynchronizationMethod3##_INTERPOLATION_MEMORY_INIT(VariableName3) \
} \
virtual bool ProcessDistributedMemoryStack(RakNet::BitStream *bitStream, bool isWrite, bool forceWrite, bool isServerAuthoritative) \
{ \
	bool anyDataWritten=false; \
	bool dataChanged=false; \
	if (distributedMemoryInitialized==false) \
	{ \
		DistributedMemoryInit(isServerAuthoritative); \
		SynchronizationMethod1##_EXPANDED(AuthoritativeNetwork1, VariableType1, VariableName1) \
		SynchronizationMethod2##_EXPANDED(AuthoritativeNetwork2, VariableType2, VariableName2) \
		SynchronizationMethod3##_EXPANDED(AuthoritativeNetwork3, VariableType3, VariableName3) \
	} \
	else \
		SynchronizationMethod1##_EXPANDED(AuthoritativeNetwork1, VariableType1, VariableName1) \
		SynchronizationMethod2##_EXPANDED(AuthoritativeNetwork2, VariableType2, VariableName2) \
		SynchronizationMethod3##_EXPANDED(AuthoritativeNetwork3, VariableType3, VariableName3) \
		if (BaseClass::ProcessDistributedMemoryStack(bitStream, isWrite, forceWrite,isServerAuthoritative)==true) \
			anyDataWritten=true;\
	return anyDataWritten; \
} \
\
virtual void InterpolateDistributedMemory(bool isServerAuthoritative) \
{ \
	unsigned int currentTime=RakNet::GetTime(); \
	float percentageOfTimeElapsed; \
	if (distributedMemoryInitialized==false) \
		DistributedMemoryInit(isServerAuthoritative); \
	SynchronizationMethod1##_INTERPOLATION_CODE(AuthoritativeNetwork1, VariableName1) \
	SynchronizationMethod2##_INTERPOLATION_CODE(AuthoritativeNetwork2, VariableName2) \
	SynchronizationMethod3##_INTERPOLATION_CODE(AuthoritativeNetwork3, VariableName3) \
}
// -------------------------------------------------------------------------------------------------------------------------------------------
#define REGISTER_4_DISTRIBUTED_OBJECT_MEMBERS(BaseClass, SynchronizationMethod1, AuthoritativeNetwork1, VariableType1, VariableName1 \
                , SynchronizationMethod2, AuthoritativeNetwork2, VariableType2, VariableName2 \
                , SynchronizationMethod3, AuthoritativeNetwork3, VariableType3, VariableName3 \
                , SynchronizationMethod4, AuthoritativeNetwork4, VariableType4, VariableName4 \
                                             )\
SynchronizationMethod1##_INTERPOLATION_MEMBERS(VariableType1, VariableName1)  \
SynchronizationMethod2##_INTERPOLATION_MEMBERS(VariableType2, VariableName2)  \
SynchronizationMethod3##_INTERPOLATION_MEMBERS(VariableType3, VariableName3)  \
SynchronizationMethod4##_INTERPOLATION_MEMBERS(VariableType4, VariableName4)  \
VariableType1 VariableName1##_LastReadValue, VariableName1##_LastWriteValue; \
VariableType2 VariableName2##_LastReadValue, VariableName2##_LastWriteValue; \
VariableType3 VariableName3##_LastReadValue, VariableName3##_LastWriteValue; \
VariableType4 VariableName4##_LastReadValue, VariableName4##_LastWriteValue; \
\
virtual void DistributedMemoryInit(bool isServerAuthoritative) \
{ \
	BaseClass::DistributedMemoryInit(isServerAuthoritative); \
	SynchronizationMethod1##_INTERPOLATION_MEMORY_INIT(VariableName1) \
	SynchronizationMethod2##_INTERPOLATION_MEMORY_INIT(VariableName2) \
	SynchronizationMethod3##_INTERPOLATION_MEMORY_INIT(VariableName3) \
	SynchronizationMethod4##_INTERPOLATION_MEMORY_INIT(VariableName4) \
} \
virtual bool ProcessDistributedMemoryStack(RakNet::BitStream *bitStream, bool isWrite, bool forceWrite, bool isServerAuthoritative) \
{ \
	bool anyDataWritten=false; \
	bool dataChanged=false; \
	if (distributedMemoryInitialized==false) \
	{ \
		DistributedMemoryInit(isServerAuthoritative); \
		SynchronizationMethod1##_EXPANDED(AuthoritativeNetwork1, VariableType1, VariableName1) \
		SynchronizationMethod2##_EXPANDED(AuthoritativeNetwork2, VariableType2, VariableName2) \
		SynchronizationMethod3##_EXPANDED(AuthoritativeNetwork3, VariableType3, VariableName3) \
		SynchronizationMethod4##_EXPANDED(AuthoritativeNetwork4, VariableType4, VariableName4) \
	} \
	else \
		SynchronizationMethod1##_EXPANDED(AuthoritativeNetwork1, VariableType1, VariableName1) \
		SynchronizationMethod2##_EXPANDED(AuthoritativeNetwork2, VariableType2, VariableName2) \
		SynchronizationMethod3##_EXPANDED(AuthoritativeNetwork3, VariableType3, VariableName3) \
		SynchronizationMethod4##_EXPANDED(AuthoritativeNetwork4, VariableType4, VariableName4) \
		if (BaseClass::ProcessDistributedMemoryStack(bitStream, isWrite, forceWrite,isServerAuthoritative)==true) \
			anyDataWritten=true;\
	return anyDataWritten; \
} \
\
virtual void InterpolateDistributedMemory(bool isServerAuthoritative) \
{ \
	unsigned int currentTime=RakNet::GetTime(); \
	float percentageOfTimeElapsed; \
	if (distributedMemoryInitialized==false) \
		DistributedMemoryInit(isServerAuthoritative); \
	SynchronizationMethod1##_INTERPOLATION_CODE(AuthoritativeNetwork1, VariableName1) \
	SynchronizationMethod2##_INTERPOLATION_CODE(AuthoritativeNetwork2, VariableName2) \
	SynchronizationMethod3##_INTERPOLATION_CODE(AuthoritativeNetwork3, VariableName3) \
	SynchronizationMethod4##_INTERPOLATION_CODE(AuthoritativeNetwork4, VariableName4) \
}
// -------------------------------------------------------------------------------------------------------------------------------------------
#define REGISTER_5_DISTRIBUTED_OBJECT_MEMBERS(BaseClass, SynchronizationMethod1, AuthoritativeNetwork1, VariableType1, VariableName1 \
                , SynchronizationMethod2, AuthoritativeNetwork2, VariableType2, VariableName2 \
                , SynchronizationMethod3, AuthoritativeNetwork3, VariableType3, VariableName3 \
                , SynchronizationMethod4, AuthoritativeNetwork4, VariableType4, VariableName4 \
                , SynchronizationMethod5, AuthoritativeNetwork5, VariableType5, VariableName5 \
                                             )\
SynchronizationMethod1##_INTERPOLATION_MEMBERS(VariableType1, VariableName1)  \
SynchronizationMethod2##_INTERPOLATION_MEMBERS(VariableType2, VariableName2)  \
SynchronizationMethod3##_INTERPOLATION_MEMBERS(VariableType3, VariableName3)  \
SynchronizationMethod4##_INTERPOLATION_MEMBERS(VariableType4, VariableName4)  \
SynchronizationMethod5##_INTERPOLATION_MEMBERS(VariableType5, VariableName5)  \
VariableType1 VariableName1##_LastReadValue, VariableName1##_LastWriteValue; \
VariableType2 VariableName2##_LastReadValue, VariableName2##_LastWriteValue; \
VariableType3 VariableName3##_LastReadValue, VariableName3##_LastWriteValue; \
VariableType4 VariableName4##_LastReadValue, VariableName4##_LastWriteValue; \
VariableType5 VariableName5##_LastReadValue, VariableName5##_LastWriteValue; \
\
virtual void DistributedMemoryInit(bool isServerAuthoritative) \
{ \
	BaseClass::DistributedMemoryInit(isServerAuthoritative); \
	SynchronizationMethod1##_INTERPOLATION_MEMORY_INIT(VariableName1) \
	SynchronizationMethod2##_INTERPOLATION_MEMORY_INIT(VariableName2) \
	SynchronizationMethod3##_INTERPOLATION_MEMORY_INIT(VariableName3) \
	SynchronizationMethod4##_INTERPOLATION_MEMORY_INIT(VariableName4) \
	SynchronizationMethod5##_INTERPOLATION_MEMORY_INIT(VariableName5) \
} \
virtual bool ProcessDistributedMemoryStack(RakNet::BitStream *bitStream, bool isWrite, bool forceWrite, bool isServerAuthoritative) \
{ \
	bool anyDataWritten=false; \
	bool dataChanged=false; \
	if (distributedMemoryInitialized==false) \
	{ \
		DistributedMemoryInit(isServerAuthoritative); \
		SynchronizationMethod1##_EXPANDED(AuthoritativeNetwork1, VariableType1, VariableName1) \
		SynchronizationMethod2##_EXPANDED(AuthoritativeNetwork2, VariableType2, VariableName2) \
		SynchronizationMethod3##_EXPANDED(AuthoritativeNetwork3, VariableType3, VariableName3) \
		SynchronizationMethod4##_EXPANDED(AuthoritativeNetwork4, VariableType4, VariableName4) \
		SynchronizationMethod5##_EXPANDED(AuthoritativeNetwork5, VariableType5, VariableName5) \
	} \
	else \
		SynchronizationMethod1##_EXPANDED(AuthoritativeNetwork1, VariableType1, VariableName1) \
		SynchronizationMethod2##_EXPANDED(AuthoritativeNetwork2, VariableType2, VariableName2) \
		SynchronizationMethod3##_EXPANDED(AuthoritativeNetwork3, VariableType3, VariableName3) \
		SynchronizationMethod4##_EXPANDED(AuthoritativeNetwork4, VariableType4, VariableName4) \
		SynchronizationMethod5##_EXPANDED(AuthoritativeNetwork5, VariableType5, VariableName5) \
		if (BaseClass::ProcessDistributedMemoryStack(bitStream, isWrite, forceWrite,isServerAuthoritative)==true) \
			anyDataWritten=true;\
	return anyDataWritten; \
} \
\
virtual void InterpolateDistributedMemory(bool isServerAuthoritative) \
{ \
	unsigned int currentTime=RakNet::GetTime(); \
	float percentageOfTimeElapsed; \
	if (distributedMemoryInitialized==false) \
		DistributedMemoryInit(isServerAuthoritative); \
	SynchronizationMethod1##_INTERPOLATION_CODE(AuthoritativeNetwork1, VariableName1) \
	SynchronizationMethod2##_INTERPOLATION_CODE(AuthoritativeNetwork2, VariableName2) \
	SynchronizationMethod3##_INTERPOLATION_CODE(AuthoritativeNetwork3, VariableName3) \
	SynchronizationMethod4##_INTERPOLATION_CODE(AuthoritativeNetwork4, VariableName4) \
	SynchronizationMethod5##_INTERPOLATION_CODE(AuthoritativeNetwork5, VariableName5) \
}

// -------------------------------------------------------------------------------------------------------------------------------------------
#define REGISTER_6_DISTRIBUTED_OBJECT_MEMBERS(BaseClass, SynchronizationMethod1, AuthoritativeNetwork1, VariableType1, VariableName1 \
                , SynchronizationMethod2, AuthoritativeNetwork2, VariableType2, VariableName2 \
                , SynchronizationMethod3, AuthoritativeNetwork3, VariableType3, VariableName3 \
                , SynchronizationMethod4, AuthoritativeNetwork4, VariableType4, VariableName4 \
                , SynchronizationMethod5, AuthoritativeNetwork5, VariableType5, VariableName5 \
                , SynchronizationMethod6, AuthoritativeNetwork6, VariableType6, VariableName6 \
                                             )\
SynchronizationMethod1##_INTERPOLATION_MEMBERS(VariableType1, VariableName1)  \
SynchronizationMethod2##_INTERPOLATION_MEMBERS(VariableType2, VariableName2)  \
SynchronizationMethod3##_INTERPOLATION_MEMBERS(VariableType3, VariableName3)  \
SynchronizationMethod4##_INTERPOLATION_MEMBERS(VariableType4, VariableName4)  \
SynchronizationMethod5##_INTERPOLATION_MEMBERS(VariableType5, VariableName5)  \
SynchronizationMethod6##_INTERPOLATION_MEMBERS(VariableType6, VariableName6)  \
VariableType1 VariableName1##_LastReadValue, VariableName1##_LastWriteValue; \
VariableType2 VariableName2##_LastReadValue, VariableName2##_LastWriteValue; \
VariableType3 VariableName3##_LastReadValue, VariableName3##_LastWriteValue; \
VariableType4 VariableName4##_LastReadValue, VariableName4##_LastWriteValue; \
VariableType5 VariableName5##_LastReadValue, VariableName5##_LastWriteValue; \
VariableType6 VariableName6##_LastReadValue, VariableName6##_LastWriteValue; \
\
virtual void DistributedMemoryInit(bool isServerAuthoritative) \
{ \
	BaseClass::DistributedMemoryInit(isServerAuthoritative); \
	SynchronizationMethod1##_INTERPOLATION_MEMORY_INIT(VariableName1) \
	SynchronizationMethod2##_INTERPOLATION_MEMORY_INIT(VariableName2) \
	SynchronizationMethod3##_INTERPOLATION_MEMORY_INIT(VariableName3) \
	SynchronizationMethod4##_INTERPOLATION_MEMORY_INIT(VariableName4) \
	SynchronizationMethod5##_INTERPOLATION_MEMORY_INIT(VariableName5) \
	SynchronizationMethod6##_INTERPOLATION_MEMORY_INIT(VariableName6) \
} \
virtual bool ProcessDistributedMemoryStack(RakNet::BitStream *bitStream, bool isWrite, bool forceWrite, bool isServerAuthoritative) \
{ \
	bool anyDataWritten=false; \
	bool dataChanged=false; \
	if (distributedMemoryInitialized==false) \
	{ \
		DistributedMemoryInit(isServerAuthoritative); \
		SynchronizationMethod1##_EXPANDED(AuthoritativeNetwork1, VariableType1, VariableName1) \
		SynchronizationMethod2##_EXPANDED(AuthoritativeNetwork2, VariableType2, VariableName2) \
		SynchronizationMethod3##_EXPANDED(AuthoritativeNetwork3, VariableType3, VariableName3) \
		SynchronizationMethod4##_EXPANDED(AuthoritativeNetwork4, VariableType4, VariableName4) \
		SynchronizationMethod5##_EXPANDED(AuthoritativeNetwork5, VariableType5, VariableName5) \
		SynchronizationMethod6##_EXPANDED(AuthoritativeNetwork6, VariableType6, VariableName6) \
	} \
	else \
		SynchronizationMethod1##_EXPANDED(AuthoritativeNetwork1, VariableType1, VariableName1) \
		SynchronizationMethod2##_EXPANDED(AuthoritativeNetwork2, VariableType2, VariableName2) \
		SynchronizationMethod3##_EXPANDED(AuthoritativeNetwork3, VariableType3, VariableName3) \
		SynchronizationMethod4##_EXPANDED(AuthoritativeNetwork4, VariableType4, VariableName4) \
		SynchronizationMethod5##_EXPANDED(AuthoritativeNetwork5, VariableType5, VariableName5) \
		SynchronizationMethod6##_EXPANDED(AuthoritativeNetwork6, VariableType6, VariableName6) \
		if (BaseClass::ProcessDistributedMemoryStack(bitStream, isWrite, forceWrite,isServerAuthoritative)==true) \
			anyDataWritten=true;\
	return anyDataWritten; \
} \
\
virtual void InterpolateDistributedMemory(bool isServerAuthoritative) \
{ \
	unsigned int currentTime=RakNet::GetTime(); \
	float percentageOfTimeElapsed; \
	if (distributedMemoryInitialized==false) \
		DistributedMemoryInit(isServerAuthoritative); \
	SynchronizationMethod1##_INTERPOLATION_CODE(AuthoritativeNetwork1, VariableName1) \
	SynchronizationMethod2##_INTERPOLATION_CODE(AuthoritativeNetwork2, VariableName2) \
	SynchronizationMethod3##_INTERPOLATION_CODE(AuthoritativeNetwork3, VariableName3) \
	SynchronizationMethod4##_INTERPOLATION_CODE(AuthoritativeNetwork4, VariableName4) \
	SynchronizationMethod5##_INTERPOLATION_CODE(AuthoritativeNetwork5, VariableName5) \
	SynchronizationMethod6##_INTERPOLATION_CODE(AuthoritativeNetwork6, VariableName6) \
}
/**
 * @internal 
 * @note Extend this pattern as many times as necessary.  If anyone
 * figures out a way to automate a way to do that with the
 * preprocessor let me know
 */
