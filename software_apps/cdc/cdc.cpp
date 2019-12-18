#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <cmath>
#include <vector>

#define FP_POLY 0xbfe6b8a5bf378d83ULL
#define RAB_POLYNOMIAL_CONST 153191
#define RAB_POLYNOMIAL_WIN_SIZE 16
#define POLY_MASK (0xffffffffffULL)
#define RAB_BLK_MIN_BITS 12
#define RAB_BLK_MASK (((1 << RAB_BLK_MIN_BITS) - 1))
#define PRIME 3
#define MAX_CHUNKS_SIZE 8000
#define MIN_CHUNK_SIZE 2000

class Rabin{

public:
/*
* create a quick look up table to compute the power instead of recalculating every iteration
*/
void create_table()
{
    uint64_t curPower = 1;
    for (int index = 0; index < RAB_POLYNOMIAL_WIN_SIZE + 1; index++)
    {
        polynomial_lookup_buf[index] = curPower;
        curPower *= PRIME;
    }
    // bring power back down for the next table
    curPower /= PRIME;
    // multiply every power by every possible index we can encounter
    for (int index = 0; index < 256; index++)
    {
        prime_table[index] = index * curPower;
    }
}
/*
* given a string iterate through and compute our finger print boundaries
*/
int patternSearch(unsigned char* buff, unsigned int file_length)
{
    // assign the incoming text to our file block
    uint8_t window[RAB_POLYNOMIAL_WIN_SIZE];

    uint64_t textHash = 0;

    // place first window into the chunk
    for (int j = 0; j < RAB_POLYNOMIAL_WIN_SIZE; j++) {
        textHash += buff[j] * polynomial_lookup_buf[(RAB_POLYNOMIAL_WIN_SIZE - 1) - j];
        window[j] = buff[j];
    }
    
    uint8_t evict = 0;
    uint8_t new_char = 0;
    uint8_t old_char = 0;
    uint64_t power = 0;

    uint16_t length = RAB_POLYNOMIAL_WIN_SIZE;

    // iterate through entire length
    for (int i = RAB_POLYNOMIAL_WIN_SIZE; i < file_length; i++)
    {
    	// get incoming and outgoing byte
        new_char = buff[i];
        old_char = window[evict];

        // look in the prime table for value to take away from the hash
        power = prime_table[old_char];

        // store our new char
        window[evict] = new_char;

        // add the char to our chunk
        chunk += old_char;

        //
        evict++;

        // calculate roll hash
        textHash *= PRIME;
        textHash += new_char;
        textHash -= power;
        length++;

        // adjust our moving window
        if (evict == RAB_POLYNOMIAL_WIN_SIZE)
            evict = 0;

        // obtain our finger print
        uint64_t finger = textHash ^ FP_POLY;

        // if we have a fingerprint that is larger than our min chunk or we have exceesed length 
        if ((((finger & RAB_BLK_MASK) == 0) || (length == 8000)) && (length > 2000))
        {
        	length = 0;

        	// flush the rest of our window
            for (int j = i; j < RAB_POLYNOMIAL_WIN_SIZE; j++)
                chunk += buff[j];

            // store the chunk and then clear the chunk
            std::cout << "chunk size " << chunk.size() << std::endl;
            chunks.push_back(chunk);
            chunk.clear();
        }// found chunk
    }// for length

    // flush our window
    for (int j = evict; j < RAB_POLYNOMIAL_WIN_SIZE; j++)
    {
        chunk+= window[j];
    }

    // flush our window
    for (int j = 0; j < evict; j++)
    {   
        chunk+= window[j];
    }

    // save last chunk
    chunks.push_back(chunk);

    chunk.clear();

    std::cout << "chunks found " << chunks.size() << std::endl;
    std::cout << "average chunk size " << file_length / chunks.size() << std::endl;
    return 1;
	}

private:
	uint64_t polynomial_lookup_buf[256];
    std::vector<std::string> chunks;
    std::string chunk;
    uint64_t prime_table[256];
}; // class rabin


void test_cdc_sw( const char* file )
{
    //

    
    //
	FILE* fp = fopen(file,"r" );
	if(fp == NULL ){
		perror("invalid file");
		return;
	}
		
	//
	fseek(fp, 0, SEEK_END); // seek to end of file
	int file_size = ftell(fp); // get current file pointer
	fseek(fp, 0, SEEK_SET); // seek back to beginning of file

	//
	unsigned char* buff = (unsigned char*)(malloc(sizeof(unsigned char) * file_size ));	
	if(buff == NULL)
	{
		perror("not enough space");
		fclose(fp);
		return;
	}	
	
	//	
	int bytes_read = fread(&buff[0],sizeof(unsigned char),file_size,fp);
	printf("bytes_read %d\n",bytes_read);

	Rabin* rks = new Rabin;

    // create table and then perform CDC
    rks->create_table();
    rks->patternSearch(buff,file_size);

    free(buff);
    delete rks;
    return;
}

//
// TODO add some way of more automated testing
//
// int test_cdc()
// {
//     // sw tests
//     test_cdc_sw("file");
//     test_cdc_sw("file");
//     test_cdc_sw("file");
//     test_cdc_sw("file");

//     // hw tests
//     test_cdc_hw("file");
//     test_cdc_hw("file");
//     test_cdc_hw("file");
//     test_cdc_hw("file");
// }