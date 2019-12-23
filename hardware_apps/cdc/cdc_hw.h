#ifndef _CDCHW_H_
#define _CDCHW_H_

#include "../../testbenches/cdc/cdc_testbench.h"

int patternSearch(unsigned char* buff, unsigned int file_length,cdc_test_t* cdc_test_check);
void create_table(uint64_t polynomial_lookup_buf[256],uint64_t prime_table[256]);

#endif
