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


const string clSrc =
"void matrix_mult(float* matrix, float* matrix1, float* matrix2, int n, int k, int m) {     \r\n"
"  float tmp = 0.0;														                    \r\n"
"  for (int i = 0; i < m; i++) {															\r\n"
"    for (int j = 0; j < n; j++) {															\r\n"
"           matrix[i * n + j] = 0;															\r\n"
"       for (int f = 0; f < k; f++) {														\r\n"
"           tmp = matrix1[i * k + f] * matrix2[f * n + j];									\r\n"
"			matrix[i * n + j] += tmp;														\r\n"
"		}																					\r\n"
	"}																						\r\n"
"	}																						\r\n"
"}																							\r\n";



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


int initAllDataGPU(int n,int k,int m )  {

	cl_uint numPlatforms;
	
	//get Platform
	clGetPlatformIDs(0, NULL, &numPlatforms);
	cl_platform_id* platform_ids = (cl_platform_id*)malloc(sizeof(cl_platform_id) * numPlatforms);
	cl_device_id device_id;
	cl_context context;
	
	int err = clGetPlatformIDs(numPlatforms, platform_ids, NULL);

	//get Device
	err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to create a device group!\n");
		return EXIT_FAILURE;
	}

	//create context
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	if (!context)
	{
		printf("Error: Failed to create a compute context!\n");
		return EXIT_FAILURE;
	}


	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&clSrc, NULL, &err);
	if (!program)
	{
		printf("Error: Failed to create compute program!\n");
		return EXIT_FAILURE;
	}


	// Build the program executable
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		size_t len;
		char buffer[2048];

		printf("Error: Failed to build program executable!\n");
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		exit(1);
	}

	// Create the compute kernel in the program we wish to run
	//
	cl_kernel kernel = clCreateKernel(program, "allToOne", &err);
	if (!kernel || err != CL_SUCCESS)
	{
		printf("Error: Failed to create compute kernel!\n");
		exit(1);
	}

	cl_mem matrix1 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * m * k, NULL, NULL);
	cl_mem matrix2 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * k * n, NULL, NULL);
	cl_mem matrix =  clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * m * n, NULL, NULL);
	if (!matrix1 || !matrix2 || !matrix)
	{
		printf("Error: Failed to allocate device memory!\n");
		exit(1);
	}
	
	err = 0;
	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &matrix1);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &matrix2);
	err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &matrix);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to set kernel arguments! %d\n", err);
		exit(1);
	}



	//////--- пишем сгенерированные матрицы в буферы девайса
	//CLBufferWrite(clMemIn1, first);
	//CLBufferWrite(clMemIn2, second);
	//CLBufferWrite(clMemOut, thirdGPU);   // 0.0 везде

	return;
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
	int method = 0;

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

		//--- начинаем работать с OCL
		int clCtx;             // хэндл контекста
		int clPrg;             // хэндл программы на девайсе
		int clKrn;             // хэндл кернела
		int clMemIn1;          // хэндл первого буфера (входного)
		int clMemIn2;          // хэндл второго буфера (входного)
		int clMemOut;          // хэндл третьего буфера (выходного)


		//initAllDataGPU(clCtx, clPrg, clKrn, clMemIn1, clMemIn2, clMemOut);

		//executeGPU(clKrn);

		////--- создаем буфер для чтения и считываем результат; он нам пригодится позднее
		//float buf[];
		//readOutBuf(clMemOut, buf);




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

	delete[] matrix;
	delete[] matrix1;
	delete[] matrix2;

	fout.close();

	
	
	return 0;
}
