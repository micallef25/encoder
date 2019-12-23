#include <stdlib.h>
#include<string.h>
#include<stdio.h>
#include<iostream>
#include "cdc_testbench.h"
#include "../../hardware_apps/cdc/cdc_hw.h"
#include "../../software_apps/cdc/cdc_sw.h"
#include "../../common/sds_utils.h"
#include "../../common/utils.h"

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
	unsigned char* buff = Allocate((sizeof(unsigned char) * file_size ));	
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
	cdc_test_t* test_ptr = new cdc_test_t;

    // create table and then perform CDC
    rks->create_table();
    rks->patternSearch(buff,file_size,test_ptr);

    free(buff);
    delete rks;
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



int test_cdc()
{
	test_cdc_sw("file");
	// test_cdc_random();
	// test_cdc_repeition();
	return 0;
}
