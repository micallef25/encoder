#ifndef _CDC_SHA_H
#define _CDC_SHA_H

int run_cdc_to_sha_testbench();

typedef struct cdc_sha_struct{
	unsigned int key[8];
	unsigned int raw_data[8196];
}cdc_sha_t;

#endif