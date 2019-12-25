//include "compute_hw.h"
#include "../common/sds_utils.h"
#include "../hardware_apps/cdc/cdc_hw.h"
#include "../hardware_apps/sha/sha.h"

 // our top level function DMA is streamed in and out see header for declaration
 void compute_hw( const unsigned char input[4096], unsigned int output[8*30000],unsigned int length)
 {
 	static hls::stream<unsigned short> data_stream;
 	static hls::stream<unsigned short> stream_out;

 //
 #pragma HLS STREAM variable=data_stream depth=2
 #pragma HLS STREAM variable=stream_out depth=2

 // consider why the data flow pragma may be useful we did this in homework 7
 #pragma HLS DATAFLOW
 	// hw interface is meant for possibly sending 128bits
 	cdc_hw_interface(input,data_stream,length);
 	patternSearch(data_stream,stream_out);
 	sha_hw_stream(stream_out,output);

 }
