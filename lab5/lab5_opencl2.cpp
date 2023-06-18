#include "CL/cl.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <malloc.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#pragma comment(lib, "opencl.lib")
#endif
#include <map>
#include <set>
using namespace std;


#define _CRT_SECURE_NO_WARNINGS
#define MAX_SOURCE_SIZE 4096



using namespace std;


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


    // string* device_names = new string[SIZE];
     //int counter = 0;
    int cnt_discrete = 0;
    int cnt_integrete = 0;
    int cnt_cpu = 0;


    // cl_device_id* full_device_ids = (cl_device_id*)malloc(sizeof(cl_device_id) * SIZE);


    ret = clGetPlatformIDs(0, NULL, &platform_num);
    if (!platform_num)
    {
        printf("Number of platforms: 0\n");

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



float* matrix_mult_opencl_tile(float* matrix1, float* matrix2, int n, int k, int m, int device_num) {

    cl_platform_id platform_id;
    cl_uint error_num_platforms;
    cl_int error;
    cl_device_id device_id;
    cl_uint error_num_devices;
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    map<int, int> types;
    map<int, int> tile;
    int* counter = new int[1];
    // counter[0] = 0;
    int SIZE = 20;
    int cnt = 0;
    int TILE_SIZE = 1;


    string* device_names = new string[SIZE];

    if (device_names == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");

        exit(1);

    }
    int* device_types = new int[SIZE];
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
    else if (cnt == 0) {
        delete[] device_names;
        delete[] device_types;
        free(full_device_ids);
        printf("Number of devices: 0\n");
        exit(1);
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

    cl_float* matrix_opencl = new(nothrow) cl_float[m * n];
    if (matrix_opencl == nullptr) {
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
        delete[] matrix_opencl;
        delete[] device_types;
        free(full_device_ids);
        exit(1);

    }
    cl_command_queue commands = clCreateCommandQueueWithProperties(context, full_device_ids[device_num], 0, &error);

    if (error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to create commands!\n");
        delete[] matrix_opencl;
        clReleaseContext(context);
        delete[] device_names;
        delete[] device_types;
        free(full_device_ids);
        exit(1);
    }


    std::string src = get_program_text("local_memory.cl");
    const char* src_text = src.data();
    const size_t src_length = src.size();


    program = clCreateProgramWithSource(context, 1, &src_text, &src_length, &error);

    std::string build_option = "-DTILE_SIZE=" + std::to_string(TILE_SIZE);
    error = clBuildProgram(program, 1, &full_device_ids[device_num], build_option.c_str(), NULL, NULL);


    if (error != CL_SUCCESS)
    {
        size_t len;
        char buffer[2048];

        fprintf(stderr, "Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(program, full_device_ids[device_num], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        fprintf(stderr, "%s\n", buffer);

        clReleaseProgram(program);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] matrix_opencl;
        delete[] device_names;
        delete[] device_types;
        free(full_device_ids);
        exit(1);
    }

    cl_mem matrix_buf1 = NULL;
    cl_mem matrix_buf2 = NULL;
    cl_mem matrix_buf = NULL;
    cl_mem size_buf = NULL;
    cl_int sizes[3] = { m, k, n };

    auto start_time = chrono::steady_clock::now();

    matrix_buf1 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * m * k, NULL, &error);
    matrix_buf2 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * k * n, NULL, &error);
    size_buf = clCreateBuffer(context, CL_MEM_READ_WRITE, 3 * sizeof(cl_int), NULL, &error);
    matrix_buf = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * n * m, NULL, &error);
    if (!matrix_buf1 || !matrix_buf2 || !matrix_buf || !size_buf)
    {
        fprintf(stderr, "Error: Failed to allocate device memory!\n");


        clReleaseProgram(program);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_types;
        delete[] device_names;
        delete[] matrix_opencl;
        free(full_device_ids);
        exit(1);
    }



    kernel = clCreateKernel(program, "matrix_mult_tile", &error);
    if (!kernel || error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to create compute kernel!\n");
        clReleaseMemObject(matrix_buf);
        clReleaseMemObject(matrix_buf1);
        clReleaseMemObject(matrix_buf2);
        clReleaseMemObject(size_buf);
        clReleaseProgram(program);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_names;
        delete[] device_types;
        delete[] matrix_opencl;
        free(full_device_ids);
        exit(1);
    }

    error = clEnqueueWriteBuffer(commands, matrix_buf1, CL_TRUE, 0, sizeof(cl_float) * m * k, matrix1, 0, NULL, NULL);
    error |= clEnqueueWriteBuffer(commands, matrix_buf2, CL_TRUE, 0, sizeof(cl_float) * n * k, matrix2, 0, NULL, NULL);
    error |= clEnqueueWriteBuffer(commands, size_buf, CL_TRUE, 0, 3 * sizeof(cl_int), sizes, 0, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to write to source array!\n");
        clReleaseMemObject(matrix_buf);
        clReleaseMemObject(matrix_buf1);
        clReleaseMemObject(matrix_buf2);
        clReleaseMemObject(size_buf);
        clReleaseProgram(program);
        clReleaseKernel(kernel);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_names;
        delete[] matrix_opencl;
        delete[] device_types;
        free(full_device_ids);
        exit(1);
    }
    error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &matrix_buf1);
    error |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &matrix_buf2);
    error |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &matrix_buf);
    error |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &size_buf);
    if (error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to set kernel arguments! %d\n", error);
        clReleaseMemObject(matrix_buf);
        clReleaseMemObject(matrix_buf1);
        clReleaseMemObject(matrix_buf2);
        clReleaseMemObject(size_buf);
        clReleaseProgram(program);
        clReleaseKernel(kernel);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_names;
        delete[] matrix_opencl;
        delete[] device_types;
        free(full_device_ids);
        exit(1);
    }
    const size_t global_work_size[3] = { static_cast<size_t>(n), static_cast<size_t>(m), static_cast<size_t>(k) };
    const size_t local_work_size[3] = { static_cast<size_t>(TILE_SIZE),static_cast<size_t>(TILE_SIZE), 1 };


    auto start_host = chrono::steady_clock::now();

    error = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, global_work_size, local_work_size, 0, NULL, NULL);


    auto end_host = chrono::steady_clock::now();

    error |= clEnqueueReadBuffer(commands, matrix_buf, CL_TRUE, 0, sizeof(cl_float) * n * m, matrix_opencl, 0, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to read buffer! %d\n", error);
        clReleaseMemObject(matrix_buf);
        clReleaseMemObject(matrix_buf1);
        clReleaseMemObject(matrix_buf2);
        clReleaseMemObject(size_buf);
        clReleaseProgram(program);
        clReleaseKernel(kernel);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_names;
        delete[] matrix_opencl;
        delete[] device_types;
        free(full_device_ids);
        exit(1);
    }

    auto end_time = chrono::steady_clock::now();


    auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    auto time_ms2 = chrono::duration_cast<chrono::milliseconds>(end_host - start_host);
    cout << "Time:  " << time_ms.count() << "ms" << "\t" << time_ms2.count() << "ms" << "\n";
    //for (int j = 0; j < n*m; j++) {
    //printf("%f", matrix_opencl[j]);
    //printf("\n");
    //}
    printf("local work size = %i", TILE_SIZE);
    clFinish(commands);

    clReleaseMemObject(matrix_buf);
    clReleaseMemObject(matrix_buf1);
    clReleaseMemObject(matrix_buf2);
    clReleaseMemObject(size_buf);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);
    delete[] device_names;

    delete[] device_types;
    free(full_device_ids);

    return matrix_opencl;
}


