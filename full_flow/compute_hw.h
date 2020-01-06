 #ifndef _COMPUTE_H_
 #define _COMPUTE_H_

// #define EOF_BIT 1 << 10
// #define NUM_INSTANCES 2
// #define END_TRANSFER_INSTANCE_1 1 << 8
// #define END_TRANSFER_INSTANCE_2 2 << 8
// #define END_TRANSFER_ALL 3 << 8

// // max number of elements we can get from ethernet
// #define MAX_CHUNKSIZE 8192


 #pragma SDS data copy(input[0:length], output[0:key*8])
 #pragma SDS data mem_attribute( input:PHYSICAL_CONTIGUOUS , output:PHYSICAL_CONTIGUOUS  )
 #pragma SDS data access_pattern( input:SEQUENTIAL, output:SEQUENTIAL )
 void compute_hw( unsigned char* input, unsigned int* output,unsigned int length,unsigned int key);

 #endif
