// #include "compute_hw.h"
// #include "append_hw.h"
// #include "utils.h"
// #include <stdio.h>
// #include <stdint.h>
// #include <stdlib.h>
// #include <string.h>
// #include <iostream>
// #include "server.h"
// #include "sha.h"

// #include "cdc.h"

// #define RING_SIZE 8
// #define pipe_depth 4
// #define DEFAULT_CHUNKSIZE 2048
// #define DONE_BIT_L (1 << 7)
// #define DONE_BIT_H (1 << 15)


// #ifdef __SDSCC__
// #include <sds_lib.h>
// #include "sds_utils.h"
// #include <sys/mman.h>

// typedef long long int u64;
// unsigned char* ocm8_vptr;
// unsigned char* sds_map;
// int ocm_fd;


// int init_ocm()
// {
//   unsigned int ocm_size = 0x40000;
//   off_t ocm_pbase = 0xFFFC0000; // physical base address
//   ocm8_vptr = NULL;


//   // Map the OCM physical address into user space getting a virtual address for it
//   if ((ocm_fd = open("/dev/mem", O_RDWR | O_SYNC)) != -1) {
// 	   //if (( ocm8_vptr = (unsigned char*)mmap((void*)ocm_pbase, ocm_size, PROT_READ|PROT_WRITE, MAP_SHARED, ocm_fd, 0)) == NULL)
// 	   if (( ocm8_vptr = (unsigned char*)mmap(NULL, ocm_size, PROT_READ|PROT_WRITE, MAP_SHARED, ocm_fd, ocm_pbase)) == NULL)
// 	   {
// 		   printf("mmap failed\n");
// 	   }
// 	   sds_map = (unsigned char*)sds_mmap((void*)ocm_pbase, ocm_size, (void*)ocm8_vptr);

// 	   printf("%p\n",sds_map);
// 	   printf("%p\n",ocm8_vptr);
//      // Write to the memory that was mapped, use devmem from the command line of Linux to verify it worked
//      // it could be read back here also

// 	   //ocm64_vptr[0] = 0xDEADBEEFFACEB00C;
//     // close(fd);
//   }
//   return 0;
// }

// #endif




// int encoder(int argc, char* argv[])
// {
// 	// ring and file buffers
// 	unsigned char* input[RING_SIZE];
// 	unsigned char* output[RING_SIZE];
// 	unsigned char* file;

// 	// ring buffer position
// 	int writer = 0;
// 	int reader = 0;
// 	int done = 0;

// 	//
// 	int compressed_length = 0;
// 	int compressed_offset = 0;

// 	int offset =0;
// 	int length = 0;

// 	ESE532_Server server;
// 	int appends = 0;

// 	// default is 2k
// 	int chunksize = DEFAULT_CHUNKSIZE;

// 	// set chunksize if decalred through command line
// 	handle_input(argc,argv,&chunksize);

// 	//
// 	if (server.setup_server( chunksize ) == -1)
// 	{
// 		std::cout << "error setting up server " << std::endl;
// 		return 1;
// 	}


// 	// allocate our big file buffer
// 	file = Allocate( sizeof(unsigned char)* MAX_INPUT_SIZE );


// 	// Allocate our smaller ring buffers
// 	for(int i = 0; i < RING_SIZE; i++)
// 	{
// 		input[i] = (unsigned char*)Allocate( sizeof(unsigned char)* (MAX_CHUNKSIZE + HEADER) );
// 		output[i] = (unsigned char*)Allocate( sizeof(unsigned char)* (MAX_CHUNKSIZE + HEADER) );
// 		if(input[i] == NULL || output[i] == NULL)
// 		{
// 			std::cout << "aborting " <<std::endl;
// 			return 1;
// 		}
// 	}

// 	std::cout << "Succesfully setup server and allocated memory " << std::endl;

// #ifdef __SDSCC__
// 	sds_utils::perf_counter hw_ctr;
// 	std::cout << "Starting test run"  << std::endl;
// #endif
// 	// 1 resource is about 1.5gbps 2 resources is about 3gbps
// 	for(int i =0; i < pipe_depth; i+=1)
// 	{
// #ifdef __SDSCC__
// 		server.get_packet(input[i]);

// 		// get packet
// 		unsigned char* buffer = input[i];

