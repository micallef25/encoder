#ifndef _CDC_H
#define _CDC_H

// capture metadat for comparison of hw to sw
typedef struct cdc_test_t
{
	int chunks;
	int avg_chunksize;
}cdc_test_t;

int run_cdc_testbench();

#endif
