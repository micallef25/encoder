#include <stdlib.h>
#include<string.h>
#include<stdio.h>
#include "../../hardware_apps/sha/sha.h"
#include "../../software_apps/sha/sha_sw.h"


int main()
{
	//
	unsigned char shain[64] = { 0x61,0x62,0x63,0x80, 0x00,0x00,0x00,0x00,   0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,
								0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,   0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,
								0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,   0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,
								0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,   0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x18 };

		unsigned char shain_unpadded[3] = { 0x61,0x62,0x63 };

	//
	unsigned char shain2[128] = { 0x61,0x62,0x63,0x64, 0x62,0x63,0x64,0x65,   0x63,0x64,0x65,0x66,  0x64,0x65,0x66,0x67,
								0x65,0x66,0x67,0x68, 0x66,0x67,0x68,0x69,   0x67,0x68,0x69,0x6a,  0x68,0x69,0x6a,0x6b,
								0x69,0x6a,0x6b,0x6c, 0x6a,0x6b,0x6c,0x6d,   0x6b,0x6c,0x6d,0x6e,  0x6c,0x6d,0x6e,0x6f,
								0x6d,0x6e,0x6f,0x70, 0x6e,0x6f,0x70,0x71,   0x80,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,
								0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,   0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,
								0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,   0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,
								0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,   0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,
								0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,   0x00,0x00,0x00,0x00,  0x00,0x00,0x01,0xc0};
		//
	unsigned char shain_notpadded[128] = { 0x61,0x62,0x63,0x64, 0x62,0x63,0x64,0x65,   0x63,0x64,0x65,0x66,  0x64,0x65,0x66,0x67,
								0x65,0x66,0x67,0x68, 0x66,0x67,0x68,0x69,   0x67,0x68,0x69,0x6a,  0x68,0x69,0x6a,0x6b,
								0x69,0x6a,0x6b,0x6c, 0x6a,0x6b,0x6c,0x6d,   0x6b,0x6c,0x6d,0x6e,  0x6c,0x6d,0x6e,0x6f,
								0x6d,0x6e,0x6f,0x70, 0x6e,0x6f,0x70,0x71,};
	unsigned int shaout_hw[8];
	unsigned int shaout_sw[8];
	sha_256(&shain_notpadded[0],&shaout_hw[0],128);

	// for(int i = 0; i < 8; i++)
 //   {
 //   	printf("hash=%d %#08x\n",i,shaout_hw[i]);
 //   }

	sha_sw(&shain_notpadded[0],&shaout_sw[0],56);

	// for(int i = 0; i < 8; i++)
 //   {
 //   	printf("hash=%d %#08x\n",i,shaout_sw[i]);
 //   }

	if(memcmp(shaout_sw,shaout_hw,8) == 0)
	{
		printf("test pass\n");
	}
	else{
		printf("test failed\n");
	}

	return 0;
}
