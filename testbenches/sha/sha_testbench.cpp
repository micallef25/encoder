#include <stdlib.h>
#include<string.h>
#include<stdio.h>
#include<iostream>
#include "../../hardware_apps/sha/sha.h"
#include "../../software_apps/sha/sha_sw.h"
#include "../../common/sds_utils.h"
#include "../../common/utils.h"

// default is pipelining we can only run certain tests
// due to synthesis creating too many interfaces and using up all the resources
#ifdef UNIT_TEST_SHA


void test_string_size()
{

	unsigned int* shaout_sw = AllocateInt(8 * sizeof(unsigned int));
	unsigned int* shaout_hw = AllocateInt(8 * sizeof(unsigned int));

	sds_utils::perf_counter hw_ctr;
	sds_utils::perf_counter sw_ctr;
	hw_ctr.reset();
	sw_ctr.reset();
	FILE *fptr = fopen("varysize.csv", "w+");
	if(fptr == NULL)
	{
		return;
	}


	std::cout << "running varying string SHA tests" << std::endl;

	// run tests
	for(int i = 1; i < MAX_TEST_SIZE; i++)
	{

		unsigned char* buffer = Allocate( i * sizeof(unsigned char));
		if(buffer == NULL)
		{
			std::cout << "failed to make memory " << std::endl;
			return;
		}

		for(int k = 0; k < i; k++)
		{
			buffer[k] = rand();
		}

		hw_ctr.start();

		//
		sha_hw(&buffer[0],&shaout_hw[0],i);


		hw_ctr.stop();


		sw_ctr.start();

		//
		sha_sw(&buffer[0],&shaout_sw[0],i);

		sw_ctr.stop();



		if(memcmp(shaout_sw,shaout_hw,8) == 0)
		{
			std::cout << "Test " << i << " passed!" << std::endl;

			float speedup = (float)sw_ctr.cpu_cycles() / (float)hw_ctr.cpu_cycles();

			std::cout << "Bytes processed: " << i * sizeof(char) << " Average number of CPU cycles in hardware: " << hw_ctr.cpu_cycles() << std::endl;
			std::cout << "Bytes processed: " << i * sizeof(char) << " Average number of CPU cycles in software: " << sw_ctr.cpu_cycles() << std::endl;
			std::cout << "speed up " << speedup << std::endl;

			// format of hw,sw,speedup
			fprintf(fptr,"%d,%lu,%lu,%f",i,hw_ctr.cpu_cycles(),sw_ctr.cpu_cycles(),speedup);

			//
			hw_ctr.reset();
			sw_ctr.reset();
		}
		else{
			printf("test failed %d\n",i);

			Free(buffer);

			break;
		}

		Free(buffer);
	}

	FreeInt(shaout_sw);
	FreeInt(shaout_hw);
	fclose(fptr);

}

void test_constant_size()
{

	unsigned int* shaout_sw = AllocateInt(8 * sizeof(unsigned int));
	unsigned int* shaout_hw = AllocateInt(8 * sizeof(unsigned int));

	sds_utils::perf_counter hw_ctr;
	sds_utils::perf_counter sw_ctr;
	hw_ctr.reset();
	sw_ctr.reset();
	FILE *fptr = fopen("varysize.csv", "w+");
	if(fptr == NULL)
	{
		return;
	}

	std::cout << "running constant string SHA tests" << std::endl;

	unsigned char* buffer = Allocate( MAX_BUFF_SIZE * sizeof(unsigned char));
	if(buffer == NULL)
	{
		std::cout << "failed to make memory " << std::endl;
		return;
	}

	// run tests
	for(int i = 1; i < MAX_TEST_SIZE; i++)
	{

		for(int k = 0; k < MAX_BUFF_SIZE; k++)
		{
			buffer[k] = rand();
		}

		hw_ctr.start();

		//
		sha_hw(&buffer[0],&shaout_hw[0],MAX_BUFF_SIZE);

		hw_ctr.stop();

		sw_ctr.start();

		//
		sha_sw(&buffer[0],&shaout_sw[0],MAX_BUFF_SIZE);

		sw_ctr.stop();

		if(memcmp(shaout_sw,shaout_hw,8) == 0)
		{
			std::cout << "Test " << i << " passed!" << std::endl;

			float speedup = (float)sw_ctr.cpu_cycles() / (float)hw_ctr.cpu_cycles();

			std::cout << "Bytes processed: " << MAX_BUFF_SIZE * sizeof(char) << " Average number of CPU cycles in hardware: " << hw_ctr.cpu_cycles() << std::endl;
			std::cout << "Bytes processed: " << MAX_BUFF_SIZE * sizeof(char) << " Average number of CPU cycles in software: " << sw_ctr.cpu_cycles() << std::endl;
			std::cout << "speed up " << speedup << std::endl;

			// format of bytes,hw,sw,speedup
			fprintf(fptr,"%d,%lu,%lu,%f",MAX_BUFF_SIZE,hw_ctr.cpu_cycles(),sw_ctr.cpu_cycles(),speedup);

			//
			hw_ctr.reset();
			sw_ctr.reset();
		}
		else{
			printf("test failed %d\n",i);

			Free(buffer);

			break;
		}

	}

	Free(buffer);
	FreeInt(shaout_sw);
	FreeInt(shaout_hw);
	fclose(fptr);
}
#else
void test_constant_size()
{

}
void test_string_size()
{

}

