#include <stdlib.h>
#include<string.h>
#include<stdio.h>
#include<iostream>

#include "../../hardware_apps/lzw/lzw.h"
#include "../../common/sds_utils.h"
#include "../../common/utils.h"


#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdint.h>

#define CODE_LENGTH (13)
#define MAX_CHUNK_SIZE (1<<CODE_LENGTH)

#define COMP "C:/school/Penn/SOC/personal/src/src/testfiles/compress.dat"
#define UNCOMP "C:/school/Penn/SOC/personal/src/src/testfiles/compress.dat"
#define GOLDEN "C:/school/Penn/SOC/personal/src/src/testfiles/golden.dat"

typedef std::vector<std::string> code_table;
typedef std::vector<std::string> chunk_list;

static code_table Code_table;

static std::ifstream Input;
static std::ifstream Golden;
static size_t Input_position;

void compareFiles(FILE *fp1, FILE *fp2) 
{ 
    // fetching character of two file 
    // in two variable ch1 and ch2 
    char ch1 = getc(fp1); 
    char ch2 = getc(fp2); 
  
    // error keeps track of number of errors 
    // pos keeps track of position of errors 
    // line keeps track of error line 
    int error = 0, pos = 0, line = 1; 
  
    // iterate loop till end of file 
    while (ch1 != EOF && ch2 != EOF) 
    { 
        pos++; 
  
        // if both variable encounters new 
        // line then line variable is incremented 
        // and pos variable is set to 0 
        if (ch1 == '\n' && ch2 == '\n') 
        { 
            line++; 
            pos = 0; 
        } 
  
        // if fetched data is not equal then 
        // error is incremented 
        if (ch1 != ch2) 
        { 
            error++; 
            printf("Line Number : %d \tError"
               " Position : %d \n", line, pos); 
        } 
  
        // fetching character until end of file 
        ch1 = getc(fp1); 
        ch2 = getc(fp2); 
    } 
  
    printf("Total Errors : %d\n", error); 
} 

static int Read_code(void)
{
  static unsigned char Byte;

  int Code = 0;
  int Length = CODE_LENGTH;
  for (int i = 0; i < Length; i++)
  {
    if (Input_position % 8 == 0)
      Byte = Input.get();
    Code = (Code << 1) | ((Byte >> (7 - Input_position % 8)) & 1);
    Input_position++;
  }
  return Code;
}

static const std::string Decompress(size_t Size)
{
  Input_position = 0;

  Code_table.clear();
  for (int i = 0; i < 256; i++)
    Code_table.push_back(std::string(1, (char) i));

  int Old = Read_code();
  std::string Symbol(1, Old);
  std::string Output = Symbol;
  while (Input_position / 8 < Size - 1)
  {
    int New = Read_code();
    std::string Symbols;
    if (New >= (int) Code_table.size())
      Symbols = Code_table[Old] + Symbol;
    else
      Symbols = Code_table[New];
    Output += Symbols;
    Symbol = std::string(1, Symbols[0]);
    Code_table.push_back(Code_table[Old] + Symbol);
    Old = New;
  }

  return Output;
}

int Decode(char* file_in, char* file_out )
{

  Input.open(file_in, std::ios::binary);
  if (!Input.good())
  {
    std::cerr << "Could not open input file.\n";
    return EXIT_FAILURE;
  }

  std::ofstream Output(file_out, std::ios::binary);
  if (!Output.good())
  {
    std::cerr << "Could not open output file.\n";
    return EXIT_FAILURE;
  }

  chunk_list Chunks;
  int i = 0;
  while (true)
  {
    uint32_t Header;
    Input.read((char *) &Header, sizeof(int32_t));
    if (Input.eof())
      break;

    if ((Header & 1) == 0)
    {
      int Chunk_size = Header >> 1;
      const std::string & Chunk = Decompress(Chunk_size);
      Chunks.push_back(Chunk);
      std::cout << "Decompressed chunk of size " << Chunk.length() << ".\n";
      Output.write(&Chunk[0], Chunk.length());
    }
    else
    {
     int Location = Header >> 1;
      if (Location<Chunks.size()) {  // defensive programming to avoid out-of-bounds reference
          const std::string & Chunk = Chunks[Location];
          std::cout << "Found chunk of size " << Chunk.length() << " in database.\n";
          Output.write(&Chunk[0], Chunk.length());
	  }
      else
      {
       std::cerr << "Location " << Location << " not in database of length " << Chunks.size() << " ignoring block.  Likely encoder error.\n";
       }
    }
    i++;
  }

  Input.close();
  Output.close();


  // opening both file in read only mode 
  FILE *fp1 = fopen(GOLDEN, "r"); 
  FILE *fp2 = fopen(UNCOMP, "r"); 
  
  if (fp1 == NULL || fp2 == NULL) 
  { 
    printf("Error : Files not open"); 
    exit(0); 
  } 
  compareFiles(fp1,fp2);
  return EXIT_SUCCESS;

}



void test_lzw( )
{

	sds_utils::perf_counter hw_ctr;
	sds_utils::perf_counter sw_ctr;
	hw_ctr.reset();
	sw_ctr.reset();

	// test lowest and highest
	for(unsigned int file_size = 512; file_size < 2048; file_size++)
	{
    
		//
		unsigned char* buff = Allocate((sizeof(unsigned char) * file_size ));
		if(buff == NULL)
		{
			perror("not enough space");
			//fclose(fp);
			return;
		}
		//
		unsigned char* outbuff = Allocate((sizeof(unsigned char) * file_size*2 ));
		if(outbuff == NULL)
		{
			perror("not enough space");
			//fclose(fp);
			return;
		}
	
	// fill with random data
	for(unsigned int z = 0; z < file_size; z++)
	{
		buff[z] = rand();
	}

   unsigned int compressed_size = 0;
   hw_ctr.start();

   // lzw_top
   //std::cout << "starting lzw" << std::endl;
   do_lzw_hw(buff,file_size,outbuff,&compressed_size);
   //std::cout << "ending lzw" << std::endl;

   hw_ctr.stop();

   std::cout << "compressed size" << compressed_size << std::endl;
   //Store_data(buff,file_size,GOLDEN);
   //Store_data(outbuff,compressed_size,COMP);
   //std::cout << "decoding" << std::endl;
   //Decode(COMP,UNCOMP);


    std::cout << "Bytes processed: " << file_size * sizeof(char) << " Average number of CPU cycles in hardware: " << hw_ctr.cpu_cycles() << std::endl;


    Free(buff);
    Free(outbuff);

	}

    return;
}


int run_lzw_testbench()
{
for(int i = 0; i < 10; i++)
	test_lzw();

	return 0;
}
