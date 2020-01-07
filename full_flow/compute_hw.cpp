#include "compute_hw.h"
#include "../common/sds_utils.h"
#include "../hardware_apps/cdc/cdc_hw.h"
#include "../hardware_apps/sha/sha.h"

//#define DONE_BIT_10 (0x200)

void dma_out(hls::stream<unsigned long long> &out_stream, unsigned int* output,unsigned int key)
{
	unsigned long long done = 0;
	unsigned int k = 0;
	end_loop:while(!done)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=1
		for(unsigned int i=0; i < 8;i++)
		{
#pragma HLS pipeline II=1
			output[i+(k*8)] = out_stream.read();
		}
	done = out_stream.read();
	k++;
	//#ifdef DEBUG
	//std::cout << done <<std::endl;
	//#endif
	}
	for(; k < key; k++)
	{
		for(unsigned int i=0; i < 8;i++)
		{
		#pragma HLS pipeline II=1
				output[i+(k*8)] = 0x00;
		}
	}
}

void dma_in(unsigned char* input,hls::stream<unsigned short> &interface_stream_out,unsigned int length)
{
	unsigned int i = 0;
	for(i = 0; i < length-1;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=64 max=64
#pragma HLS pipeline II=1
		interface_stream_out.write( (input[i] & ~(0x200)) );
	}
	interface_stream_out.write((input[i] | (0x200)) );
}

 // our top level function DMA is streamed in and out see header for declaration
 //void compute_hw( const unsigned char input[4096], unsigned int output[8*30000],unsigned int length)
 void compute_hw( unsigned char* input, unsigned int* output,unsigned int length, unsigned int key)
 {
 	hls::stream<unsigned short> data_stream;
 	hls::stream<unsigned short> stream_out;
 	hls::stream<unsigned long long> write_stream;

 //
 #pragma HLS STREAM variable=data_stream depth=4096
 #pragma HLS STREAM variable=stream_out depth=4096
 #pragma HLS STREAM variable=write_stream depth=4096

 // consider why the data flow pragma may be useful we did this in homework 7
 #pragma HLS DATAFLOW
 	// hw interface is meant for possibly sending 128bits
 	dma_in(input,data_stream,length);
 	patternSearch(data_stream,stream_out);
 	sha_hw_stream(stream_out,write_stream);
 	dma_out(write_stream,output,key);
 }
