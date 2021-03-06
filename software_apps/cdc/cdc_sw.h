#ifndef _CDCSW_H_
#define _CDCSW_H_


#include <vector>
#include <stdint.h>
#include <string>
#include "../../testbenches/cdc/cdc_testbench.h"


class Rabin{
	public:
		void create_table();
		int patternSearch(unsigned char* buff, unsigned int file_length,cdc_test_t* cdc_test_check);
		std::vector<std::string> chunks;

	private:
		uint64_t polynomial_lookup_buf[256];
    	std::string chunk;
    	uint64_t prime_table[256];
}; // class rabin

#endif
