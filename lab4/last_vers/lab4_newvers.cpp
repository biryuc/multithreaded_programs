#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
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


void get_platforms(cl_device_id* full_device_ids, string* device_names, map<int, int> &types,int &cnt,map<int,int> &tile) {
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
            free(platform_ids);
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





float* matrix_mult_opencl_tile(float* matrix1, float* matrix2, int n, int k, int m, int device_num,int method) {

    cl_platform_id platform_id;
    cl_uint error_num_platforms;
    cl_int error;
    cl_device_id device_id;
    cl_uint error_num_devices;
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    map<int, int> types;
    map<int, int> tile;

    int SIZE = 20;
    int cnt = 0;
    int TILE_SIZE = 1;
    
    auto start_time = chrono::steady_clock::now();

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
    get_platforms(full_device_ids, device_names, types, cnt,tile);
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
            if ( l < 0) {
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

    cl_command_queue commands = clCreateCommandQueue(context, full_device_ids[device_num], CL_QUEUE_PROFILING_ENABLE, &error);

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
    std::string src = " ";
    if (method == 1) {
        src = get_program_text("source.cl");
    }
    else if (method == 2) {
        src = get_program_text("local_memory.cl");
    }
    else if (method== 3) {
       src = get_program_text("vector_local.cl");
    }
   // std::string src = get_program_text("local_memory.cl");
   // std::string src = get_program_text("vector_local.cl");
    const char* src_text = src.data();
    const size_t src_length = src.size();


    program = clCreateProgramWithSource(context, 1, &src_text, &src_length, &error);

    std::string build_option = "-DTILE_SIZE=" + std::to_string(TILE_SIZE);
    error = clBuildProgram(program, 1, &full_device_ids[device_num], build_option.c_str(), NULL, NULL);


    if (error != CL_SUCCESS)
    {
        size_t logSize = 0;
        clGetProgramBuildInfo(program, full_device_ids[device_num], CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);

        char* buffer = (char*)malloc(sizeof(char) * logSize);
        clGetProgramBuildInfo(program, full_device_ids[device_num], CL_PROGRAM_BUILD_LOG, logSize, buffer, nullptr);

        fprintf(stderr, "Error: Failed to build program executable!\n");
        fprintf(stderr, "%s\n", buffer);

        clReleaseProgram(program);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] matrix_opencl;
        delete[] device_names;
        delete[] device_types;
        free(full_device_ids);
        free(buffer);
        exit(1);
    }

    cl_mem matrix_buf1 = NULL;
    cl_mem matrix_buf2 = NULL;
    cl_mem matrix_buf = NULL;
   /* cl_mem size_buf = NULL;
    cl_int sizes[1] = { TILE_SIZE };*/



    matrix_buf1 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * m * k, NULL, &error);
    if (!matrix_buf1)
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
    matrix_buf2 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * k * n, NULL, &error);
    if (!matrix_buf2)
    {
        fprintf(stderr, "Error: Failed to allocate device memory!\n");


        clReleaseMemObject(matrix_buf1);
        clReleaseProgram(program);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_types;
        delete[] device_names;
        delete[] matrix_opencl;
        free(full_device_ids);
        exit(1);
    }

    matrix_buf = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * n * m, NULL, &error);
    if (!matrix_buf)
    {
        fprintf(stderr, "Error: Failed to allocate device memory!\n");

        clReleaseMemObject(matrix_buf1);
        clReleaseMemObject(matrix_buf2);
        //clReleaseMemObject(size_buf);
        clReleaseProgram(program);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        delete[] device_types;
        delete[] device_names;
        delete[] matrix_opencl;
        free(full_device_ids);
        exit(1);
    }



    kernel = clCreateKernel(program, "matrix_mult", &error);
    if (!kernel || error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to create compute kernel!\n");
        clReleaseMemObject(matrix_buf);
        clReleaseMemObject(matrix_buf1);
        clReleaseMemObject(matrix_buf2);
        //clReleaseMemObject(size_buf);
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
   // error |= clEnqueueWriteBuffer(commands, size_buf, CL_TRUE, 0, 1 * sizeof(cl_int), &sizes, 0, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to write to source array!\n");
        clReleaseMemObject(matrix_buf);
        clReleaseMemObject(matrix_buf1);
        clReleaseMemObject(matrix_buf2);
       // clReleaseMemObject(size_buf);
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
   // error |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &size_buf);
    if (error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to set kernel arguments! %d\n", error);
        clReleaseMemObject(matrix_buf);
        clReleaseMemObject(matrix_buf1);
        clReleaseMemObject(matrix_buf2);
       // clReleaseMemObject(size_buf);
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




    cl_event kernel_event;

    error = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, global_work_size, local_work_size, 0, NULL, &kernel_event);




    error |= clEnqueueReadBuffer(commands, matrix_buf, CL_TRUE, 0, sizeof(cl_float) * n * m, matrix_opencl, 0, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        fprintf(stderr, "Error: Failed to read buffer! %d\n", error);
        clReleaseMemObject(matrix_buf);
        clReleaseMemObject(matrix_buf1);
        clReleaseMemObject(matrix_buf2);
        //clReleaseMemObject(size_buf);
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


    // ВЫВОД ВРЕМЕНИ
    cl_ulong begin = NULL, end = NULL;
    cl_ulong time_ns = 0;

   

    if (clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &begin, NULL) != CL_SUCCESS) {
        std::cerr << "Can not get profiling info" << std::endl;
        clReleaseMemObject(matrix_buf);
        clReleaseMemObject(matrix_buf1);
        clReleaseMemObject(matrix_buf2);
        //clReleaseMemObject(size_buf);
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
    if (clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL) != CL_SUCCESS) {
        std::cerr << "Can not get profiling info" << std::endl;
        clReleaseMemObject(matrix_buf);
        clReleaseMemObject(matrix_buf1);
        clReleaseMemObject(matrix_buf2);
        //clReleaseMemObject(size_buf);
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
    time_ns += end - begin;

    auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    cout << "Time:  " << time_ms.count() <<"ms" << "\t" << time_ns/1000 << "ms" << "\n";
        //for (int j = 0; j < n*m; j++) {
        //printf("%f", matrix_opencl[j]);
        //printf("\n");
        //}
    if (method == 2 || method == 3) {
        printf("local work size = %i", TILE_SIZE);
    }
   
    clFinish(commands);

    clReleaseMemObject(matrix_buf);
    clReleaseMemObject(matrix_buf1);
    clReleaseMemObject(matrix_buf2);
    //clReleaseMemObject(size_buf);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);
    delete[] device_names;

    delete[] device_types;
    free(full_device_ids);

    return matrix_opencl;
}

void matrix_mult(float* matrix, float* matrix1, float* matrix2, int n, int k, int m) {

    float tmp = 0.0;

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i * n + j] = 0;
            for (int f = 0; f < k; f++) {
                tmp = matrix1[i * k + f] * matrix2[f * n + j];
                matrix[i * n + j] += tmp;
            }
        }
    }


}

