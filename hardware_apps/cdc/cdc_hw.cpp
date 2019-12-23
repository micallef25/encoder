#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cdc_hw.h"
#include <hls_stream.h>


#define FP_POLY 0xbfe6b8a5bf378d83ULL
#define RAB_POLYNOMIAL_CONST 153191
#define RAB_POLYNOMIAL_WIN_SIZE 16
#define POLY_MASK (0xffffffffffULL)
#define RAB_BLK_MIN_BITS 12
#define RAB_BLK_MASK (((1 << RAB_BLK_MIN_BITS) - 1))
#define PRIME 3
#define MAX_CHUNK_SIZE 8192
#define MIN_CHUNK_SIZE 2048
#define DONE_BIT_9 (0x100)

// helps for testing out streaming without any other features built
void cdc_hw_interface(unsigned char input[4096],hls::stream<unsigned short> &interface_stream_out,int length)
{
	int i = 0;
	for(i = 0; i < length-1;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=64 max=64
#pragma HLS pipeline II=1
		interface_stream_out.write( (input[i] & ~DONE_BIT_9) );
	}
	interface_stream_out.write((input[i] | DONE_BIT_9) );
}

/*
* create a quick look up table to compute the power instead of recalculating every iteration
*/
void create_table(uint64_t polynomial_lookup_buf[256],uint64_t prime_table[256])
{
    uint64_t curPower = 1;
    for (int index = 0; index < RAB_POLYNOMIAL_WIN_SIZE + 1; index++)
    {
#pragma HLS pipeline II=1        
        polynomial_lookup_buf[index] = curPower;
        curPower *= PRIME;
    }
    // bring power back down for the next table
    curPower /= PRIME;
    // multiply every power by every possible index we can encounter
    for (int index = 0; index < 256; index++)
    {
#pragma HLS pipeline II=1
        prime_table[index] = index * curPower;
    }
}
/*
* given a string iterate through and compute our finger print boundaries
* might be able to exploit 122 bit packing rw here depending on speed up
*/
void patternSearch(hls::stream<unsigned short> &stream_in,cdc_test_t* cdc_test_check)
{
    // assign the incoming text to our file block
    uint8_t window[RAB_POLYNOMIAL_WIN_SIZE];
    uint64_t polynomial_lookup_buf[256];
    uint64_t prime_table[256];
    uint64_t textHash = 0;
    uint8_t chunk_buff[MAX_CHUNK_SIZE];
    uint64_t chunks = 0;
    int file_length = 0;
#pragma HLS array_partition variable=window complete dim=1
#pragma HLS array_partition variable=polynomial_lookup_buf complete dim=1
#pragma HLS array_partition variable=prime_table complete dim=1

    // 
    create_table(polynomial_lookup_buf,prime_table);

    // place first window into the chunk
    for (int j = 0; j < RAB_POLYNOMIAL_WIN_SIZE; j++) {
#pragma HLS pipeline II=1
    	unsigned short in = stream_in.read();
        textHash += (unsigned char)in * polynomial_lookup_buf[(RAB_POLYNOMIAL_WIN_SIZE - 1) - j];
        window[j] = (unsigned char)in;
    }
    
    uint8_t evict = 0;
    uint16_t new_char = 0;
    uint8_t old_char = 0;
    uint64_t power = 0;

    uint16_t length = RAB_POLYNOMIAL_WIN_SIZE;
    unsigned short done = 0;

    // iterate through entire length
//    hash:for (int i = RAB_POLYNOMIAL_WIN_SIZE; i < file_length; i++)
    hash:while(!done)
    {
#pragma HLS LOOP_TRIPCOUNT min=64 max=64
#pragma HLS pipeline II=1
        // get incoming and outgoing byte
        new_char = stream_in.read();
        old_char = window[evict];

        // extract the bit if it exists
        done = new_char & DONE_BIT_9;

        // clear bit
        new_char &= ~DONE_BIT_9;

        // look in the prime table for value to take away from the hash
        power = prime_table[old_char];

        // store our new char
        window[evict] = new_char;

        // send byte to next app
        //stream.write(old_char);

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
        if ((((finger & RAB_BLK_MASK) == 0) || (length == MAX_CHUNK_SIZE)) && (length > MIN_CHUNK_SIZE))
        {
        	// flush the rest of our window
            //for (int j = i; j < RAB_POLYNOMIAL_WIN_SIZE; j++)
            //    stream.write(buff[j]);

            // store the chunk and then clear the chunk
            //std::cout << "chunk size " << length << std::endl;
            length = 0;
            chunks++;
        }// found chunk
    }// for length

    // flush our window
//    flush1:for(int j = evict; j < RAB_POLYNOMIAL_WIN_SIZE; j++)
//    {
//#pragma HLS pipeline II=1
//        stream.write(window[j]);
//    }
//
//    // flush our window
//    flush2:for(int j = 0; j < evict; j++)
//    {
//#pragma HLS pipeline II=1
//        stream.write(window[j]);
//    }

    // account last chunk
    chunks++;

    std::cout << "hw chunks found " << chunks << std::endl;
    std::cout << "hw average chunk size " << file_length / chunks << std::endl;
    cdc_test_check->avg_chunksize =  file_length / chunks;
    cdc_test_check->chunks = chunks;
}

void cdc_top(unsigned char buff[4096], unsigned int file_length,cdc_test_t* cdc_test_check)
{
	static hls::stream<unsigned short> stream;
#pragma HLS STREAM variable=stream depth=2


	// stream data into modules to compute sha256 digest
	// http://www.iwar.org.uk/comsec/resources/cipher/sha256-384-512.pdf
#pragma HLS DATAFLOW
	cdc_hw_interface(buff,stream,file_length);
	patternSearch(stream,cdc_test_check);
	//output(hash_stream,outbuff);
}
