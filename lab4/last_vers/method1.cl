__kernel void matrix_mult(__global float* m1, __global float* m2, __global float* mat)
{


	int i = get_global_id(1);
	int j = get_global_id(0);
    int m = get_global_size(1);
	int k = get_global_size(2);
	int n = get_global_size(0);
	float sum = 0;
	
	for (int f = 0; f < k; f++) {
		sum += m1[i * k + f] * m2[f * n + j];
	}
	mat[i * n + j] = sum;

}
