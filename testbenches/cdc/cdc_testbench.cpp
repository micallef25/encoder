#include <stdlib.h>
#include<string.h>
#include<stdio.h>
#include<iostream>
#include "cdc_testbench.h"
#include "../../hardware_apps/sha/cdc_hw.h"

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

    // create table and then perform CDC
    rks->create_table();
    rks->patternSearch(buff,file_size);

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
	test_file();
	// test_cdc_random();
	// test_cdc_repeition();
}