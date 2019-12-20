// #include "app3.h"


// void application3_hw( hls::stream<unsigned short> in_stream[NUM_INSTANCES], unsigned char out_buff[MAX_CHUNKSIZE], unsigned short done_bit )
// {
// 	unsigned char index = 0;
// 	unsigned short in_data = 0;
// 	unsigned short done_flag = 0;
// 	uint64_t i =0;
// 	unsigned short length = 0;
// 	unsigned short local_buffer[MAX_CHUNKSIZE];
// //	long test = 0;

// 	// think of how static variables or static tables can help you build your LZW dictionary if a file is broken into many packets
// 	//
// 	static unsigned short debug_ctr = 0;

// 	// new pragma but what does it do? How is this helpful?
// 	while( done_flag != done_bit  )
// 	{
// //	#pragma HLS LOOP_TRIPCOUNT min=256 max=256
// //	#pragma HLS pipeline II=1
// 		// read from DMA stream
// 		in_data = in_stream[index].read();

// 		//read data from other queue. We can maintain order this way
// 		index = (index == (NUM_INSTANCES-1) ) ? 0 : index+1;

// 		// extract our flag
// 		done_flag |= in_data & done_bit;

// 		// if actually data we will write it to the output buffer
// 		if( (done_flag != END_TRANSFER_INSTANCE_1) && (done_flag != END_TRANSFER_INSTANCE_2) && (done_flag != done_bit) )
// 		{
// 			//in_data = (in_data == EOF_BIT) ? debug_ctr : in_data;

// 			// write to application stream
// 			local_buffer[i] = in_data;
// 			// index to buffer write
// 			i++;
// 		}

// 		//
// 		length++;

// 		// same concept as in app2.cpp
// 		debug_ctr++;
// 	}
// 	//i--;
// 	char high = i >> 8;
// 	char low = i & 0xFF;

// 	out_buff[0] = low;
// 	out_buff[1] = high;

// 	for(i = 2; i < length; i++ )
// 	{
// #pragma HLS pipeline II=1
// 		out_buff[i] = local_buffer[i-2];
// 	}


// }
