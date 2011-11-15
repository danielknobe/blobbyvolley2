#include "LagDetectionSystem.h"
#include <iostream>

LagDetector::LagDetector()
{
	sended.resize(100);
	received.resize(50);
}
void LagDetector::insertData(PlayerInput send_value, PlayerInput received_value)
{
	sended.push_front(send_value);
	received.push_front(received_value);
}

template<class T>
float crossCorrelationTest(const T& signal, const T& search_pattern, int offset)
{
	float rel = 0;
	typename T::const_iterator signal_read = signal.begin();
	std::advance(signal_read, offset);
	for(typename T::const_iterator comp = search_pattern.begin(); comp != search_pattern.end(); ++comp, ++signal_read)
	{
		assert(signal_read != signal.end());
		if((*signal_read) == (*comp))
			rel += 1;
	};
	
	return rel / search_pattern.size();
}

template<class T>
int crossCorrelation(const T& A, const T& B)
{
	assert(A.size() >= B.size());
	float best = 0;
	int boffset = 0;
	for(int offset = 0; offset <= A.size() - B.size(); ++offset)
	{
		float val = crossCorrelationTest(A, B, offset);
		if(val > best)
		{
			best = val;
			boffset = offset;
		}
	}
	return boffset;
}

int LagDetector::getLag() const
{
	return crossCorrelation(sended, received);
}

std::string LagDetector::getDebugString() const
{
	std::string s;
	for( boost::circular_buffer<PlayerInput>::const_iterator it = sended.begin(); it != sended.end(); ++it)
	{
		s += it->left ? (it->up ? (it->right ? '-' : 'L') : (it->right ? '_' : 'l')) : 
						(it->up ? (it->right ? 'R' : '^') : (it->right ? 'r' : 'o'));  
	}
	
	s += "\n";
	
	for( boost::circular_buffer<PlayerInput>::const_iterator it = received.begin(); it != received.end(); ++it)
	{
		s += it->left ? (it->up ? (it->right ? '-' : 'L') : (it->right ? '_' : 'l')) : (it->up ? (it->right ? 'R' : '^') : (it->right ? 'r' : 'o'));  
	}
	
	return s;
}
