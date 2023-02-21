#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>

short OMP_NUM_THREADS = 1;

using namespace std;

float** create_matrix(int size) {
    float** matrix = new float* [size];

    if (matrix == nullptr) {
        printf("Memory cannot be allocated");
        
    }
    for (int i = 0; i < size; i++) {
        matrix[i] = new float[size];
        if (matrix[i] == nullptr) {
            printf("Memory cannot be allocated");
           
        }
    }

    return matrix;
}

void free_memory(float** matrix,int size) {
    for (int i = 0; i < size; i++) {
        delete[] matrix[i];
    }
    delete[] matrix;
}


void matrix_deductions(float** matrix, int size, int row, int col, float** reduced_matrix) {

    int row_offset = 0;
    int col_offset = 0;

    #pragma omp parallel for schedule(static, OMP_NUM_THREADS)
    for (int i = 0; i < size - 1; i++) {
        if (i == row) {
            row_offset = 1;
        }
        
        col_offset = 0;
        for (int j = 0; j < size - 1; j++) {
            if (j == col) {
                col_offset = 1;
            }

            reduced_matrix[i][j] = matrix[i + row_offset][j + col_offset];
        }
    }
}



float determinant_(float** matrix, int size) {
    float det = 0;
    int sign = 1;

    if (size == 2) {
        return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];
    }
    else if (size == 1) {
        return matrix[0][0];
    }

    float** reduced_matrix = create_matrix(size - 1);
    if (reduced_matrix == nullptr) {
        return 1;
    }

    #pragma omp parallel for schedule(static, OMP_NUM_THREADS)
    for (int j = 0; j < size; j++) {
        matrix_deductions(matrix, size, 0, j, reduced_matrix);
        
        //#pragma omp atomic
        //{
            det = det + (sign * matrix[0][j] * determinant_(reduced_matrix, size - 1));
            
       // }
        sign *= -1;
    }

    free_memory(reduced_matrix, size - 1);

    return det;
}



int main(int argc, char* argv[])
{
   if (argc < 4) {
        printf("enter the arguments in the format: program.exe input_file.txt output_file.txt num_threads");
        return 1;
    }
    string r_filename = argv[1];

    string w_filename = argv[2];

    OMP_NUM_THREADS = atoi(argv[3]);

   

    ifstream fin;
    string file_name = "test_in.txt";
   
    int n = 0;
    int cols = 0;
    int rows = 0;
    float tmp = 0;


    fin.open(r_filename);
    if (!(fin.is_open())) {
        printf("cannot open file");
        return 1;
    }

    fin >> n;
    cols = n;
    rows = n;

    float** matrix = create_matrix(n);
    if (matrix == nullptr) {
        return 1;
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fin >> tmp;
            matrix[i][j] = tmp;
        }
    }


    fin.close();


    omp_set_num_threads(OMP_NUM_THREADS);
    
    int size = n;
    float tstart = omp_get_wtime();
    float det = determinant_(matrix, size);
    float tend = omp_get_wtime();

    free_memory(matrix, size);

    //printf("time (sec): %f/%u", tend - tstart);
    printf("time(%i thread(s)) : %g ms\n", omp_get_num_threads(), tend - tstart);
    ofstream fout;
    fout.open(w_filename);
    if (!(fout.is_open())) {
        printf("cannot open file");
        return 1;
    }
    fout << det << endl;

    fout.close();
    return 0;
}
