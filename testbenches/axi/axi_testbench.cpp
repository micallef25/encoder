#include "axi_testbench.h"
#include "../../hardware_apps/axi/axi.h"
#include "../../common/sds_utils.h"
#include "../../common/utils.h"




void test_bit_packing()
{
	unsigned int* hwbuffer1 = AllocateInt(MAX_BUFF_SIZE * sizeof(unsigned int));
	unsigned int* hwbuffer2 = AllocateInt(MAX_BUFF_SIZE * sizeof(unsigned int));
	unsigned int* hw_results = AllocateInt(MAX_BUFF_SIZE * sizeof(unsigned int));

	unsigned int size = MAX_BUFF_SIZE;
	sds_utils::perf_counter hw_ctr;
	sds_utils::perf_counter hw_ctr2;
	hw_ctr2.reset();
	hw_ctr.reset();

	for(int i = 0; i < MAX_BUFF_SIZE; i++)
	{
		hwbuffer1[i] = rand();
		hwbuffer2[i] = rand();
	}


	for(int i = 0; i < 32; i++)
	{

		 hw_ctr.start();
        //Type-Casting int* datatype to wide_dt * to match Hardware Function 
        //declaration 
        vadd_accel_wide( (wide_dt *)hwbuffer1, (wide_dt *)hwbuffer2, (wide_dt *)hw_results, size/NUM_ELEMENTS );

        //
        hw_ctr.stop();

        hw_ctr2.start();
        //Type-Casting int* datatype to wide_dt * to match Hardware Function 
        //declaration 
        vadd_accel_normal( hwbuffer1,hwbuffer2, hw_results, size );

        //
        hw_ctr2.stop();

    	std::cout << "Number of CPU cycles running application in wide lanes: " << hw_ctr.cpu_cycles() << std::endl;
    	std::cout << "Number of CPU cycles running application in normal lanes: " << hw_ctr2.cpu_cycles() << std::endl;


	}

    // Release Memory
    FreeInt(hwbuffer1);
    FreeInt(hwbuffer2);
    FreeInt(hw_results);

}




int run_axi_testbench()
{
	test_bit_packing();

	return 0;
}
