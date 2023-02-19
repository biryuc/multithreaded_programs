#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

float** create_matrix(int size) {
    float** matrix = new float* [size];
    for (int i = 0; i < size; i++) {
        matrix[i] = new float[size];
    }

    return matrix;
}

void free_memory(float** matrix,int size) {
    for (int i = 0; i < size; i++) {
        delete[] matrix[i];
    }
    delete[] matrix;
}


void getMatrixWithoutRowAndCol(float** matrix, int size, int row, int col, float** newMatrix) {
    int offsetRow = 0;
    int offsetCol = 0;
    for (int i = 0; i < size - 1; i++) {
        if (i == row) {
            offsetRow = 1;
        }

        offsetCol = 0;
        for (int j = 0; j < size - 1; j++) {
            if (j == col) {
                offsetCol = 1;
            }

            newMatrix[i][j] = matrix[i + offsetRow][j + offsetCol];
        }
    }
}

float matrixDet(float** matrix, int size) {
    float det = 0;
    int degree = 1;

    if (size == 1) {
        return matrix[0][0];
    }

    if (size == 2) {
        return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];
    }

   /* float** newMatrix = new float* [size - 1];
    for (int i = 0; i < size - 1; i++) {
        newMatrix[i] = new float[size - 1];
    }*/

    float** newMatrix = create_matrix(size - 1);

    for (int j = 0; j < size; j++) {
        getMatrixWithoutRowAndCol(matrix, size, 0, j, newMatrix);

        det = det + (degree * matrix[0][j] * matrixDet(newMatrix, size - 1));

        degree = -degree;
    }

    /*for (int i = 0; i < size - 1; i++) {
        delete[] newMatrix[i];
    }
    delete[] newMatrix;*/

    free_memory(newMatrix, size - 1);

    return det;
}
pair<float**, int> initialize(float** matrix, int size) {
    return make_pair(matrix, size);
}

pair<float**, int> read_file(string file_name) {
    ifstream fin;
    int n = 0;
    int cols = 0;
    int rows = 0;
    float tmp = 0;
   /* fin.open("test_in.txt");*/
    fin.open(file_name);
    fin >> n;
    cols = n;
    rows = n;

    float** matrix = create_matrix(n);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fin >> tmp;
            matrix[i][j] = tmp;
        }
    }

    pair<float**, int> p = initialize(matrix,n);

    fin.close();

    return p;
}




int main(int argc, char* argv[])
{

    
    //ifstream fin;
    //fin.open("test_in.txt");
    //
    //int n;
    //int cols;
    //int rows;
    //
    //float tmp;

    //fin >> n;
    //
    //cols = n;
    //rows = n;
   
    //matrix = new float* [n];
    //for (int i = 0; i < n; i++) {
    //    matrix[i] = new float[n];
    //}

    //float** matrix = create_matrix(n);

    //for (int i = 0; i < rows; i++) {
    //    for (int j = 0; j < cols; j++) {
    //        fin >> tmp;
    //        matrix[i][j] = tmp;
    //    }
    //      

    //}

    //for (int i = 0; i < rows; i++) {
    //    for (int j = 0; j < cols; j++) {

    //        printf("%f\n", matrix[i][j]);
    //    }
    //}
    string file_name = "test_in.txt";
    pair<float**, int> pair = read_file(file_name);
    float** matrix = pair.first;
    int size = pair.second;
    float det = matrixDet(matrix, size);
    free_memory(matrix, size);
    

//    float tstart = omp_get_wtime();
//    int sum = 0;
//#pragma omp parallel for
//    for (int i = 0; i < n; ++i)
//    {
//#pragma omp atomic
//        sum += i;
//    }
//    float tend = omp_get_wtime();


    //printf("Time (sec): %f", tend - tstart);
   
   /* for (int i = 0; i < n; i++) {
        delete[] matrix[i];
    }

    delete[] matrix;*/
    return 0;
}