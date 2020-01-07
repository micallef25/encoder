#ifndef UTILS_H_
#define UTILS_H_


#define MAX_INPUT_SIZE 	200000000 //200MB
#define MAX_CHUNK 	8192 //8KB
#define NUM_CHUNKS 	25000 //ceil(INPUT_SIZE/MAX_CHUNK) This doesn't work because of static array needing constant length
#define MAX_OUTPUT_SIZE 150000000 //WAY too large, but conservative, noting that encoded value may be larger than original


#define MAX_TEST_SIZE 128 // 32k
#define MAX_BUFF_SIZE 32784 // 32k

void handle_input(int argc, char* argv[],int* chunksize);
unsigned char * Allocate(int Size);
void Free(unsigned char * Frame);
unsigned int * AllocateInt(int Size);
void FreeInt(unsigned int * Frame);
unsigned int** AllocateArr(int Size);
void FreeArr(unsigned int** Frame);
void Store_data(unsigned char * Data, unsigned int Size,char* name);
unsigned int Load_data(unsigned char * Data);

#endif
