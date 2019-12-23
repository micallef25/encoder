#include "axi.h"

//Memory Datawidth of accelerator is calculated based on argument type.
//Here argument type of in1 and in2 is wide_dt which is 128bit wide, so memory
//interface will be created to 128bit wide. 
void vadd_accel_wide(
        const wide_dt *in1, // Read-Only Vector 1
        const wide_dt *in2, // Read-Only Vector 2
        wide_dt *out,       // Output Result
        int size            // Size of total elements
        )
{
    vadd:for(int i = 0; i < size;  ++i)
    {
        //Pipelined this loop which will eventually infer burst read/write
        //for in1, in2 and out as access pattern is sequential
        #pragma HLS pipeline
        #pragma HLS LOOP_TRIPCOUNT min=c_size max=c_size
        wide_dt tmpV1     = in1[i];
        wide_dt tmpV2     = in2[i];
        wide_dt tmpOut;
        for (int k = 0 ; k < NUM_ELEMENTS ; k++){
            //As Upper loop "vadd" is marked for Pipeline so this loop 
            //will be unrolled and will do parallel vector addition for 
            //all elements of structure.
            tmpOut.data[k] = tmpV1.data[k] + tmpV2.data[k];
        }
        out[i] = tmpOut;
    }
}


void vadd_accel_normal(
        const unsigned int *in1, // Read-Only Vector 1
        const unsigned int *in2, // Read-Only Vector 2
        unsigned int *out,       // Output Result
        int size            // Size of total elements
        )
{
    vadd:for(int i = 0; i < size;  ++i)
    {
        //Pipelined this loop which will eventually infer burst read/write
        //for in1, in2 and out as access pattern is sequential
        #pragma HLS pipeline
        #pragma HLS LOOP_TRIPCOUNT min=c_size max=c_size

    	out[i] = in1[i]+in2[i];
    }
}