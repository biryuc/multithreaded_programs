__kernel void matrix_mult_tile(__global float* m1, __global float* m2, __global float* mat, __global int* sizes) {

    int m = sizes[0];
	int k = sizes[1];
	int n = sizes[2];
	const int l_i = get_local_id(0); 
    const int l_j = get_local_id(1); 
    const int g_i = get_global_id(0); 
    const int g_j = get_global_id(1); 
 
   
    __local float m1_tile[TILE_SIZE][TILE_SIZE];
    __local float m2_tile[TILE_SIZE][TILE_SIZE];
 
   
    float tmp = 0;
    int t_i;
    int t_j;
    for(int p = 0;p<TILE_SIZE;p++){
        for(int h = 0;h<TILE_SIZE;h++){
                m1_tile[p][h] = 0;
                m2_tile[p][h] = 0;
        }
    }
   
  
    for (int t=0; t<m; t+=TILE_SIZE) {

       
        t_i = t + l_i;
        t_j = t + l_j;


       m1_tile[l_i][l_j] = m1[g_j*k + t_i];
       m2_tile[l_i][l_j] = m2[t_j*n + g_i];
 
        
        barrier(CLK_LOCAL_MEM_FENCE);
 
        
        for (int f=0; f<TILE_SIZE; f++) {
            tmp += m1_tile[f][l_j]*m2_tile[l_i][f];

           
        }
 
       
        barrier(CLK_LOCAL_MEM_FENCE);
    }

   
    mat[g_j*n + g_i] = tmp;
   
}
