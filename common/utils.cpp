#include "utils.h"

#include <iostream>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <fcntl.h>

#ifdef __SDSCC__
#include "sds_lib.h"
#endif

#define FILEIN "/home/tzvi/ESE532/Project/code/Test/lorem.test"
#define FILEOUT "D:/School/Penn/Personal/testfiles/compress.dat"

void Check_error(int Error, const char * Message) {
	if (Error) {
		fputs(Message, stderr);
		exit(EXIT_FAILURE);
	}
}

void Exit_with_error(void) {
	perror(NULL);
	exit(EXIT_FAILURE);
}

unsigned char * Allocate(int Size) {
	unsigned char * Frame = (unsigned char *)
#ifdef __SDSCC__
	sds_alloc(Size);
#else
	malloc(Size);
#endif
	Check_error(Frame == NULL, "Could not allocate memory.\n");

	return Frame;
}

void Free(unsigned char * Frame) {
#ifdef __SDSCC__
	sds_free(Frame);
#else
	free(Frame);
#endif
}

unsigned int * AllocateInt(int Size) {
	unsigned int * Frame = (unsigned int *)
#ifdef __SDSCC__
	sds_alloc(Size);
#else
	malloc(Size);
#endif
	Check_error(Frame == NULL, "Could not allocate memory.\n");

	return Frame;
}

void FreeInt(unsigned int * Frame) {
#ifdef __SDSCC__
	sds_free(Frame);
#else
	free(Frame);
#endif
}

unsigned int Load_data(unsigned char * Data) {

	FILE * File = fopen(FILEIN, "rb");
	if (File == NULL)
		Exit_with_error();

	unsigned int Bytes_read = fread(Data, 1, MAX_INPUT_SIZE, File);

	if (fclose(File) != 0)
		Exit_with_error();

	return Bytes_read;
}

void Store_data(unsigned char * Data, unsigned int Size) {
	FILE * File = fopen(FILEOUT, "w+");
	if (File == NULL)
		Exit_with_error();

	if (fwrite(Data, 1, Size, File) != Size)
		Exit_with_error();

	if (fclose(File) != 0)
		Exit_with_error();
}


int check_data( unsigned char* in, unsigned char* out, uint32_t length)
{
	for( uint32_t i = 0; i < length; i++)
	{
		if(in[i] != out[i]){
		std::cout << "test failed index " << i << std::endl;
			return 1;
		}
	}
	return 0;
}

void handle_input(int argc, char* argv[],int* chunksize)
{
	int x;
	extern char *optarg;
	extern int optind, optopt, opterr;


	while ((x = getopt(argc, argv, ":c:")) != -1)
	{
		switch(x)
		{
		case 'c':
		    *chunksize = atoi(optarg);
			printf("chunkisize is set to %d optarg\n",*chunksize);
			break;
    	case ':':
        	printf("-%c without parameter\n", optopt);
        	break;
    	}
	}
}
