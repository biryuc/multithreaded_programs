__kernel void matrix_mult(__global float* m1, __global float* m2, __global float* mat, __global int* sizes)
{


	int i = get_global_id(0);
	int m = sizes[0];
	int k = sizes[1];
	int n = sizes[2];
	for (int j = 0; j < n; j++) {
		for (int f = 0; f < k; f++) {
			mat[i * n + j] += m1[i * k + f] * m2[f * n + j];
		}
	}

}
