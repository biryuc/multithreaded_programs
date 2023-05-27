__kernel void matrix_mult(__global float *m1, __global float *m2, __global float *m, __global int *sizes)
{

	
	int i = get_global_id(0);
	for(int j=0; j<sizes[2]; j++){
	    for(int k=0; k<sizes[1]; k++){
            m[i * sizes[2] + j]  += m1[i * sizes[1] + k] * m2[k * sizes[2] + j];
        }    
	}

}
