#include <cstring>
#include <fstream>
#include <iostream>
#include <string.h>
#include <hls_stream.h>
#include <stdlib.h>
#include "sha.h"
#include <ap_int.h>

 #define DEBUG


// #define DONE_BIT_9 (1 << 8)
#define DONE_BIT_9 (0x100)
#define DONE_BIT_10 (0x200)
// for some reason 1 << 32 does not work... some one please tell me why
//#define DONE_BIT_33 (1 << 32)
#define DONE_BIT_33 (0x100000000)
#define DONE_BIT_34 (0x200000000)

/****************************** TYPES ******************************/
// for some reason making it ap int is not  working
// FIFO LUTS go up about 1% if we use primitive types instead of custom
 typedef unsigned long long uint33_t;
 typedef unsigned short uint10_t;
//typedef ap_uint<33> uint33_t;
//typedef ap_uint<10> uint10_t;

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
void sha_out(unsigned int output[4*8], hls::stream<uint33_t> &out_stream)
{
	uint33_t done = 0;
	uint8_t k = 0;
	while(!done){
#pragma HLS LOOP_TRIPCOUNT min=1 max=1
	for(int i=0; i < 8;i++)
	{
		output[i+(k*8)] = out_stream.read();
	}
	done = out_stream.read();
	k++;
	#ifdef DEBUG
	std::cout << done <<std::endl;
	#endif
	}
}


// helps for testing out streaming without any other features built 
void hw_interface(unsigned char input[MAX_BUFF_SIZE],hls::stream<uint10_t> &interface_stream_out,int length)
{
	int i = 0;
	for(i = 0; i < length-1;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=3 max=3
#pragma HLS pipeline II=1
		interface_stream_out.write( (input[i] & ~DONE_BIT_10) );
	}
	interface_stream_out.write( (input[i] | DONE_BIT_10) );
}

