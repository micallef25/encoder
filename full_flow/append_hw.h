// #ifndef _APPEND_H_
// #define _APPEND_H_

// #include "compute_hw.h"

// // consider what other pragmas ay be useful for your assignment
// // perhaps over riding the compiler chosen DMA channel is in your best interest?
// // look here: https://www.xilinx.com/html_docs/xilinx2019_1/sdsoc_doc/sds-pragmas-nmc1504034362475.html#dzz1504034363418
// // we looked at data motion reports in homework 6 what did you observe or notice?
// #pragma SDS data copy(input[0:length], output[0:length])
// #pragma SDS data mem_attribute( input:PHYSICAL_CONTIGUOUS , output:PHYSICAL_CONTIGUOUS  )
// #pragma SDS data access_pattern( input:SEQUENTIAL, output:SEQUENTIAL )
// void append_hw( const unsigned char input[MAX_CHUNKSIZE], unsigned char output[MAX_CHUNKSIZE],unsigned int length);

// #endif
