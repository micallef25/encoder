#ifndef AXI_H
#define AXI_H

#define MAX_BUFF_SIZE 32784 // 32k
#define NUM_ELEMENTS   4 // To make structure size 128bit
//Structure overall width is set to 4 Integers = 4 *32 = 128bit to match to
//Zynq ultrascale Memory Interface Datawidth to get the optimum memory access 
//performance.
typedef struct wide_dt_struct{
    int data[NUM_ELEMENTS];
} __attribute__ ((packed, aligned(4))) wide_dt;


#pragma SDS data copy(in1[0:size],in2[0:size], out[0:size])
#pragma SDS data mem_attribute( in1:PHYSICAL_CONTIGUOUS ,in2:PHYSICAL_CONTIGUOUS , out:PHYSICAL_CONTIGUOUS  )
#pragma SDS data access_pattern( in1:SEQUENTIAL,in2:SEQUENTIAL , out:SEQUENTIAL )
void vadd_accel_wide(  const wide_dt *in1, const wide_dt *in2,wide_dt *out,int size );

#pragma SDS data copy(in1[0:size],in2[0:size], out[0:size])
#pragma SDS data mem_attribute( in1:PHYSICAL_CONTIGUOUS ,in2:PHYSICAL_CONTIGUOUS , out:PHYSICAL_CONTIGUOUS  )
#pragma SDS data access_pattern( in1:SEQUENTIAL,in2:SEQUENTIAL , out:SEQUENTIAL )
void vadd_accel_normal(  const unsigned int *in1, const unsigned int *in2, unsigned int *out,int size );

#endif
