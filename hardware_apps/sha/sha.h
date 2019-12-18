#ifndef SHA_H_
#define SHA_H_

#pragma SDS data copy(input[0:64], output[0:8])
#pragma SDS data mem_attribute( input:PHYSICAL_CONTIGUOUS , output:PHYSICAL_CONTIGUOUS  )
#pragma SDS data access_pattern( input:SEQUENTIAL, output:SEQUENTIAL )
void sha_256(unsigned char input[64],unsigned int output[8], int length);


#endif
