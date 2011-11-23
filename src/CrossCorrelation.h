/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

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

typedef float(*timeWeightFunction)(float pdt);
inline float constantWeightFunction(float )
{
	return 1;
}

/// \brief Results of Cross Correlation Test
struct CC_Result
{
	CC_Result(int o) : offset(o)
	{
		
	}
	
	int offset;		//!< determined time shift
};

#if CC_USE_EXTERN_DEFINITIONS
// just extern template definitions
template<class T>
extern float crossCorrelationTest(const T& signal, const T& search_pattern, int offset);

template<class T>
extern CC_Result crossCorrelation(const T& A, const T& B);

#else

// Functions for cross correlation. we should move them into a seperate file
template<class T>
float crossCorrelationTest(const T& signal, const T& search_pattern, int offset, timeWeightFunction twf)
{
	float rel = 0;
	typename T::const_iterator signal_read = signal.begin();
	std::advance(signal_read, offset);
	int pos = 0;
	int len = search_pattern.size();
	for(typename T::const_iterator comp = search_pattern.begin(); comp != search_pattern.end(); ++comp, ++signal_read, ++pos)
	{
		assert(signal_read != signal.end());
		if((*signal_read) == (*comp))
			rel += twf( (float)pos / len );
	};
	
	return rel / search_pattern.size();
}

/// \brief Cross Correlation between to containers
/// \todo document algorithm
template<class T>
CC_Result crossCorrelation(const T& A, const T& B, timeWeightFunction f = constantWeightFunction)
{
	assert(A.size() >= B.size());
	
	float best = 0;
	int boffset = 0;
	int samevals = 0;
	
	for(unsigned int offset = 0; offset <= A.size() - B.size(); ++offset)
	{
		float val = crossCorrelationTest(A, B, offset, f);
		if(val > best)
		{
			best = val;
			boffset = offset;
			samevals = 1;
		} else if ( val == best)
		{
			samevals++;
		}
		
	}
	
	return CC_Result(boffset);
}

#endif