/*
*
*/
void producer(hls::stream<unsigned short> &producer_stream_in, hls::stream<uint33_t> &producer_stream_out)
{
	// table to see what place we shift our value
	unsigned char shift[4] = {24,16,8,0};

	uint33_t message = 0;
	uint10_t strm_msg = 0;
	uint10_t done = 0;
	uint10_t end = 0;
	unsigned int ctr = 0;
	unsigned int msg_ctr = 0;
	unsigned long long digest_length = 0;
	unsigned long long bread= 0;

	// we can pump in messages fine.
	// change the granularity maybe we can read in 16 bits instead of 8
	while(!end){
#pragma HLS LOOP_TRIPCOUNT min=1 max=1
	produce_message:while(!done && !end)
	{
#pragma HLS LOOP_TRIPCOUNT min=3 max=3
#pragma HLS pipeline II=1

		strm_msg = producer_stream_in.read();
		digest_length++;
		bread++;

		// extract the bit if it exists
		done = strm_msg & DONE_BIT_9;

#ifdef DEBUG
// std::cout << (char)strm_msg;
if(done != 0)
// printf("-----------\n");
printf("done bit found %d\n",digest_length);
#endif

		// clear bit
		strm_msg &= ~DONE_BIT_9;

		// extract the bit if it exists
		end = strm_msg & DONE_BIT_10;

#ifdef DEBUG
if(end != 0)
printf("end bit found %d\n",digest_length);
#endif

		// clear bit
		strm_msg &= ~DONE_BIT_10;

		//
		message |= (strm_msg << shift[ctr]);

		// inc
		ctr++;
		msg_ctr++;
		//digest_length++;

		// write our 32 bit word
		if(ctr == 4)
		{
			message &= ~DONE_BIT_33;
			message &= ~DONE_BIT_34;
			producer_stream_out.write(message);

			// #ifdef DEBUG
			// printf("msg_p%d: %#08x\n",digest_length,(unsigned int)message);
			// #endif

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
		message &= ~DONE_BIT_33;
		message &= ~DONE_BIT_34;
		producer_stream_out.write(message);

		// #ifdef DEBUG
		// printf("msg final: %#08x \n",(unsigned int)message);
		// #endif

		// reset vars
		ctr = 0;
		message = 0;
	}

	#ifdef DEBUG
	printf("msg ctr: %d \n",(unsigned int)msg_ctr);
	#endif

	// optimize this if else
	if(msg_ctr < 56){
		msg_ctr++;
		while(msg_ctr < 56)
		{
#pragma HLS LOOP_TRIPCOUNT min=53 max=53
#pragma HLS pipeline II=1

			ctr++;
			msg_ctr++;

			//
			message |= (0x00 << shift[ctr]);
			if(ctr == 4)
			{
				message &= ~DONE_BIT_33;
				message &= ~DONE_BIT_34;
				producer_stream_out.write(message);

				// #ifdef DEBUG
				// printf("msg final=%d: %#04x \n",msg_ctr,(unsigned int)message);
				// #endif
				// reset vars
				ctr = 0;
				message = 0;
			}
		}
	}
	else{
		msg_ctr++;
		while(msg_ctr < 64+56)
		{
#pragma HLS LOOP_TRIPCOUNT min=53 max=53
#pragma HLS pipeline II=1

			ctr++;
			msg_ctr++;

			//
			message |= (0x00 << shift[ctr]);
			if(ctr == 4)
			{
				message &= ~DONE_BIT_33;
				message &= ~DONE_BIT_34;
				producer_stream_out.write(message);

				// #ifdef DEBUG
				// printf("msg final2=%d: %#04x \n",msg_ctr,(unsigned int)message);
				// #endif

				// reset vars
				ctr = 0;
				message = 0;
			}
		}
	}

	// finish with our 64 bit length
	digest_length *= 8;
	message = digest_length >> 32;
	message &= ~DONE_BIT_33;
	message &= ~DONE_BIT_34;


//	#ifdef DEBUG
//	printf("msg: %#08x \n",(unsigned int)message);
//	#endif

	// write the lengths of the stream
	producer_stream_out.write(message);

	message = (digest_length & 0xFFFFFFFF) | DONE_BIT_33 ;

	if(end != 0)
		message |= DONE_BIT_34;
	else
		message &= ~DONE_BIT_34;

	producer_stream_out.write( message );
	done = 0;
	digest_length = 0;
	msg_ctr = 0;
	message = 0;
	ctr = 0;
	#ifdef DEBUG
	printf("-----------\n");
	#endif
}

	 #ifdef DEBUG
	 printf("msg: %d \n",bread);
	 #endif

}

void message_prepare(hls::stream<uint33_t> &message_strm_in,hls::stream<uint33_t> &message_stream_out)
{

	unsigned int i, m[64];
	
	uint33_t done = 0;
	uint33_t strm_msg = 0;
	uint33_t end = 0;

#pragma HLS array_partition variable=m complete dim=1
while(!end){
#pragma HLS LOOP_TRIPCOUNT min=1 max=1
while(!done && !end)
{
#pragma HLS LOOP_TRIPCOUNT min=1 max=1
	message_loop_1:for (i = 0; i < 16; ++i)
	{
#pragma HLS pipeline II=1
//#pragma HLS unroll factor=2
		// with a message size of 32 bits we can achieve an II of 1
		strm_msg =  message_strm_in.read();

		// extract the bit if it exists
		done = strm_msg & DONE_BIT_33;

		// clear the bit so it does not propagate early
		strm_msg &= ~DONE_BIT_33;

		// extract the bit if it exists
		end = strm_msg & DONE_BIT_34;

		// clear the bit so it does not propagate early
		strm_msg &= ~DONE_BIT_34;

//		#ifdef DEBUG
//		if(done != 0 )
//		printf("done %d =%llu\n",i,(unsigned long long)done);
//		if(end != 0 )
//		printf("end %d =%llu\n",i,(unsigned long long)end);
//		#endif

		// write the message into a temp buffer for the second loop
		// stream the message to the update loop
		m[i] = (unsigned int)strm_msg;

		//
		message_stream_out.write(strm_msg);
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
	message_loop_2:for ( ; i < 63; ++i)
	{
// TODo if we have one instance we can pipelin else we have to unroll
//#pragma HLS unroll factor=2
 #pragma HLS pipeline II=1
		unsigned int message = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
		m[i] = message;

		// if last then set done bit if it was encountered
		message_stream_out.write(message);
	}

// save a mux by no if else
	unsigned int message = SIG1(m[63 - 2]) + m[63 - 7] + SIG0(m[63 - 15]) + m[i - 16];
	message_stream_out.write(message | done | end);
	done = 0;
}
}
}

/*
*
*
*/
void transform(hls::stream<uint33_t> &transform_stream_in, unsigned int state[8], hls::stream<uint33_t> &transform_strm_out)
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
	
	uint33_t done = 0;
	uint33_t strm_msg = 0;
	unsigned int transforms=0;
	uint33_t end = 0;

	// done bit is set when the last byte arrived
while(!end){
#pragma HLS LOOP_TRIPCOUNT min=1 max=1
	transform_iters:while(!done && !end)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=1

		// potentially hide the latency by adding another array
		// and unrolling
		a = state[0];
		b = state[1];
		c = state[2];
		d = state[3];
		e = state[4];
		f = state[5];
		g = state[6];
		h = state[7];


		//
		// our main bottle neck is this loop
		// coming in at about 3456 cycles per 64 bytes
		//
		update_loop:for (i = 0; i < 64; ++i)
		{
	#pragma HLS pipeline II=1

			// read msg from the message stage
			strm_msg =  transform_stream_in.read();

			// extract the bit if it exists
			done = strm_msg & DONE_BIT_33;

			#ifdef DEBUG
			if(done != 0)
			printf("done %d tran=%llu \n",i,(unsigned long long)done);
			#endif

			// clear bit
			strm_msg &= ~DONE_BIT_33;

			// extract the bit if it exists
			end = strm_msg & DONE_BIT_34;

			// clear the bit so it does not propagate early
			strm_msg &= ~DONE_BIT_34;

			//compute t1,t2
			unsigned int t1 = h + EP1(e) + CH(e,f,g) + k[i] + (unsigned int)strm_msg;
			unsigned int t2  = EP0(a) + MAJ(a,b,c);

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

	// #ifdef DEBUG
	// 		printf("t=%d ",i);
	// 		printf("a: %#08x ",a);
	// 		printf("b: %#08x ",b);
	// 		printf("c: %#08x ",c);
	// 		printf("d: %#08x ",d);
	// 		printf("e: %#08x ",e);
	// 		printf("f: %#08x ",f);
	// 		printf("g: %#08x ",g);
	// 		printf("h: %#08x\n",h);
	// #endif

		}// loop

		// reset state array here probably
		if(done || end)
		{
	#ifdef DEBUG
		printf("writing output %d \n",transforms);
	#endif
			transform_strm_out.write(state[0] += a);
			transform_strm_out.write(state[1] += b);
			transform_strm_out.write(state[2] += c);
			transform_strm_out.write(state[3] += d);
			transform_strm_out.write(state[4] += e);
			transform_strm_out.write(state[5] += f);
			transform_strm_out.write(state[6] += g);
			transform_strm_out.write(state[7] += h);
			transform_strm_out.write(end);
			state[0] = 0x6a09e667;
			state[1] = 0xbb67ae85;
			state[2] = 0x3c6ef372;
			state[3] = 0xa54ff53a;
			state[4] = 0x510e527f;
			state[5] = 0x9b05688c;
			state[6] = 0x1f83d9ab;
			state[7] = 0x5be0cd19;
			done = 0;
			transforms = 0;
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
			transforms++;
		}
	}// while
}
}// transform


