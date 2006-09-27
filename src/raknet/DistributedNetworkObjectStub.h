/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @ingroup RAKNET_DNO
 * @file 
 * @brief Provide glue code for all Distributed Object. 
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