// 		// decode
// 		done = buffer[1] & DONE_BIT_L;
// 		length = buffer[0] | (buffer[1] << 8);
// 		length &= ~DONE_BIT_H;


// #pragma SDS async(1);
// 		compute_hw(&buffer[HEADER],output[i], length);
// #else
// 		length = server.get_packet(input[i]);
// 		std::cout << "length " << length <<std::endl;
// 		if(length == 0)
// 		{
// 			done = 1;
// 			break;
// 		}
// 		// get packet
// 		unsigned char* buffer = input[i];

// #pragma SDS async(1);
// 		compute_hw(&buffer[0],output[i], length);
// #endif

// 		//
// 		offset+= length;
// 	}
// #ifdef __SDSCC__
// 	hw_ctr.start();
// #endif

// 	writer = pipe_depth;
// 	reader = 0;

// 	//last message
// 	while(!done)
// 	{
// 		// manage ring buffer index
// 		writer = (writer == RING_SIZE) ? 0 : writer;

// 		// manage ring buffer index
// 		reader = (reader == RING_SIZE) ? 0 : reader;

// #ifdef __SDSCC__
// 		server.get_packet(input[writer]);

// 		// get packet
// 		unsigned char* buffer = input[writer];

// 		// decode
// 		done = buffer[1] & DONE_BIT_L;
// 		length = buffer[0] | (buffer[1] << 8);
// 		length &= ~DONE_BIT_H;

// #pragma SDS wait(1)
// #pragma SDS async(1);
// 		compute_hw(&buffer[HEADER],output[writer], length);

// 		// get packet
// 		unsigned char* bufferout = output[reader];

// 		// decode
// 		compressed_length = bufferout[0] | (bufferout[1] << 8);

// #pragma SDS async(2);
// 		append_hw(&bufferout[HEADER],&file[compressed_offset],compressed_length);
// #else
// 		length = server.get_packet(input[writer]);
// 		std::cout << "length " << length <<std::endl;
// 		if(length == 0){
// 			done = 1;
// 			break;
// 		}

// 		// get packet
// 		unsigned char* buffer = input[writer];

// #pragma SDS wait(1)
// #pragma SDS async(1);
// 		compute_hw(&buffer[0],output[writer], length);

// 		// get packet
// 		unsigned char* bufferout = output[reader];

// 		// decode
// 		compressed_length = bufferout[0] | (bufferout[1] << 8);
// 		printf("clength: %d\n",compressed_length);

// #pragma SDS async(2);
// 		append_hw(&bufferout[HEADER],&file[compressed_offset],compressed_length);

// #endif

// 		//
// 		offset+= length;
// 		writer++;
// 		reader++;
// 		appends++;
// 		compressed_offset += compressed_length;
// 	}


// 	// flush the pipe of compute hw
// 	for(int i =0; i < pipe_depth; i++)
// 	{
// 		// manage ring buffer index
// 		reader = (reader == RING_SIZE) ? 0 : reader;

// 		// get packet
// 		unsigned char* bufferout = output[reader];

// 		// decode
// 		compressed_length = bufferout[0] | (bufferout[1] << 8);
// 		printf("clength: %d\n",compressed_length);

// #pragma SDS wait(1);
// #pragma SDS async(2);
// 		append_hw(&bufferout[HEADER],&file[compressed_offset],compressed_length);

// 		compressed_offset += compressed_length;
// 		reader++;
// 		appends++;
// 	}

// 	// flush the pipe of append_hw
// 	for(int i =0; i < appends; i++)
// 	{
// #pragma SDS wait(2);
// 	}

// #ifdef __SDSCC__
// 	hw_ctr.stop();
// 	std::cout << "Bytes processed: " << offset * sizeof(char) << " Average number of CPU cycles in hardware: " << hw_ctr.avg_cpu_cycles() << std::endl;
// #endif

// 	//
// 	Store_data(file, compressed_offset);

// 	//
// 	std::cout << "Succesfully wrote " << compressed_offset <<  std::endl;


// 	//
// 	for(int i = 0; i < RING_SIZE; i++)
// 	{
// 		Free(input[i]);
// 		Free(output[i]);
// 	}

// 	//
// 	Free(file);
// 	return 0;
// }


