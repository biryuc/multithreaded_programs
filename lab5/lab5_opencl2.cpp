#include <chrono>
#include <fstream>
#include <CL/cl.h>
#include <chrono>
#include <vector>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <string>
#include <chrono>
#include <malloc.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#pragma comment(lib, "opencl.lib")
#endif
#include <map>
#include <set>
using namespace std;

#define WORK_GROUP_SIZE 256

struct comp
{
    template<typename T>
    bool operator()(const T& l, const T& r) const
    {
        if (l.second != r.second) {
            return l.second < r.second;
        }

        return l.first < r.first;
    }
};

std::string get_program_text(string filename) {
    std::ifstream t(filename);
    return std::string((std::istreambuf_iterator<char>(t)),
        std::istreambuf_iterator<char>());
}


void get_platforms(cl_device_id* full_device_ids, string* device_names, map<int, int>& types, int& cnt, map<int, int>& tile) {
    cl_int err;
    cl_int ret;
    cl_uint platform_num;
    cl_uint device_num;

    int cnt_discrete = 0;
    int cnt_integrete = 0;
    int cnt_cpu = 0;




    ret = clGetPlatformIDs(0, NULL, &platform_num);
    if (!platform_num)
    {
        printf("Number of platforms: 0\n");
        return;

    }

    cl_platform_id* platform_ids = (cl_platform_id*)malloc(sizeof(cl_platform_id) * platform_num);
    if (platform_ids == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        exit(1);
    }
    clGetPlatformIDs(platform_num, platform_ids, NULL);

    for (cl_uint i = 0; i < platform_num; ++i)
    {
        size_t pn_size;
        clGetPlatformInfo(platform_ids[i], CL_PLATFORM_VENDOR, 0, NULL, &pn_size);
        char* platform_name = (char*)malloc(sizeof(char) * pn_size);
        if (platform_name == nullptr) {
            fprintf(stderr, "Memory cannot be allocated");
            free(platform_ids);
            exit(1);
        }
        clGetPlatformInfo(platform_ids[i], CL_PLATFORM_VENDOR, pn_size, platform_name, 0);

        cl_uint device_num;
        clGetDeviceIDs(platform_ids[i], CL_DEVICE_TYPE_ALL, 0, NULL, &device_num);

        if (!device_num)
        {
            continue;
        }

        cl_device_id* device_ids = (cl_device_id*)malloc(sizeof(cl_device_id) * device_num);
        if (device_ids == nullptr) {
            fprintf(stderr, "Memory cannot be allocated");
            free(platform_name);
            free(platform_ids);
            exit(1);
        }
        clGetDeviceIDs(platform_ids[i], CL_DEVICE_TYPE_ALL, device_num,
            device_ids, NULL);

        for (cl_uint d = 0; d < device_num; ++d)
        {
            size_t size;
            cl_device_type type;
            cl_uint discrete_type;
            size_t max_work_group_size;
            clGetDeviceInfo(device_ids[d], CL_DEVICE_TYPE, sizeof(type), &type, NULL);

            clGetDeviceInfo(device_ids[d], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL);

            if (type == CL_DEVICE_TYPE_GPU) {
                clGetDeviceInfo(device_ids[d], CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(discrete_type), &discrete_type, NULL);
                if (discrete_type == CL_TRUE) {
                    cnt_discrete++;
                    types[cnt] = 2;
                    tile[cnt] = sqrt(max_work_group_size);
                }
                else if (discrete_type == CL_FALSE) {
                    cnt_integrete++;
                    types[cnt] = 1;
                    tile[cnt] = sqrt(max_work_group_size);
                }
            }
            else if (type == CL_DEVICE_TYPE_CPU) {
                cnt_cpu++;
                types[cnt] = 3;
                tile[cnt] = 1;
            }

            clGetDeviceInfo(device_ids[d], CL_DEVICE_NAME, 0, NULL, &size);
            char* name = (char*)malloc(sizeof(char) * size);
            if (name == nullptr) {
                fprintf(stderr, "Memory cannot be allocated");
                free(platform_name);
                free(platform_ids);
                free(device_ids);
                exit(1);
            }
            clGetDeviceInfo(device_ids[d], CL_DEVICE_NAME, size, name, 0);
            device_names[cnt] = name;
            full_device_ids[cnt] = device_ids[d];
            cnt++;

            free(name);
        }
        free(device_ids);
        free(platform_name);

    }


    free(platform_ids);


}



