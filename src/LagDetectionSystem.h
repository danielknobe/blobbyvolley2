#ifndef LAGDETECTIONSYSTEM_H_INCLUDED
#define LAGDETECTIONSYSTEM_H_INCLUDED

#include <utility>
#include <boost/circular_buffer.hpp>
#include <string>

#include "InputSource.h"

class LagDetector
{
	public:
		LagDetector();
		void insertData(PlayerInput send_value, PlayerInput received_value);
		
		int getLag() const;
		std::string getDebugString() const;
	private:
		boost::circular_buffer<PlayerInput> sended;
		boost::circular_buffer<PlayerInput> received;
};

#endif // LAGDETECTIONSYSTEM_H_INCLUDED
