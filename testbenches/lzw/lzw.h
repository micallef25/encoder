#ifndef LZW_HW
#define LZW_HW
/* HW LZW parameters */
#define LZW_CODE_BITS				13 // log2(MAX_CHUNK_SIZE) + 1
#define LZW_KEY_BITS				(LZW_CODE_BITS + 8)
// ^^ We encode this as [key|newchar]
/* LZW CAM parameters */
#define KEY_DEPTH_BITS				9
#define KEY_DEPTH					512 // 2^KEY_DEPTH_BITS
#define CAM_VALUES_SIZE				64
#define CAM_BRAM_BITS				CAM_VALUES_SIZE
#define CAM_VALUES_BITS				6 // log2(CAM_VALUES_SIZE)
// ^^ CAM_BRAM_PART * CAM_BRAM_BITS = CAM_VALUES_SIZE (!!!)
/* LZW Hash table parameters */
#define LZW_HASH_BITS				13
#define NUM_BUCKETS_PER_HASH		4
#define HASH_TABLE_COUNTER_BITS		3 // We need one extra bit for the overflow
#define HASH_TABLE_ITEM_WIDTH		(LZW_CODE_BITS + LZW_KEY_BITS + 1)
#define HASH_TIW					HASH_TABLE_ITEM_WIDTH
#define HASH_TABLE_ROW_WIDTH		(HASH_TIW * NUM_BUCKETS_PER_HASH)
#define HASH_TABLE_SIZE				8192

#define MIN_CHUNK_SIZE				2048
#define MAX_CHUNK_SIZE				8192

/* Dummy boundaries (replace?) */
#ifdef __SDSCC__
#define MAX_INPUT_SIZE				(200283920) // Don't ask me about this!
#define MAX_OUTPUT_SIZE				(200283920)
#else
#define MAX_INPUT_SIZE				(1 << 28)
#define MAX_OUTPUT_SIZE				(200283920) // formerly 1<<29
#endif
#define LZW_WORST_CASE_FACTOR		2
#define MAX_LZW_SIZE				(LZW_WORST_CASE_FACTOR * MAX_CHUNK_SIZE)
#define MAX_WORD_SIZE				128

/* Output parameters */
#define OUTPUT_BITS					13

#endif
