#ifndef _SHA_HW_H_
#define _SHA_HW_H_

#define MAX_TEST_SIZE 32784 // 32k
#define MAX_BUFF_SIZE 32784 // 32k

#pragma SDS data copy(input[0:length], output[0:8])
#pragma SDS data mem_attribute( input:PHYSICAL_CONTIGUOUS , output:PHYSICAL_CONTIGUOUS  )
#pragma SDS data access_pattern( input:SEQUENTIAL, output:SEQUENTIAL )
void sha_hw(unsigned char input[MAX_TEST_SIZE],unsigned int output[8], int length);


#endif