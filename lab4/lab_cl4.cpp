#include <stdio.h>
#include <stdlib.h>
#define _CRT_SECURE_NO_WARNINGS
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#pragma comment(lib, "opencl.lib")
#endif

using namespace std;
#define MAX_SOURCE_SIZE 4096

const string clSrc =
"__kernel /*void*/ matrix_mult ( __global float* matrix,__global float* matrix1,__global float* matrix2, int n, int k, int m ) {     \r\n"
"  float tmp = 0.0;														                    \r\n"
"  for (int i = 0; i < m; i++) {															\r\n"
"    for (int j = 0; j < n; j++) {															\r\n"
"           matrix[i * n + j] = 0;															\r\n"
"       for (int f = 0; f < k; f++) {														\r\n"
"           tmp = matrix1[i * k + f] * matrix2[f * n + j];									\r\n"
"			matrix[i * n + j] += tmp;														\r\n"
"		}																					\r\n"
"    }																				     	\r\n"
"  }																						\r\n"
"}																							\r\n";

char* fileToString(const char* filename) {
	
	long input_file_size;
	FILE* input_file = fopen(filename, "rb");
	fseek(input_file, 0, SEEK_END);
	input_file_size = ftell(input_file);
	rewind(input_file);
	char*  ret = (char*)malloc(input_file_size * (sizeof(char)));
	fread(ret, sizeof(char), input_file_size, input_file);
	fclose(input_file);
	return ret;
}



int get_platforms() {
	cl_int err;
	cl_int ret;
	cl_uint platform_num;
	cl_uint device_num;

	ret = clGetPlatformIDs(0, NULL, &platform_num);
	if (!platform_num)
	{
		printf("Number of platforms: 0\n");
		return 0;
	}

	cl_platform_id* platform_ids = (cl_platform_id*)malloc(sizeof(cl_platform_id) * platform_num);
	clGetPlatformIDs(platform_num, platform_ids, NULL);

	for (cl_uint i = 0; i < platform_num; ++i)
	{
		size_t pn_size;
		clGetPlatformInfo(platform_ids[i], CL_PLATFORM_VENDOR, 0, NULL, &pn_size);
		char* platform_name = (char*)malloc(sizeof(char) * pn_size);
		clGetPlatformInfo(platform_ids[i], CL_PLATFORM_VENDOR, pn_size, platform_name, 0);

		cl_uint device_num;
		clGetDeviceIDs(platform_ids[i], CL_DEVICE_TYPE_ALL, 0, NULL, &device_num);

		if (!device_num)
		{
			continue;
		}

		cl_device_id* device_ids = (cl_device_id*)malloc(sizeof(cl_device_id) * device_num);
		clGetDeviceIDs(platform_ids[i], CL_DEVICE_TYPE_ALL, device_num,
			device_ids, NULL);

		for (cl_uint d = 0; d < device_num; ++d)
		{
			size_t size;
			clGetDeviceInfo(device_ids[d], CL_DEVICE_NAME, 0, NULL, &size);
			char* name = (char*)malloc(sizeof(char) * size);
			clGetDeviceInfo(device_ids[d], CL_DEVICE_NAME, size, name, 0);

			printf("platform[%i]: %s\tdevice[%i/%i]: %s\n",
				i, platform_name, d, device_num, name);
			free(name);
		}
		free(device_ids);
		free(platform_name);
		printf("\n");
	}
	free(platform_ids);
}


void matrix_mult(float* matrix,float* matrix1, float* matrix2, int n, int k,int m ) {
	
	float tmp = 0.0;

	for (int i = 0; i < m; i++) {
		for (int j = 0; j < n; j++) {
			matrix[i * n + j] = 0;
			for (int f = 0; f < k; f++) {
				tmp = matrix1[i * k + f] * matrix2[f * n + j];
				matrix[i*n + j] += tmp;
			}
		}
	}
	
	
}

cl_device_id create_device() {
	cl_platform_id platform;
	cl_device_id dev;
	cl_int err = 0;

	err |= clGetPlatformIDs(1, &platform, NULL);
	err |= clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
	if (err == CL_DEVICE_NOT_FOUND) {
		err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
	}
	if (err) throw;
	return dev;
}

int align(int x, int y) {
	return (x + y - 1) / y * y;
}

