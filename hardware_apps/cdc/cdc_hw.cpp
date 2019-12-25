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
#define DONE_BIT_10 (0x200)

#define DEBUG_CDC

// helps for testing out streaming without any other features built
void output(unsigned char output[4096],hls::stream<unsigned short> &stream)
{
	int done = 0;
	int i = 0;
	unsigned char data;
	unsigned short strm;
	while(!done)
	{
#pragma HLS LOOP_TRIPCOUNT min=64 max=64
#pragma HLS pipeline II=1

		strm = stream.read();

		// extract the bit if it exists
		done = strm & DONE_BIT_10;

		// clear bit
		strm &= ~DONE_BIT_10;

		// store
		output[i] = strm;
		i++;
	}
}

// helps for testing out streaming without any other features built
void cdc_hw_interface(const unsigned char input[4096],hls::stream<unsigned short> &interface_stream_out,unsigned int length)
{
	int i = 0;
	for(i = 0; i < length-1;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=64 max=64
#pragma HLS pipeline II=1
		interface_stream_out.write( (input[i] & ~DONE_BIT_10) );
	}
	interface_stream_out.write((input[i] | DONE_BIT_10) );
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
//void patternSearch(hls::stream<unsigned short> &stream_in,hls::stream<unsigned short> &stream_out,cdc_test_t* cdc_test_check)
void patternSearch(hls::stream<unsigned short> &stream_in,unsigned char raw[4][8196],hls::stream<unsigned short> &stream_out)
{
    // assign the incoming text to our file block
	// window is 16 bits to account for the done bit
    hls::stream<unsigned short> window;
#pragma HLS STREAM variable=window depth=16

    uint64_t polynomial_lookup_buf[256];
    uint64_t prime_table[256];
    uint64_t textHash = 0;
    uint64_t chunks = 0;
    int file_length = RAB_POLYNOMIAL_WIN_SIZE;

#pragma HLS array_partition variable=polynomial_lookup_buf complete dim=1
#pragma HLS array_partition variable=prime_table complete dim=1

    // 
    create_table(polynomial_lookup_buf,prime_table);

    // place first window into the chunk
    for (int j = 0; j < RAB_POLYNOMIAL_WIN_SIZE; j++) {
#pragma HLS pipeline II=1
    	unsigned short in = stream_in.read();
        textHash += (unsigned char)in * polynomial_lookup_buf[(RAB_POLYNOMIAL_WIN_SIZE - 1) - j];
        window.write((unsigned char)in);
        //std::cout << (char)in;
    }
    

    uint16_t new_char = 0;
    uint16_t old_char = 0;
    uint64_t power = 0;
    unsigned short strm_bit = 0;

    uint16_t length = RAB_POLYNOMIAL_WIN_SIZE;
    unsigned short done = 0;

    // iterate through entire length
    hash:while(!done)
    {
#pragma HLS LOOP_TRIPCOUNT min=64 max=64
#pragma HLS pipeline II=1
        // get incoming and outgoing byte
        new_char = stream_in.read();
        file_length++;
        old_char = window.read();
        //std::cout << (char)new_char;
        //raw[chunks][length] = new_char;
        // old_char = window[read];

#ifdef DEBUG
        if(old_char > 255)
        {
        	std::cout << "bit set " << length <<std::endl;
        }
#endif


        // send byte to next app
        stream_out.write(old_char);

        // extract the bit if it exists
        done = new_char & DONE_BIT_10;

        // look in the prime table for value to take away from the hash
        // type cast away the done bit
        power = prime_table[(uint8_t)old_char];


        // calculate roll hash
        textHash *= PRIME;
        textHash += new_char;
        textHash -= power;

        length++;

        // obtain our finger print
        uint64_t finger = textHash ^ FP_POLY;

        // if we have a fingerprint that is larger than our min chunk or we have exceesed length 
        if ((((finger & RAB_BLK_MASK) == 0) || (length == MAX_CHUNK_SIZE)) && (length > MIN_CHUNK_SIZE))
        {

#ifdef DEBUG_CDC
            //std::cout << std::endl << "-----" << std::endl<< "chunk size " << length << std::endl;
            chunks++;
#endif
            length = 0;
            strm_bit = DONE_BIT_9;

        }// found chunk
        else
        {
        	strm_bit = 0;
        }

        // store our new char append the chunk bit
       window.write(new_char | strm_bit | done);

    }// for length


    // flush our window
    flush2:for(int j = 0; j < RAB_POLYNOMIAL_WIN_SIZE; j++)
    {
#pragma HLS pipeline II=1
    	stream_out.write(window.read());
        length++;
    }

    // account last chunk
#ifdef DEBUG_CDC
    chunks++;
    //std::cout << "rem " << length <<std::endl;
    std::cout << "hw chunks found " << chunks << std::endl;
    std::cout << "hw average chunk size " << file_length / chunks << std::endl;
#endif
    //cdc_test_check->avg_chunksize =  length / chunks;
    //cdc_test_check->chunks = chunks;
}

void cdc_top(unsigned char buff[4096],unsigned char outbuff[4096], unsigned int file_length,cdc_test_t* cdc_test_check)
{
	static hls::stream<unsigned short> stream;
#pragma HLS STREAM variable=stream depth=2

	static hls::stream<unsigned short> stream_out;
#pragma HLS STREAM variable=stream depth=2


	// stream data into modules to compute sha256 digest
	// http://www.iwar.org.uk/comsec/resources/cipher/sha256-384-512.pdf
#pragma HLS DATAFLOW
	cdc_hw_interface(buff,stream,file_length);
	//patternSearch(stream,stream_out);
	output(outbuff,stream_out);
}
