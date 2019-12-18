#include <cstring>
#include <fstream>
#include <iostream>
#include <string.h>
#include <hls_stream.h>
#include <stdlib.h>
#include "sha.h"

#define DEBUG
#define DONE_BIT 0x80

/****************************** MACROS ******************************/
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b)))) 			// roughly 4 cycles
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b)))) 			// rouglhy 4 cycles

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))                   	// rouglhy 4 cycles
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))     	// rouglhy 5 cycles
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22)) 	// rouglhy 16 cycles
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25)) 	// rouglhy 16 cycles
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))       // rouglhy 11 cycles
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))     // roughly 11 cycles


// write back stage
void sha_out(unsigned int output[8], hls::stream<unsigned int> &stream)
{
	for(int i=0; i < 8;i++)
	{
		output[i] = stream.read();
	}
}

// // concatenate chars to ints
// // if we can give a buffer or anything but a stream then we can
// // get a better II 
// void producer(unsigned char input[8192], hls::stream<unsigned int> &sha_stream, int length)
// {
// 	// given 100 this is 32
// 	int remainder = length % 64;
// 	int loops = length / 64;

// 	// we can pump in messages fine.
// 	produce_message:for(int i = 0; i < loops; i++)
// 	{
// #pragma HLS LOOP_TRIPCOUNT min=64/4 max=64/4
// #pragma HLS pipeline II=1
// 		unsigned int message = (input[i] << 24 ) | ( input[i+1] << 16 ) |( input[i+2] << 8 ) | ( input[i+3] );
// 		//unsigned int message =  (data.read() << 24) | (data.read() << 16) | (data.read() << 8) | (data.read() );
// 		sha_stream.write(message);
// 	}

// 	// finish the rest of the data and finish the digest
// 	add_padding:for(int i = 0; i < remainder;i++)
// 	{

// 	}
// }

// helps for testing out streaming without any other features built 
void hw_interface(unsigned char input[64],hls::stream<unsigned char> &data_stream,int length)
{
	int i = 0;
	for(i = 0; i < length-1;i++)
	{
#pragma HLS pipeline II=1
		data_stream.write(input[i]);
	}
	data_stream.write( (input[i] | DONE_BIT) );
}

// goal II = 4
// given an infinite stream we produce SHA keys
void producer(hls::stream<unsigned char> &data_stream, hls::stream<unsigned int> &sha_stream)
{
	// table to see what place we shift our value
	unsigned char shift[4] = {24,16,8,0};

	unsigned int message = 0;
	unsigned char strm_msg = 0;
	unsigned char done = 0;
	unsigned int ctr = 0;
	unsigned int msg_ctr = 0;
	unsigned long long digest_length = 0;
	// we can pump in messages fine.
	produce_message:while(!done)
	{
#pragma HLS LOOP_TRIPCOUNT min=3 max=3
#pragma HLS pipeline II=1

		strm_msg = data_stream.read();

		// extract the bit if it exists
		done = strm_msg & DONE_BIT;

		// clear bit
		strm_msg &= ~DONE_BIT;

		//
		message |= (strm_msg << shift[ctr]);

		// inc
		ctr++;
		// inc
		msg_ctr++;

		//
		digest_length++;

		// write our 32 bit word
		if(ctr == 4)
		{
			sha_stream.write(message);

			#ifdef DEBUG
			printf("msg_p%d: %#04x\n",digest_length,message);
			#endif
			// reset vars
			ctr = 0;
			message = 0;
		}
		if(msg_ctr == 64)
		{
			msg_ctr = 0;
		}
	}

	// finalize
	message |= (0x80 << shift[ctr]);
	ctr++;
	if(ctr == 4)
	{
		sha_stream.write(message);

		#ifdef DEBUG
		printf("msg final: %#04x \n",message);
		#endif
		// reset vars
		ctr = 0;
		message = 0;
	}

	if(msg_ctr < 56){
		// account for here
		msg_ctr++;
		while(msg_ctr < 56)
		{
			ctr++;
			msg_ctr++;
			//
			message |= (0x00 << shift[ctr]);
			if(ctr == 4)
			{
				sha_stream.write(message);

				#ifdef DEBUG
				printf("msg final=%d: %#04x \n",msg_ctr,message);
				#endif
				// reset vars
				ctr = 0;
				message = 0;
			}
		}
	}
	else{
		while(msg_ctr < 64+56)
		{
			ctr++;
			msg_ctr++;
			//
			message |= (0x00 << shift[ctr]);
			if(ctr == 4)
			{
				sha_stream.write(message);

				#ifdef DEBUG
				printf("msg final2=%d: %#04x \n",msg_ctr,message);
				#endif
				// reset vars
				ctr = 0;
				message = 0;
			}
		}
	}

	// finish with our 64 bit length
	digest_length *= 8;
	message = digest_length >> 32;
	sha_stream.write(message);
	message = digest_length & 0xFFFF;
	sha_stream.write(message);
	#ifdef DEBUG
	printf("msg: %#04x \n",message);
	#endif
}