float* prefix_sum_opencl(float* prefix_arr,int size, int device_num) {

    cl_platform_id platform_id;
    cl_uint error_num_platforms;
    cl_int error;
    cl_device_id device_id;
    cl_uint error_num_devices;
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    cl_kernel kernel_group_sum = NULL;
    map<int, int> types;
    map<int, int> tile;
    int SIZE = 20;
    int cnt = 0;
    int TILE_SIZE = 1;
  
    


    string* device_names = new(nothrow) string[SIZE];

    if (device_names == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");

        exit(1);

    }
    int* device_types = new(nothrow) int[SIZE];
    if (device_types == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        delete[] device_names;
        exit(1);

    }
    cl_device_id* full_device_ids = (cl_device_id*)malloc(sizeof(cl_device_id) * SIZE);
    if (full_device_ids == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        delete[] device_names;
        delete[] device_types;
        exit(1);

    }
    get_platforms(full_device_ids, device_names, types, cnt, tile);
    if (cnt == 0) {
        delete[] device_names;
        delete[] device_types;
        free(full_device_ids);
        exit(1);
    }


    set<std::pair<int, int>, comp> set_types(types.begin(), types.end());

    if (cnt <= device_num && cnt != 0) {

        for (auto const& pair : set_types) {
            device_num = pair.first;
            for (auto it = tile.begin(); it != tile.end(); it++) {
                if (it->first == device_num) {
                    TILE_SIZE = it->second;
                    break;
                }
            }
            break;
        }

    }
    else {
        int l = device_num;
        for (auto const& pair : set_types) {
            device_num = pair.first;
            for (auto it = tile.begin(); it != tile.end(); it++) {
                if (it->first == device_num) {
                    TILE_SIZE = it->second;
                    break;
                }
            }
            l--;
            if (l < 0) {
                break;
            }

        }


    }

    printf("Device: %s\n", device_names[device_num].c_str());

    cl_float* prefix_arr_opencl = new(nothrow) cl_float[size];
    if (prefix_arr_opencl == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        delete[] device_names;
        delete[] device_types;
        free(full_device_ids);
        exit(1);

    }


    cl_context context = clCreateContext(NULL, 1, &full_device_ids[device_num], NULL, NULL, &error);
    if (!context)
    {
        fprintf(stderr, "Error: Failed to create a compute context!\n");
        delete[] device_names;
        delete[] prefix_arr_opencl;
        delete[] device_types;
        free(full_device_ids);
        exit(1);

    }
    cl_command_queue commands = clCreateCommandQueue(context, full_device_ids[device_num], 0, &error);

    if (error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to create commands!\n");
        delete[] prefix_arr_opencl;
        clReleaseContext(context);
        delete[] device_names;
        delete[] device_types;
        free(full_device_ids);
        exit(1);
    }


  
    std::string src = get_program_text("source.cl");
   
    
    const char* src_text = src.data();
    const size_t src_length = src.size();


    program = clCreateProgramWithSource(context, 1, &src_text, &src_length, &error);

  
    error = clBuildProgram(program, 1, &full_device_ids[device_num], NULL, NULL, NULL);


    if (error != CL_SUCCESS)
    {
        size_t len;
        char buffer[10000];

        fprintf(stderr, "Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(program, full_device_ids[device_num], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        fprintf(stderr, "%s\n", buffer);

        clReleaseProgram(program);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] prefix_arr_opencl;
        delete[] device_names;
        delete[] device_types;
        free(full_device_ids);
        exit(1);
    }

    cl_mem prefix_buf_in = NULL;
    cl_mem prefix_buf_out = NULL;
    cl_mem work_group_sum_buf = NULL;
    auto start_time = chrono::steady_clock::now();


    size_t global_work_size = size;
    size_t local_work_size = WORK_GROUP_SIZE;
    size_t groups_count = global_work_size/ local_work_size;

    if (global_work_size % local_work_size != 0) {
        global_work_size -=( global_work_size % local_work_size - local_work_size);
        groups_count += 1;
    }

   
    prefix_buf_in      = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * size, NULL, &error);
    if (!prefix_buf_in)
    {
        fprintf(stderr, "Error: Failed to allocate device memory!\n");

      
      
        clReleaseProgram(program);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_types;
        delete[] device_names;
        delete[] prefix_arr_opencl;
        free(full_device_ids);
        exit(1);
    }
    prefix_buf_out     = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * size, NULL, &error);
    if (!prefix_buf_out)
    {
        fprintf(stderr, "Error: Failed to allocate device memory!\n");

        clReleaseMemObject(prefix_buf_in);
       
        clReleaseProgram(program);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_types;
        delete[] device_names;
        delete[] prefix_arr_opencl;
        free(full_device_ids);
        exit(1);
    }
    work_group_sum_buf = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * groups_count, NULL, &error);
    if (!work_group_sum_buf)
    {
        fprintf(stderr, "Error: Failed to allocate device memory!\n");

        clReleaseMemObject(prefix_buf_in);
        clReleaseMemObject(prefix_buf_out);
        clReleaseProgram(program);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_types;
        delete[] device_names;
        delete[] prefix_arr_opencl;
        free(full_device_ids);
        exit(1);
    }



    kernel = clCreateKernel(program, "prefix_sum", &error);
    if (!kernel || error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to create compute kernel2!\n");
        clReleaseMemObject(prefix_buf_in);
        clReleaseMemObject(prefix_buf_out);
        clReleaseMemObject(work_group_sum_buf);
        clReleaseProgram(program);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_names;
        delete[] device_types;
        delete[] prefix_arr_opencl;
        free(full_device_ids);
        exit(1);
    }

    kernel_group_sum = clCreateKernel(program, "sum_from_groups", &error);
    if (!kernel || error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to create compute kernel1!\n");
        clReleaseMemObject(prefix_buf_in);
        clReleaseMemObject(prefix_buf_out);
        clReleaseMemObject(work_group_sum_buf);
        clReleaseProgram(program);
        clReleaseKernel(kernel);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_names;
        delete[] device_types;
        delete[] prefix_arr_opencl;
        free(full_device_ids);
        exit(1);
    }
    

    error = clEnqueueWriteBuffer(commands, prefix_buf_in, CL_TRUE, 0, sizeof(cl_float) * size, prefix_arr, 0, NULL, NULL);
   
    if (error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to write to source array!\n");
        clReleaseMemObject(prefix_buf_in);
        clReleaseMemObject(prefix_buf_out);
        clReleaseMemObject(work_group_sum_buf);
        clReleaseProgram(program);
        clReleaseKernel(kernel);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_names;
        delete[] prefix_arr_opencl;
        delete[] device_types;
        free(full_device_ids);
        exit(1);
    }
    error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &prefix_buf_in);
    error = clSetKernelArg(kernel, 1, sizeof(cl_mem), &prefix_buf_out);
    error = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&work_group_sum_buf);
    error = clSetKernelArg(kernel_group_sum, 0, sizeof(cl_mem), &prefix_buf_out);
    error = clSetKernelArg(kernel_group_sum, 1, sizeof(cl_mem), (void*)&work_group_sum_buf);
    if (error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to set kernel arguments! %d\n", error);
        clReleaseMemObject(prefix_buf_in);
        clReleaseMemObject(prefix_buf_out);
        clReleaseMemObject(work_group_sum_buf);
        clReleaseProgram(program);
        clReleaseKernel(kernel);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_names;
        delete[] prefix_arr_opencl;
        delete[] device_types;
        free(full_device_ids);
        exit(1);
    }
   
    auto start_host = chrono::steady_clock::now();

    error = clEnqueueNDRangeKernel(commands, kernel_group_sum, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
    error = clEnqueueNDRangeKernel(commands, kernel_group_sum, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);

    auto end_host = chrono::steady_clock::now();

    error |= clEnqueueReadBuffer(commands, prefix_buf_out, CL_TRUE, 0, sizeof(cl_float) * size, prefix_arr_opencl, 0, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to read buffer! %d\n", error);
        clReleaseMemObject(prefix_buf_in);
        clReleaseMemObject(prefix_buf_out);
        clReleaseMemObject(work_group_sum_buf);
        clReleaseProgram(program);
        clReleaseKernel(kernel);
        clReleaseKernel(kernel_group_sum);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_names;
        delete[] prefix_arr_opencl;
        delete[] device_types;
        free(full_device_ids);
        exit(1);
    }

    auto end_time = chrono::steady_clock::now();


    auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    auto time_ms2 = chrono::duration_cast<chrono::milliseconds>(end_host - start_host);
    cout << "Time:  " << time_ms.count() << "ms" << "\t" << time_ms2.count() << "ms" << "\n";
    //for (int j = 0; j < size; j++) {
    //printf("%f", prefix_arr_opencl[j]);
    //printf("\n");
    //}
    printf("local work size = %i", WORK_GROUP_SIZE);
    clFinish(commands);

    clReleaseMemObject(prefix_buf_in);
    clReleaseMemObject(prefix_buf_out);
    clReleaseMemObject(work_group_sum_buf);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseKernel(kernel_group_sum);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);
    delete[] device_names;

    delete[] device_types;
    free(full_device_ids);

    return prefix_arr_opencl;
}