#endif

#ifdef __SDSCC__ && defined(TEST_SHA_PIPELINED)
void test_pipeline()
{
	unsigned int* shaout_sw[16];
	unsigned int* shaout_hw[16];
	unsigned char* buffer[16];

	for(int i = 0; i < 16; i++)
	{
		shaout_sw[i] = (unsigned int*)sds_alloc(8 * sizeof(unsigned int));
		shaout_hw[i] = (unsigned int*)sds_alloc(8 * sizeof(unsigned int));
	}

	sds_utils::perf_counter hw_ctr;
	sds_utils::perf_counter sw_ctr;
	hw_ctr.reset();
	sw_ctr.reset();

	FILE *fptr = fopen("pipeline.csv", "w+");
	if(fptr == NULL)
	{
		return;
	}

	std::cout << "pipelining 4 SHA accelerators..." << std::endl;

	for(int i = 0; i < 16; i++)
	{
		buffer[i] = (unsigned char*)sds_alloc( MAX_BUFF_SIZE * sizeof(unsigned char));
		if(buffer[i] == NULL)
		{
			std::cout << "failed to make memory " << std::endl;
			exit(0);
		}
		for(int k = 0; k < MAX_BUFF_SIZE; k++ )
		{
			buffer[i][k] = rand();
		}
	}


hw_ctr.start();


	// run tests
	for(int i = 0; i < 16; i+=4)
	{

		//
#pragma SDS async(1)
#pragma SDS resource(1)
		sha_hw(buffer[i],shaout_hw[i],MAX_BUFF_SIZE);
		//
#pragma SDS async(2)
#pragma SDS resource(2)
		sha_hw(buffer[i+1],shaout_hw[i+1],MAX_BUFF_SIZE);
		//
#pragma SDS async(3)
#pragma SDS resource(3)
		sha_hw(buffer[i+2],shaout_hw[i+2],MAX_BUFF_SIZE);
		//
#pragma SDS async(4)
#pragma SDS resource(4)
		sha_hw(buffer[i+3],shaout_hw[i+3],MAX_BUFF_SIZE);

	}

	for(int i = 0; i < 16; i+=4)
	{
#pragma SDS wait(1)
#pragma SDS wait(2)
#pragma SDS wait(3)
#pragma SDS wait(4)
	}


		hw_ctr.stop();

		sw_ctr.start();


for(int i = 0; i < 16; i++)
{
		//
		sha_sw(buffer[i],shaout_sw[i],MAX_BUFF_SIZE);
}

		sw_ctr.stop();

		std::cout << "pipeline test passed!" << std::endl;
		float speedup = (float)sw_ctr.cpu_cycles() / (float)hw_ctr.cpu_cycles();
			
		std::cout << "Bytes processed: " << MAX_BUFF_SIZE * sizeof(char) << " Average number of CPU cycles in hardware: " << hw_ctr.cpu_cycles() << std::endl;
		std::cout << "Bytes processed: " << MAX_BUFF_SIZE * sizeof(char) << " Average number of CPU cycles in software: " << sw_ctr.cpu_cycles() << std::endl;
		std::cout << "speed up " << speedup << std::endl;

		// format of bytes,hw,sw,speedup
		fprintf(fptr,"%d,%lu,%lu,%f",MAX_BUFF_SIZE,hw_ctr.cpu_cycles(),sw_ctr.cpu_cycles(),speedup);

		//
		hw_ctr.reset();
		sw_ctr.reset();

	for(int i = 0; i < 16; i++)
	{
		sds_free(buffer[i]);
		sds_free(shaout_sw[i]);
		sds_free(shaout_hw[i]);
	}
	fclose(fptr);

}
#else
void test_pipeline()
{

}
#endif


int run_sha_testbench()
{

	
	test_string_size();


	test_constant_size();

#if defined(__SDSCC__) && defined(TEST_SHA_PIPELINED)
	//
	test_pipeline();
#endif

	return 0;
}
