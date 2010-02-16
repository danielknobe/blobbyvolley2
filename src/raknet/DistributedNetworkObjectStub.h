/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @ingroup RAKNET_DNO
 * @file 
 * @brief Provide glue code for all Distributed Object. 
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

#ifndef __DISTRIBUTED_CLASS_STUB_H
#define __DISTRIBUTED_CLASS_STUB_H
#include "EncodeClassName.h"

class DistributedNetworkObject;

/**
 * @ingroup RAKNET_DNO 
 * This is an abstract class providing glue code for the DNO subsystem.
 * You should never access this class by yourself. 
 * @see DistributedNetworkObject
 * 
 */

class DistributedNetworkObjectBaseStub
{

public:
	/**
	 * Retrieve the name of the class. 
	 * @return the name of the class.
	 */
	char *GetEncodedClassName( void ) const;
	/**
	 * Retrieved the network object associated to this instance. 
	 * @return a pointer to the network object.
	 */
	virtual DistributedNetworkObject *GetObject() = 0;
	
protected:
	/**
	 * Register the stub for a specific class name.
	 * @param className the name of the class to register 
	 */
	void RegisterStub( char *className );
	
	/**
	 * Contains the encoded class name for this stub
	 */
	char encodedClassName[ MAXIMUM_CLASS_IDENTIFIER_LENGTH ];
};

/**
 * @ingroup RAKNET_DNO 
 * Templatized Stub for all Network Objects. 
 */
template <class T>
class DistributedNetworkObjectStub : public DistributedNetworkObjectBaseStub
{
public:
	/**
	 * Create a new Stub and register it. 
	 * @param className The name of the class 
	 */
	DistributedNetworkObjectStub( char* className )
	{
		RegisterStub( className );
	};
	
	/**
	 * Return the network object associated to this instance. 
	 */
	virtual DistributedNetworkObject *GetObject()
	{
		return new T;
	};
	
protected:
};

#endif

