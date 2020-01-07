#include <stdlib.h>
#include<string.h>
#include<stdio.h>
#include<iostream>
#include "../sha/sha_testbench.h"
#include "../cdc/cdc_testbench.h"
#include "../lzw/lzw_testbench.h"
#include "../cdc_to_sha/cdc_to_sha_testbench.h"

int main()
{

	// this will run everything in the sha test bench
	//run_sha_testbench();

	//
	//run_cdc_testbench();

	//
	//run_cdc_to_sha_testbench();

	//
	run_lzw_testbench();

	return 0;
}
