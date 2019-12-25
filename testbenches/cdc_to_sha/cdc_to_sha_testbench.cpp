#include <stdlib.h>
#include<string.h>
#include<stdio.h>
#include<iostream>
#include "cdc_to_sha_testbench.h"

#include "../../hardware_apps/cdc/cdc_hw.h"
#include "../../software_apps/cdc/cdc_sw.h"
#include "../../common/sds_utils.h"
#include "../../common/utils.h"
#include "../../full_flow/compute_hw.h"

#define VMLINUZ "C:/school/Penn/SOC/personal/src/src/testfiles/vmlinuz.tar"
#define PRINCE "C:/school/Penn/SOC/personal/src/src/testfiles/prince.txt"

void test_cdc_sha( const char* file )
{
    //

    
    //
	FILE* fp = fopen(file,"r" );
	if(fp == NULL ){
		perror("fopen error");
		return;
	}
		
	//
	fseek(fp, 0, SEEK_END); // seek to end of file
	int file_size = ftell(fp); // get current file pointer
	fseek(fp, 0, SEEK_SET); // seek back to beginning of file

	//
	unsigned char* buff = Allocate((sizeof(unsigned char) * file_size ));
	if(buff == NULL)
	{
		perror("not enough space");
		fclose(fp);
		return;
	}
	//
	unsigned int* outbuff = AllocateInt((sizeof(unsigned int) * file_size ));
	if(outbuff == NULL)
	{
		perror("not enough space");
		fclose(fp);
		return;
	}
	
	//
	int bytes_read = fread(&buff[0],sizeof(unsigned char),file_size,fp);
	printf("bytes_read %d\n",bytes_read);

	Rabin* rks = new Rabin;
	cdc_test_t* test_ptr = new cdc_test_t;
	cdc_test_t* hw_test_ptr = new cdc_test_t;

    // create table and then perform CDC
    rks->create_table();
    rks->patternSearch(buff,file_size,test_ptr);

    // allocate nough space for key buffer

//    for(int k = 0; k < rks->chunks.size(); k++)
//    {
//    	std::string data = rks->chunks.at(k);
//    	sha_sw( (unsigned char*)data.c_str(),key[k],data.length());
//    }


    //
    compute_hw(buff,outbuff,file_size);

    //compare raw data and compare the keys


    Free(buff);
    FreeInt(outbuff);
    delete rks;
    delete test_ptr;
    delete hw_test_ptr;
    return;
}

// void test_cdc_random()
// {
//		make buffer
//		fill with random stuff
// }

// void test_cdc_repeition()
// {
// 	// make a buffer
// 	// memset to a random number
//  // see performance
// }



int run_cdc_to_sha_testbench()
{

//	test_cdc(VMLINUZ);
	test_cdc_sha(PRINCE);
	// test_cdc_random();
	// test_cdc_repeition();
	return 0;
}
