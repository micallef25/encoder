#include <stdlib.h>
#include<string.h>
#include<stdio.h>
#include<iostream>
#include "cdc_to_sha_testbench.h"

// #include "../../hardware_apps/cdc/cdc_hw.h"
#include "../../software_apps/cdc/cdc_sw.h"
#include "../../software_apps/sha/sha_sw.h"
#include "../../common/sds_utils.h"
#include "../../common/utils.h"
#include "../../full_flow/compute_hw.h"

#define MINI "C:/school/Penn/SOC/personal/src/src/testfiles/mini.tar"
#define VMLINUZ "C:/school/Penn/SOC/personal/src/src/testfiles/vmlinuz.tar"
#define PRINCE "C:/school/Penn/SOC/personal/src/src/testfiles/prince.txt"

void test_cdc_sha( const char* file )
{
    //

    
    //
	FILE* fp = fopen(file,"r+" );
	if(fp == NULL ){
		perror("fopen error");
		return;
	}
		
	//
	fseek(fp, 0, SEEK_END); // seek to end of file
	unsigned int file_size = ftell(fp); // get current file pointer
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
	unsigned int bytes_read = fread(&buff[0],sizeof(unsigned char),file_size,fp);
	printf("bytes_read %llu\n",bytes_read);

	Rabin* rks = new Rabin;
	cdc_test_t* test_ptr = new cdc_test_t;
	cdc_test_t* hw_test_ptr = new cdc_test_t;

	unsigned int key = ((file_size / 2048)+1);
	std::cout << "filesize " << file_size <<" key " << key <<std::endl;

	unsigned int* hwkeys = AllocateInt(sizeof(unsigned int*) * key*8);

	unsigned int* swkeys = AllocateInt(sizeof(unsigned int*) * key*8);


    // create table and then perform CDC
    rks->create_table();
    rks->patternSearch(buff,file_size,test_ptr);
    unsigned int sum = 0;
    // allocate nough space for key buffer
   for(int k = 0; k < rks->chunks.size(); k++)
   {
   		std::string data = rks->chunks.at(k);
   		std::cout << "sw chunk length end" << data.length() << " k " << k << std::endl;
   		sum+= data.length();
   		sha_sw( (unsigned char*)data.c_str(),&swkeys[k*8],data.length());
   }
   //unsigned char raw[4][8196];
   std::cout << "sw read " << sum <<std::endl;

   //std::cout << rks->chunks.at(0) << std::endl;
   // std::cout << "---" <<std::endl;
   // std::cout << rks->chunks.at(1) << std::endl;
   // std::cout << "---" <<std::endl;
//   for(int w = 0; w < 2844; w++)
//   {
//	   std::cout << raw[1][w] <<std::endl;
//   }

    //
    compute_hw(buff,hwkeys,file_size);

    //compare raw data and compare the keys
    if(memcmp(swkeys,hwkeys,sizeof(int)*8*rks->chunks.size()) == 0)
    {
    	std::cout << "woohoo!!!!" << std::endl;
    }
    else
    {
    	for(int i = 0; i < 8*rks->chunks.size();i++)
    	{
    		std::cout << "hw[]: " << i  << ": "<< hwkeys[i] << std::endl;
    		std::cout << "sw[]: " << i << ": " <<  swkeys[i] << std::endl;
    	}
    }

    Free(buff);
    FreeInt(outbuff);
    FreeInt(swkeys);
    FreeInt(hwkeys);
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

	test_cdc_sha(VMLINUZ);
//	test_cdc_sha(PRINCE);
	// test_cdc_random();
	// test_cdc_repeition();
	return 0;
}
