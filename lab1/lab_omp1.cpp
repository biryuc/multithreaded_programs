#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>


using namespace std;

float gaus_det(int size, float* matrix) {
    float det = 1;
    float tmp;
    int i_new;
    float eps = 1.19e-07;

    for (int i = 0; i < size; ++i) {

        i_new = i;
        //находим самый большой элемент

        for (int j = i + 1; j < size; ++j) {
            if ((abs(matrix[j * size + i]) - abs(matrix[i_new * size + i])) > eps) {
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
        det *= matrix[i * size + i];

        ////делим на ведущий элемент строки

        for (int k = i + 1; k < size; ++k) {
            matrix[i * size + k] /= matrix[i * size + i];
        }

        // вычитаем из строк первую строку умноженную на ведущий элемент

        for (int k = i + 1; k < size; ++k) {
            for (int f = i + 1; f < size; ++f) {
                matrix[(k)*size + f] -= (matrix[i * size + f] * matrix[k * size + i]); // / matrix[i * size + i];
            }
        }

    }
    return det;
}

float gaus_det_omp(int size, float* matrix) {
    float det = 1;
    float tmp;
    int i_new;
    int chunk = 1;
    float eps = 1.19e-07;

    for (int i = 0; i < size; ++i) {
        
        i_new = i;
        //находим самый большой элемент
        #pragma omp parallel for  schedule(guided,chunk )
        for (int j = i + 1; j < size; ++j) {
            #pragma omp critical
            {
                if ((abs(matrix[j * size + i]) - abs(matrix[i_new*size + i]) ) > eps ) {
               
                        i_new = j;
               
                
                }

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
        #pragma omp parallel for  schedule(guided, chunk)
        for (int k = i + 1; k < size; ++k) {
            matrix[i * size + k] /= matrix[i * size + i];
        }

        // вычитаем из строк первую строку умноженную на ведущий элемент
        #pragma omp parallel for  schedule(guided, chunk)
        for (int k = i+1; k < size; ++k) {  
            
                for (int f = i + 1; f < size; ++f) {
                    matrix[(k)*size + f] -= (matrix[i * size + f] * matrix[k * size + i]);  // /matrix[i * size + i];
                    
                }  
        }

    }
    return det;
}



int main(int argc, char* argv[])
{
   if (argc < 4) {
       // printf("Enter the arguments in the format: program.exe input_file.txt output_file.txt num_threads");
        fprintf(stderr, "Enter the arguments in the format: program.exe input_file.txt output_file.txt num_threads");
        return 1;
    }
    string r_filename = argv[1];

    string w_filename = argv[2];

    int num = atoi(argv[3]);

    //float arr[16] = { 1.0f, 2.0f, 8.8f, 9.9f,3.0f, 4.0f, 3.3f, 4.4f,2.0f, 5.2f, 2.2f, 5.5f, 6.3f, 7.8f, 6.3f, 7.8f };

    ifstream fin;
    //string file_name = "test_in.txt";
   
    int n = 0;
    int cols = 0;
    int rows = 0;
    float tmp = 0;
    int counter = 0;
    float det;


    fin.open(r_filename);
    //fin.open(file_name);
    if (!(fin.is_open())) {
        fprintf(stderr, "Cannot open file");
      //  printf("Cannot open file");
        return 1;
    }

    fin >> n;
    if (fin.fail()) {
        fprintf(stderr, "Wrong file format");
       // printf("Wrong file format");
        return 1;
    }
    cols = n;
    rows = n;

    float* matrix = new float[n*n];

    if (matrix == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
       // printf("Memory cannot be allocated");
        return 1;

    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fin >> tmp;
            counter++;
            if (fin.fail()||(fin.eof() && (counter < (n*n)))) {
                delete[] matrix;
                fprintf(stderr, "Wrong file format");
               // printf("Wrong file format");
                return 1;
            }
            else {
                matrix[i * n + j] = tmp;
            }
            
        }
    }


    fin.close();



    
  
   

    int size = n;
    float tstart = omp_get_wtime();
    if (num == -1) {
         auto start_time = chrono::steady_clock::now();
         det = gaus_det(size, matrix);
         auto end_time = chrono::steady_clock::now();
         auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
         cout << "time(1 thread(s)) : " << time_ms.count() << " ms\n";

    }
    else if (num == 0 || num > omp_get_max_threads()) {
        omp_set_num_threads(omp_get_max_threads());
        auto start_time = chrono::steady_clock::now();
        det = gaus_det_omp(size, matrix);
        auto end_time = chrono::steady_clock::now();
        auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        cout << "time(omp) : " << time_ms.count() << " ms\n";
    }
    else if (num < -1 ) {
        fprintf(stderr, "The number of threads should be more then -1");
        //printf("The number of threads should be more then -1");
        delete[] matrix;
        return 1;
    }
    else {
        omp_set_num_threads(num);
        auto start_time = chrono::steady_clock::now();
        det = gaus_det_omp(size, matrix);
        auto end_time = chrono::steady_clock::now();
        auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        cout << "time(omp) : " << time_ms.count() << " ms\n";
    }
    /*float tend = omp_get_wtime();
    int threads = omp_get_num_threads();*/
   

    delete[] matrix;

    //printf("time (sec): %f/%u", tend - tstart);
   // printf("time(%i thread(s)) : %g ms\n", num, tend - tstart);
    ofstream fout;
    fout.open(w_filename);
    //fout.open("out.txt");
    if (!(fout.is_open())) {
        fprintf(stderr, "Cannot open file");
       // printf("Cannot open file");
        return 1;
    }
    fout << det << endl;

    fout.close();
    return 0;
}