int main(int argc, char* argv[]) {
    if (argc < 5) {
    	
    	fprintf(stderr, "Enter the arguments in the format: program.exe device_num input_file.txt output_file.txt method");
    	return 1;
    }
    int device_num = atoi(argv[1]);
    string r_filename = argv[2];

    string w_filename = argv[3];

    int method = atoi(argv[4]);
    ifstream fin;
    //int device_num = 0;
    //string r_filename = "in.txt";
    //string w_filename = "out.txt";
    //int method = 3;

    

    if (device_num < 0 ) {
        device_num == 0;
    }

    fin.open(r_filename);

    if (!(fin.is_open())) {
        fprintf(stderr, "Cannot open file");

        return 1;
    }
    int n;
    int k;
    int m;
    
    float tmp;
    int counter = 0;

    fin >> n >> k >> m;
    if (fin.fail()) {
        fprintf(stderr, "Wrong file format");
        fin.close();
        return 1;
    }


    float* matrix1 = new(nothrow) float[m * k];
    if (matrix1 == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        fin.close();
        return 1;

    }
    float* matrix2 = new(nothrow) float[k * n];
    if (matrix2 == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        delete[] matrix1;
        fin.close();
        return 1;

    }

    float* matrix = new(nothrow) float[m * n];
    if (matrix == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        delete[] matrix1;
        delete[] matrix2;
        fin.close();
        return 1;

    }

    float* result = new(nothrow) float[m * n];
    if (result == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        delete[] matrix1;
        delete[] matrix2;
        delete[] matrix;
        fin.close();
        return 1;

    }

    for (int i = 0; i < m * n; i++) {
        matrix[i] = 0;
    }

    //Читаем первую матрицу
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < k; j++) {
            fin >> tmp;
            counter++;
            if (fin.fail() || (fin.eof() && (counter < (m * k)))) {
                delete[] matrix1;
                delete[] matrix2;
                delete[] matrix;
                delete[] result;
                fin.close();
                fprintf(stderr, "Wrong file format");
                return 1;
            }
            else {
                matrix1[i * k + j] = tmp;
            }

        }
    }

    //Читаем вторую матрицу
    counter = 0;
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < n; j++) {
            fin >> tmp;
            counter++;
            if (fin.fail() || (fin.eof() && (counter < (k * n)))) {
                delete[] matrix2;
                delete[] matrix1;
                delete[] matrix;
                delete[] result;
                fin.close();
                fprintf(stderr, "Wrong file format");
                return 1;
            }
            else {
                matrix2[i * n + j] = tmp;
            }

        }
    }

    fin.close();



   

    if (method == 1 || method == 2 || method == 3) {

        matrix = matrix_mult_opencl_tile(matrix1, matrix2, n, k, m, device_num, method);
    }
    else {
        fprintf(stderr, "Enter the method num from 1 to 3");
        delete[] result;
        delete[] matrix;
        delete[] matrix1;
        delete[] matrix2;
        return 1;
    }


        //Запись в файл
        ofstream fout;

        fout.open(w_filename);
        if (!(fout.is_open())) {
            fprintf(stderr, "Cannot open file");
            return 1;
        }

        fout << n << " " << m << endl;
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                tmp = matrix[i * n + j];
                fout << matrix[i * n + j];
                if (j != n - 1) {
                    fout << " ";
                }
            }
            fout << endl;
        }


        fout.close();
    

    delete[] result;
    delete[] matrix;
    delete[] matrix1;
    delete[] matrix2;

    


    return 0;
}
