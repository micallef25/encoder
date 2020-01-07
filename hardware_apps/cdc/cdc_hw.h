#ifndef _CDCHW_H_
#define _CDCHW_H_

#include <hls_stream.h>
#include "../../testbenches/cdc/cdc_testbench.h"


#pragma SDS data copy(buff[0:file_length], outbuff[0:key])
#pragma SDS data mem_attribute( buff:PHYSICAL_CONTIGUOUS , outbuff:PHYSICAL_CONTIGUOUS  )
#pragma SDS data access_pattern( buff:SEQUENTIAL, outbuff:SEQUENTIAL )
void cdc_top(unsigned char* buff,unsigned char* outbuff, unsigned int file_length,unsigned int key);

void cdc_hw_interface(const unsigned char* input,hls::stream<unsigned short> &interface_stream_out,unsigned int length);
void create_table(uint64_t polynomial_lookup_buf[256],uint64_t prime_table[256]);
void patternSearch(hls::stream<unsigned short> &stream_in,hls::stream<unsigned short> &stream_out);

#endif
