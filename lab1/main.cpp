#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>



using namespace std;

float* create_matrix(int size) {
    float* matrix = new float [size];

    if (matrix == nullptr) {
        printf("Memory cannot be allocated");
        
    }
    /*for (int i = 0; i < size; i++) {
        matrix[i] = new float[size];
        if (matrix[i] == nullptr) {
            printf("Memory cannot be allocated");
           
        }
    }*/

    return matrix;
}

void free_memory(float* matrix,int size) {
 /*   for (int i = 0; i < size; i++) {
        delete[] matrix[i];
    }*/
    delete[] matrix;
}


void matrix_deductions(float** matrix, int size, int row, int col, float** reduced_matrix) {

    int row_offset = 0;
    int col_offset = 0;

   #pragma omp parallel for schedule(static, 1)
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

float gaus_det(int size, float* matrix) {
    float det = 1;
    float tmp;
    int i_new;

    for (int i = 0; i < size; ++i) {

        i_new = i;
        //находим самый большой элемент
        for (int j = i + 1; j < size; ++j) {
            if (abs(matrix[i_new*size + i]) < abs(matrix[j*size + i])) {
                i_new = j;
            }
        }

        //поднимаем эту строку 
        for (int k = 0; k < size; ++k) {
            tmp = matrix[i * size + k];
            matrix[i * size + k] = matrix[i_new * size + k];
            matrix[i_new * size + k] = tmp;
        }
        //при замене двух строк определитель меняет знак
        if (i != i_new) {
            det *= -1;
        }

        //опеределитель равен произведению диагональных элементов в верхне треугольной форме
        det *= matrix[i*size + i];

        //делим на ведущий элемент строки
        for (int k = i + 1; k < size; ++k) {
            matrix[i * size + k] /= matrix[i * size + i];
        }

        // вычитаем из строк первую строку умноженную на ведущий элемент
        for (int k = i+1; k < size; ++k) {
                for (int f = i + 1; f < size; ++f) {
                    matrix[(k)*size + f] -= matrix[i * size + f] * matrix[k * size + i];  
            }
        }

    }
    return det;
}

float DET(int n,float *a) {
    const float EPS = 1E-100;
    float det = 1;
    float tmp = 0;
    for (int i = 0; i < n; ++i) {
        int k = i;
        #pragma omp parallel for  schedule(static, 1)
        for (int j = i + 1; j < n; ++j) {
            if (abs(a[j * n + i]) > abs(a[k * n + i]))
                k = j;
        }
        if (abs(a[k * n + i]) < EPS) {
            det = 0;
            break;
        }
        
        for (int f = 0; f < n; ++f) {
            tmp = a[i * n + f];
            a[i * n + f] = a[k*n + f];
            a[k * n + f] = tmp;
        }
        if (i != k) {
            det = -det;
        }
        det *= a[i * n + i];
        #pragma omp parallel for  schedule(static, 1)
        for (int j = i + 1; j < n; ++j) {
            a[i * n + j] /= a[i * n + i];
        }
        #pragma omp parallel for  schedule(static, 1)
        for (int j = 0; j < n; ++j) {
            if (j != i && abs(a[j * n + i]) > EPS) {
                for (int k = i + 1; k < n; ++k) {
                    a[j * n + k] -= a[i * n + k] * a[j * n + i];
                }
            }
        }
    }
    return det;
}



//float determinant_(float** matrix, int size) {
//    float det = 0;
//    int sign = 1;
//
//    if (size == 2) {
//        return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];
//    }
//    else if (size == 1) {
//        return matrix[0][0];
//    }
//
//    float** reduced_matrix = create_matrix(size - 1);
//    if (reduced_matrix == nullptr) {
//        return 1;
//    }
//
//    #pragma omp parallel for  schedule(static, 1)
//    for (int j = 0; j < size; j++) {
//        matrix_deductions(matrix, size, 0, j, reduced_matrix);
//        
//        //#pragma omp atomic
//        //
//            printf("%i thread(s)\n", omp_get_thread_num());
//            det = det + (sign * matrix[0][j] * determinant_(reduced_matrix, size - 1));
//            
//       // }
//        sign *= -1;
//    }
//
//    free_memory(reduced_matrix, size - 1);
//
//    return det;
//}



int main(int argc, char* argv[])
{
   //if (argc < 4) {
   //     printf("enter the arguments in the format: program.exe input_file.txt output_file.txt num_threads");
   //     return 1;
   // }
   // string r_filename = argv[1];

   // string w_filename = argv[2];

   // int num = atoi(argv[3]);

    float arr[16] = { 1.0f, 2.0f, 8.8f, 9.9f,3.0f, 4.0f, 3.3f, 4.4f,2.0f, 5.2f, 2.2f, 5.5f, 6.3f, 7.8f, 6.3f, 7.8f };

    ifstream fin;
    string file_name = "test_in.txt";
   
    int n = 0;
    int cols = 0;
    int rows = 0;
    float tmp = 0;


    //fin.open(r_filename);
    fin.open(file_name);
    if (!(fin.is_open())) {
        printf("cannot open file");
        return 1;
    }

    fin >> n;
    cols = n;
    rows = n;

    float* matrix = new float[n*n];

    if (matrix == nullptr) {
        printf("Memory cannot be allocated");
        return 1;

    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fin >> tmp;
            matrix[i*n+j] = tmp;
        }
    }


    fin.close();


   omp_set_num_threads(8);
   

    int size = n;
    float tstart = omp_get_wtime();
    float det = DET2( size, matrix);
    float tend = omp_get_wtime();

    delete[] matrix;

    //printf("time (sec): %f/%u", tend - tstart);
    printf("time(%i thread(s)) : %g ms\n", 8, tend - tstart);
    ofstream fout;
    //fout.open(w_filename);
    fout.open("out.txt");
    if (!(fout.is_open())) {
        printf("cannot open file");
        return 1;
    }
    fout << det << endl;

    fout.close();
    return 0;
}