void initOpenCl(int n, int k, int m, float* matrix1, float* matrix2, float* matrix) {

	//cl_device_id device = create_device();
	
	cl_platform_id platform_id;
	cl_uint ret_num_platforms;
	cl_int ret;
	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);

	cl_device_id device_id;
	cl_uint ret_num_devices;
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
	if (ret != CL_SUCCESS)
	{
		printf("Error: Failed to create a device group!\n");
		exit(1);
		
	}
	cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
	if (!context)
	{
		printf("Error: Failed to create a compute context!\n");
		exit(1);
		
	}




	cl_program program = NULL;
	cl_kernel kernel = NULL;

	FILE* fp;
	const char fileName[] = "source.c";
	size_t source_size;
	char* source_str;
	int i;

	fp = fopen(fileName, "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);

	/* создать бинарник из кода программы */
	program = clCreateProgramWithSource(context, 1, (const char**)&source_str, (const size_t*)&source_size, &ret);

	/* скомпилировать программу */
	//ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	ret = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		size_t len;
		char buffer[2048];

		printf("Error: Failed to build program executable!\n");
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);

		exit(1);
	}


	 kernel = clCreateKernel(program, "matrix_mult", &ret);
	if (!kernel || ret != CL_SUCCESS)
	{
		printf("Error: Failed to create compute kernel!\n");
		exit(1);
	}

	cl_command_queue commands = clCreateCommandQueueWithProperties(context, device_id, 0, &ret);
	if (!commands)
	{
		printf("Error: Failed to create a command commands!\n");
		exit(1);

	}

	cl_mem matrix_buf1 = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * m * k, NULL, &ret);
	cl_mem matrix_buf2 = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * k * n, NULL, &ret);
	cl_mem matrix_buf = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * m * n, NULL, &ret);
	if (!matrix_buf1 || !matrix_buf2 || !matrix_buf)
	{
		printf("Error: Failed to allocate device memory!\n");
		exit(1);
	}



	ret = clEnqueueWriteBuffer(commands, matrix_buf1, CL_TRUE, 0, sizeof(float) * m * k, matrix1, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error: Failed to write to source array!\n");
		exit(1);
	}
	ret = clEnqueueWriteBuffer(commands, matrix_buf2, CL_TRUE, 0, sizeof(float) * k * n, matrix2, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error: Failed to write to source array!\n");
		exit(1);
	}
	ret = clEnqueueWriteBuffer(commands, matrix_buf, CL_TRUE, 0, sizeof(float) * m * n, matrix, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		printf("Error: Failed to write to source array!\n");
		exit(1);
	}


	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&matrix_buf1);
	ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&matrix_buf2);
	ret |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&matrix_buf);
	if (ret != CL_SUCCESS)
	{
		printf("Error: Failed to set kernel arguments! %d\n", ret);
		exit(1);
	}
	size_t work_size[1] = { m };

	size_t local_size[2] = { 256,1 };
	size_t global_size[2] = { align(m,local_size[0]),align(n,local_size[1]) };
	//ret = clEnqueueNDRangeKernel(commands, kernel, 1, NULL, work_size, NULL, 0, NULL, NULL);
	ret = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, global_size, local_size, 0, NULL, NULL);

	ret = clEnqueueReadBuffer(commands, matrix_buf, CL_TRUE, 0, sizeof(cl_float) * m * n, matrix, 0, NULL, NULL);

	clFinish(commands);
	for (int i = 0; i < m * n; i++) {
		printf("%f", matrix[i]);
	}
	
	clReleaseMemObject(matrix_buf);
	clReleaseMemObject(matrix_buf1);
	clReleaseMemObject(matrix_buf2);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);


}




int main(int argc, char* argv[])
{
	//if (argc < 5) {
	//	
	//	fprintf(stderr, "Enter the arguments in the format: program.exe device_num input_file.txt output_file.txt method");
	//	return 1;
	//}
	/*int device_num = argv[1];
	string r_filename = argv[2];

	string w_filename = argv[3];

	int method = atoi(argv[4]);*/

	ifstream fin;

	int device_num = 0;
	string r_filename  = "in.txt";
	string w_filename = "out.txt";
	int method = 1;

	if (device_num < 0 || device_num > 2) {
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
		
		return 1;
	}
	

	float* matrix1 = new(nothrow) float[m * k];
	if (matrix1 == nullptr) {
		fprintf(stderr, "Memory cannot be allocated");
		
		return 1;

	}
	float* matrix2 = new(nothrow) float[k * n];
	if (matrix2 == nullptr) {
		fprintf(stderr, "Memory cannot be allocated");
		
		return 1;

	}

	float* matrix = new(nothrow) float[m * n];
	if (matrix == nullptr) {
		fprintf(stderr, "Memory cannot be allocated");

		return 1;

	}

	float* result = new(nothrow) float[m * n];
	if (result == nullptr) {
		fprintf(stderr, "Memory cannot be allocated");

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
				fprintf(stderr, "Wrong file format");	
				return 1;
			}
			else {
				matrix2[i * n + j] = tmp;
			}

		}
	}

	fin.close();

	if (method == 0) {
		matrix_mult(matrix, matrix1, matrix2, n, k, m);
	}
	else if (method == 1) {

		initOpenCl(n,k,m,matrix1,matrix2,matrix);

	}
	else if (method == 2) {

	}
	else {
		fprintf(stderr, "Enter the method num from 0 to 2");
	}
	





	//Запись в файл
	ofstream fout;

	fout.open(w_filename);
	if (!(fout.is_open())) {
		fprintf(stderr, "Cannot open file");
		return 1;
	}

	fout << n <<" "<< m << endl;
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
	delete[] result;
	delete[] matrix;
	delete[] matrix1;
	delete[] matrix2;

	fout.close();

	
	
	return 0;
}