void message_prepare(hls::stream<unsigned int> &data,hls::stream<unsigned int> &message_stream, int messages)
{

	unsigned int i, m[64];
#pragma HLS array_partition variable=m complete dim=1
total_messages:for(int message = 0; message<messages;message++)
{
#pragma HLS LOOP_TRIPCOUNT min=1 max=1
	message_loop_1:for (i = 0; i < 16; ++i)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=2
	// II =4 but if we can pack our data int 42 bit words easily somewhere else we can get 1
// latency per loop = 4 reads, 4 shifts, 3 ors 1 store 12 * 16 = 192 cycles
// should be easily pipelineable
//  write this to message stream
//		unsigned int message =  (data.read() << 24) | (data.read() << 16) | (data.read() << 8) | (data.read() );
		unsigned int message = data.read();
		// write the message into a temp buffer for the second loop
		// stream the message to the update loop
		m[i] = message;
		message_stream.write(message);
	}

	// starting at i = 16 --> 63
	// buff1 sig1 = 14,15,16,17,18,19 --> 61
	// buff2 m[i-7] = 9 --> 56
	// buff3 sig0 = 1 --> 48
	// buff4 m[i-16] = 0 --> 47
	// Data Reuse
	// buff4 = share 1  -- >47 with buff3 share 9 --> 47 with buff2 share 14 --> 47 with buff1
	// buff3 = share 1  --> 47 with buff4 share 9 --> 48 with buff2 share 14 --> 48 with buff1
	// buff2 = share 9  --> 47 wuth buff4 share 9 --> 48 with buff2 share 14 --> 56 with buff2
	// buff1 = share 14 --> 47 with buff4 share 14 --> 48 with buff3 share 9 --> 56 with buff2
	// notice 14 --> 47 shared with 4 modules
	//  sig0 and sig1 take rougly 11 cycles each. This loop costs us about
	// 11 + 11 + 3 + 4 = 29 * 48 = ~1300 cycles
	//
	message_loop_2:for ( ; i < 64; ++i)
	{
#pragma HLS pipeline II=1
//#pragma HLS unroll factor=2
// first iteration uses elements 14:9:1:0 / 15:10:2:1 / 16:11:3:2 / 17:12:4:3 / 18:13:5:4 / 19:14:6:5
		unsigned int message = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
		m[i] = message;
		message_stream.write(message);
	}
}
}