void prefix_sum(float*& arr, int size) {
    float tmp = arr[size - 2];

    float t = 0;
    int n = log2(size);
    for (int i = 0; i < n-1; ++i) {
        for (int j = 0; j < size - 1; j +=pow(2,i+1)) {
            arr[(int)(j + pow(2,i+1) - 1)] = arr[(int)(j + pow(2,i) - 1)] + arr[(int)(j + pow(2,i+1) - 1)];
        }

    }
    float tmp_new = 0;
    for (int k = 0; k < size+1; k++) {
        tmp_new = arr[k];
        printf("%f", arr[k]);
    }
    arr[size - 2] = 0;
    for (int i = n - 1; i >= 0; --i) {
        for (int j = 0; j < size - 1; j +=pow(2, i+1)) {
            t = arr[(int)(j + pow(2,i) - 1)];
            arr[(int)(j +pow(2,i))] = arr[(int)(j + pow(2,i+1) - 1)];
            arr[(int)(j +pow(2,i+1) - 1)] = t + arr[(int)(j + pow(2,i+1) - 1)];
        }

    }
    printf("\n");
    arr[size-1] = arr[size - 2] + tmp;
    for (int k = 0; k < size + 1; k++) {
        tmp_new = arr[k];
        printf("%f", arr[k]);
    }
}



int main(int argc, char* argv[]) {
    //if (argc < 4) {

    //    fprintf(stderr, "Enter the arguments in the format: program.exe device_num input_file.txt output_file.txt ");
    //    return 1;
    //}
    //int device_num = atoi(argv[1]);
    //string r_filename = argv[2];

    //string w_filename = argv[3];

    
    ifstream fin;
    int device_num = 1;
    string r_filename = "in.txt";
    string w_filename = "out.txt";
   
   
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

    if (size == 0 || size<0) {
        fprintf(stderr, "The size of the array will have to be greater than zero");
        fin.close();
        return 1;
    }
    

    float* arr = new(nothrow) float[size+1];
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
    arr[size] = 0;

    fin.close();


    prefix_sum(arr, size);

    //prefix_sum_opencl(matrix1, matrix2, n, k, m, device_num);




     ofstream fout;
     fout.open(w_filename);
    
     if (!(fout.is_open())) {
         fprintf(stderr, "Cannot open file");
         delete[] arr;
         return 1;
     }


     for (int i = 1; i < size+1; i++) {
         fout << arr[i] << " ";
     }
     fout << endl;
     fout.close();

     delete[] arr;


    return 0;
}