/*
* given a string produce a sha256 bit hash
*/
void sha_hw(unsigned char input[MAX_BUFF_SIZE],unsigned int output[8], int length){

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

	static hls::stream<uint10_t> data_stream;
#pragma HLS STREAM variable=data_stream depth=2

	static hls::stream<uint33_t> sha_stream;
#pragma HLS STREAM variable=sha_stream depth=2

	static hls::stream<uint33_t> message_stream;
#pragma HLS STREAM variable=message_stream depth=2

	static hls::stream<uint33_t> out_state;
#pragma HLS STREAM variable=out_state depth=1


	// stream data into modules to compute sha256 digest
	// http://www.iwar.org.uk/comsec/resources/cipher/sha256-384-512.pdf
#pragma HLS DATAFLOW
	hw_interface(input,data_stream,length);
	producer(data_stream,sha_stream);
	message_prepare(sha_stream,message_stream);
	transform(message_stream,&state[0],out_state);
	//sha_out(&output[0],out_state);
}


/*
* given a string produce a sha256 bit hash
*/
void sha_hw_stream(hls::stream<unsigned short> &data_stream,unsigned int output[8*4]){

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


	static hls::stream<uint33_t> sha_stream;
#pragma HLS STREAM variable=sha_stream depth=2

	static hls::stream<uint33_t> message_stream;
#pragma HLS STREAM variable=message_stream depth=2

	static hls::stream<uint33_t> out_state;
#pragma HLS STREAM variable=out_state depth=1


	// stream data into modules to compute sha256 digest
	// http://www.iwar.org.uk/comsec/resources/cipher/sha256-384-512.pdf
#pragma HLS DATAFLOW
	producer(data_stream,sha_stream);
	message_prepare(sha_stream,message_stream);
	transform(message_stream,&state[0],out_state);
	sha_out(&output[0],out_state);
}
