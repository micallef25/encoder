#ifndef _CDCHW_H_
#define _CDCHW_H_

#include <hls_stream.h>
#include "../../testbenches/cdc/cdc_testbench.h"

void cdc_hw_interface(const unsigned char input[4096],hls::stream<unsigned short> &interface_stream_out,unsigned int length);
void cdc_top(unsigned char buff[4096],unsigned char outbuff[4096], unsigned int file_length,cdc_test_t* cdc_test_check);
void create_table(uint64_t polynomial_lookup_buf[256],uint64_t prime_table[256]);
void patternSearch(hls::stream<unsigned short> &stream_in,unsigned char raw[4][8196],hls::stream<unsigned short> &stream_out);

#endif
