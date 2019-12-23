#ifndef _CDCSW_H_
#define _CDCSW_H_

class Rabin{
	public:
		void create_table();
		int patternSearch(unsigned char* buff, unsigned int file_length,cdc_test_t* cdc_test_check);

	private:
		uint64_t polynomial_lookup_buf[256];
    	std::vector<std::string> chunks;
    	std::string chunk;
    	uint64_t prime_table[256];
}; // class rabin

#endif