int main(int argc, char* argv[]) {

       if (argc < 4) {

          fprintf(stderr, "Enter the arguments in the format: program.exe device_num input_file.txt output_file.txt ");
          return 1;
      }
      int device_num = atoi(argv[1]);
      string r_filename = argv[2];

      string w_filename = argv[3];


    ifstream fin;
    //int device_num = 1;
    //string r_filename = "in.txt";
    //string w_filename = "out.txt";


    int size = 0;
    int counter = 0;
    float tmp = 0;

    if (device_num < 0) {
        device_num == 0;
    }

    fin.open(r_filename);

    if (!(fin.is_open())) {
        fprintf(stderr, "Cannot open file");

        return 1;
    }

    fin >> size;
    if (fin.fail()) {
        fprintf(stderr, "Wrong file format");
        fin.close();
        return 1;
    }

    if (size == 0 || size < 0) {
        fprintf(stderr, "The size of the array will have to be greater than zero");
        fin.close();
        return 1;
    }


    float* arr = new(nothrow) float[size];
    if (arr == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        fin.close();
        return 1;

    }
   


    for (int i = 0; i < size; i++) {
        fin >> tmp;
        counter++;
        if (fin.fail() || (fin.eof() && (counter < (size)))) {
            fprintf(stderr, "Wrong file format");
            delete[] arr;
            fin.close();
            return 1;
        }
        else {
            arr[i] = tmp;
           
        }
    }

    fin.close();

    float* prefix_sum = new(nothrow) float[size];
    if (prefix_sum == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        fin.close();
        return 1;

    }
 
    arr = prefix_sum_opencl(arr, size, device_num);
    

    ofstream fout;
    fout.open(w_filename);

    if (!(fout.is_open())) {
        fprintf(stderr, "Cannot open file");
        delete[] arr;
        return 1;
    }


    for (int i = 0; i < size; i++) {
        float a = arr[i];
        fout << arr[i] << " ";
    }
    fout << endl;
    fout.close();

    delete[] arr;


    return 0;
}
