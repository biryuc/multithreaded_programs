#include <iostream>
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>


using namespace std;


//Однопоточная реализация
void smooth(const double* in, double* out, int N, int h) {
    double sum = 0;
    int counter = 0;

    if (N<=0) {
        fprintf(stderr, "The array size must be greater than zero");
        exit(-1);
        
    }

    if (h == 0) {

        for (int i = 0; i < N ; i++) {
            out[i] = in[i];
        }

    }
    else if (h<0) {
        for (int i = 0; i < N; i++) {
            out[i] = 0;
        }

    }
    else {
        for (int i = 0; i < N; i++) {
            for (int k = 0; k < N ; k++) {
                if (abs(k-i) <= h) {
                    sum += in[k];
                    counter++;
                }
            }

          
                out[i] = sum / counter;
                counter = 0;
                sum = 0;

        }
    }
}

// Далее идет реализация функции при помощи распараллеливания
// Такая реализация подходит только в случае достаточно больших массивов,
// так как инициализация потоков требует дополнительного времени
// Пример замеров при помощи разных методов (Размер массива 11000 элементов):
// Section time  : 438 ms  - Используя секции 
// Tasks time    : 516 ms  - Используя потоки в очереди задач
// 1 thread time : 783 ms  - Без параллелизма
void smooth_part(const double* in, double* out, int end_index, int h,int start_index,int N) {
    double sum = 0;
    int counter = 0;

    for (int i = start_index; i < end_index; i++) {
        for (int k = 0; k < N; k++) {
            if (abs(k - i) <= h) {
                sum += in[k];
                counter++;
            }
        }

       
            out[i] = sum / counter;
            counter = 0;
            sum = 0;
       


    }
    

}

//При помощи tasks
void smooth_omp_tasks(const double* in, double* out, int N, int h) {
    int end_index = N / 2;
    int num = omp_get_max_threads();
    omp_set_num_threads(num);

    if (N <= 0) {
        fprintf(stderr, "The array size must be greater than zero");
        exit(-1);

    }

    if (h == 0) {

        for (int i = 0; i < N; i++) {
            out[i] = in[i];
        }

    }
    else if (h < 0) {
        for (int i = 0; i < N; i++) {
            out[i] = 0;
        }

    }
    else {

            #pragma omp parallel 
                    {

            #pragma omp single
                        {

            #pragma omp task
                            {
                                smooth_part(in, out, end_index, h, 0,N);
                            }
            #pragma omp task
                            {
                                smooth_part(in, out, N, h, end_index,N);
                            }

                        }
            #pragma omp taskwait
                    }

    }



}

//При помощи sections
void smooth_omp_sections(const double* in, double* out, int N, int h) {
    int end_index = N / 2;
    int num = omp_get_max_threads();
    omp_set_num_threads(num);

    if (N <= 0) {
        fprintf(stderr, "The array size must be greater than zero");
        exit(-1);

    }

    if (h == 0) {

        for (int i = 0; i < N; i++) {
            out[i] = in[i];
        }

    }
    else {

            #pragma omp parallel 
                    {

            #pragma omp sections 
                        {

            #pragma omp section
                            {
                                smooth_part(in, out, end_index, h, 0,N);
                            }
            #pragma omp section
                            {
                                smooth_part(in, out, N, h, end_index,N);
                            }

                        }

                    }
    }

}



int main()
{

    int h = 7;
  
    ifstream fin;
    string r_filename = "11000.txt";
    int size;
    double tmp = 0;
    int counter = 0;


    fin.open(r_filename);
    //fin.open(file_name);
    if (!(fin.is_open())) {
        // printf("Cannot open file");
        fprintf(stderr, "Cannot open file");
        return 1;
    }

    fin >> size;
    if (fin.fail()) {
        // printf("Wrong file format");
        fprintf(stderr, "Wrong file format");
        fin.close();
        return 1;
    }

    int N = size;

    double* in = new(nothrow) double[N];
    if (in == nullptr) {
        // printf("Memory cannot be allocated");
        fprintf(stderr, "Memory cannot be allocated");
        fin.close();
        return 1;

    }
    double* out = new(nothrow)  double[N];
    if (out == nullptr) {
        // printf("Memory cannot be allocated");
        fprintf(stderr, "Memory cannot be allocated");
        fin.close();
        delete[] in;
        return 1;

    }

    for (int i = 0; i < size; i++) {
        fin >> tmp;
        counter++;
        if (fin.fail() || (fin.eof() && (counter < (size)))) {
            fprintf(stderr, "Wrong file format");
            delete[] in;
            fin.close();
            return 1;
        }
        else {
            in[i] = tmp;
        }


    }
    fin.close();



    auto start_time = chrono::steady_clock::now();
    smooth_omp_sections(in, out, N, h);
    auto end_time = chrono::steady_clock::now();


    auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    cout << "Section time : " << time_ms.count() << " ms\n";


     start_time = chrono::steady_clock::now();
    smooth_omp_tasks(in, out, N, h);
     end_time = chrono::steady_clock::now();

     time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    cout << "Tasks time : " << time_ms.count() << " ms\n";


     start_time = chrono::steady_clock::now();
    smooth(in, out, N, h);
     end_time = chrono::steady_clock::now();

     time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    cout << "1 thread time : " << time_ms.count() << " ms\n";

    delete[] in;
    delete[] out;


}

