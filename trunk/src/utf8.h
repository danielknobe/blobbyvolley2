
#pragma once

// utility functions for handling utf8
inline void to_utf8(wchar_t codepoint, char* target)
{
	if(codepoint <= 0x7F)
	{
		target[0] = codepoint;
	} else if (codepoint <= 0x07FF )
	{
		unsigned char high = codepoint >> 8;
		unsigned char low = codepoint & 0xFF;
		// codepoint: 00000xxx|xxyyyyyy
		// => 		  110xxxxx|10yyyyyy
		target[0] = 0xC0 | (0x1F & (high << 2 | low >> 6 ) );
		target[1] = 0x80 | (0x3F & low);
	} else 
	{
	}
}

// get following char length
inline int getCodepointLength(char first)
{
	// 1 byte = 
	if( first >> 7 == 0)
	{
		return 1;
	} 
	
	if( first >> 5 == 5)
	{
		return 2;
	}
	// \todo implement more cases
	
	// no starting point
	return 0;
}
