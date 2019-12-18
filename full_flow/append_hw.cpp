#include "append_hw.h"

// our top level function DMA is streamed in and out see header for declaration
void append_hw( const unsigned char input[MAX_CHUNKSIZE], unsigned char output[MAX_CHUNKSIZE],unsigned int length)
{
	for(int i = 0; i < length; i++ )
	{
#pragma HLS pipeline II=1
		output[i] = input[i];
	}

}
