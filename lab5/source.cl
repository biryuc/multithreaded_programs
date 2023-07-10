#define WORK_GROUP_SIZE 256
__kernel void prefix_sum(__global float* in_data, __global float* out_data, __global float* sum_from_group) {

    int global_id = get_global_id(0);
    int local_id = get_local_id(0);
	int global_size = get_global_size(0);
	int group_id = get_group_id(0);
	int group_size = get_local_size(0);
	

    __local float partial_sums[WORK_GROUP_SIZE];

    partial_sums[local_id] = in_data[global_id];
	

	barrier(CLK_LOCAL_MEM_FENCE);
    for (int i = 1; i < WORK_GROUP_SIZE; i *= 2) {

		if(local_id  >= i){
			 partial_sums[local_id] +=  partial_sums[local_id - i] ;
		}
		
		barrier(CLK_LOCAL_MEM_FENCE);
    }
	
	
    out_data[global_id] = partial_sums[local_id];
	
	if(local_id == WORK_GROUP_SIZE - 1) {
		sum_from_group[group_id] = partial_sums[local_id];
	}
}

__kernel void sum_from_groups(__global float* out_data, __global float* sum_from_group) {
    int global_id = get_global_id(0);
	int groups_count = get_num_groups(0) - 1;
	

		for(int i = 0; i < groups_count; i++) {
			
			out_data[global_id] += sum_from_group[i];
	}
}