void transform(hls::stream<unsigned int> &message_stream, unsigned int state[8], hls::stream<unsigned int> &out_stream, int messages)
{
	const unsigned int k[64] = 
	{
				0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
				0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
				0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
				0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
				0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
				0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
				0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
				0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
	};
#pragma HLS array_partition variable=k complete dim=1

	unsigned int a, b, c, d, e, f, g, h, i;
	static hls::stream<unsigned int> t1_stream;
#pragma HLS STREAM variable=t1_stream depth=1
	static hls::stream<unsigned int> t2_stream;
#pragma HLS STREAM variable=t2_stream depth=1

transforms_iters:for(int m = 0; m < messages; m++)
{
#pragma HLS LOOP_TRIPCOUNT min=1 max=1
	// potentially hide the latency by adding another array
	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];
	f = state[5];
	g = state[6];
	h = state[7];

	// temp_a = state[0]
	// temp_e = state[4]
	//
	// our main bottle neck is this loop
	// coming in at about 3456 cycles per 64 bytes
	//
	update_loop:for (i = 0; i < 64; ++i)
	{
#pragma HLS pipeline II=1
		// t1 = 3 adds + 3 loads + 21 = 27 cycles
		// ep1 ~= 16 cycles  ch = 4
		// ep0 ~= 16 cycles  MAJ = 5
		// t1 = 23 cycles
		// t2 = 21 cycles
		// reassignments ~= 10
		// total = 54 * 64 = 3456 cycles
		unsigned int t1 = h + EP1(e) + CH(e,f,g) + k[i] + message_stream.read();
		unsigned int t2  = EP0(a) + MAJ(a,b,c);

		// for --> 0 64
		// while t1 is being computed we can do the rest of the load store computations
		//
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;

#ifdef DEBUG
		printf("t=%d ",i);
		printf("a: %#08x ",a);
		printf("b: %#08x ",b);
		printf("c: %#08x ",c);
		printf("d: %#08x ",d);
		printf("e: %#08x ",e);
		printf("f: %#08x ",f);
		printf("g: %#08x ",g);
		printf("h: %#08x\n",h);
#endif

	}// loop
// can pipeline or unroll this loop as well for a benefit of 8 or 16 instead of ~24
	if(m == messages-1)
	{
#ifdef DEBUG
	printf("writing output\n");
#endif
		out_stream.write(state[0] += a);
		out_stream.write(state[1] += b);
		out_stream.write(state[2] += c);
		out_stream.write(state[3] += d);
		out_stream.write(state[4] += e);
		out_stream.write(state[5] += f);
		out_stream.write(state[6] += g);
		out_stream.write(state[7] += h);
	}
	else
	{
		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
		state[4] += e;
		state[5] += f;
		state[6] += g;
		state[7] += h;
	}
}// messages

}// transform


void sha_256(unsigned char input[64],unsigned int output[8], int length){

	unsigned int state[8];
	#pragma HLS array_partition variable=state complete dim=1

	// initial hashes to begin with
	// rest are hash-1 we need to eventually reset this
	state[0] = 0x6a09e667;
	state[1] = 0xbb67ae85;
	state[2] = 0x3c6ef372;
	state[3] = 0xa54ff53a;
	state[4] = 0x510e527f;
	state[5] = 0x9b05688c;
	state[6] = 0x1f83d9ab;
	state[7] = 0x5be0cd19;

	static hls::stream<unsigned char> data_stream;
#pragma HLS STREAM variable=sha_stream depth=2
	static hls::stream<unsigned int> sha_stream;
#pragma HLS STREAM variable=sha_stream depth=2
	static hls::stream<unsigned int> message_stream;
#pragma HLS STREAM variable=sha_stream depth=2
	static hls::stream<unsigned int> out_state;
#pragma HLS STREAM variable=sha_stream depth=1
	// prepares a message ever 64 bytes
	// pad the message in the usual way.
	// for example "abc" has length 24 so it is padded with a one
	// then 448-(24+1) = 423 zeros and then its length to become 512 bit padded msg
	// 001100001 01100010 01100011 1 0->0 (423)  (64) 11000
	//
	//http://www.iwar.org.uk/comsec/resources/cipher/sha256-384-512.pdf
#pragma HLS DATAFLOW
	hw_interface(input,data_stream,56);
	producer(data_stream,sha_stream);
	message_prepare(sha_stream,message_stream,length/64);
	transform(message_stream,&state[0],out_state,length/64);
	sha_out(&output[0],out_state);
}
