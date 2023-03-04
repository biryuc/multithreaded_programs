// lab_omp3.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

using namespace std;

void swap_elem(int &a,int &b) {
    int tmp = a;
    a = b;
    b = tmp;
}


int partition(int* arr,int low,int high) {
    int pivot = arr[high];
    int idx = low;
    

    for (int i = low; i < high; i++) {
        if (arr[i] <= pivot) {
            swap_elem(arr[i], arr[idx]);
            idx++;
        }
    }
   
    swap_elem(arr[idx], arr[high]);
    return idx;

}

void quick_sort(int* arr,int start_ind,int end_ind) {
   
    int idx;

    if (start_ind < end_ind) {
        idx = partition(arr, start_ind, end_ind);
        quick_sort(arr, start_ind, idx -1);
        quick_sort(arr, idx + 1, end_ind);

    }
    else {
        return;
    }
  
}


int partition_sections(int* arr, int low, int high) {
    int pivot = arr[high];
    int idx = low;

    #pragma omp parallel for  schedule(static, 1)
    for (int i = low; i < high; i++) {
        if (arr[i] <= pivot) {
            swap_elem(arr[i], arr[idx]);
            idx++;
        }
    }

    swap_elem(arr[idx], arr[high]);
    return idx;

}

void quick_sort_sections(int* arr, int start_ind, int end_ind) {

    int idx;

    if (start_ind < end_ind) {
        idx = partition_sections(arr, start_ind, end_ind);
        #pragma omp parallel
        {
            #pragma omp sections 
            {
                #pragma omp section
                {
                    quick_sort_sections(arr, start_ind, idx - 1);
                }
                #pragma omp section
                {
                    quick_sort_sections(arr, idx + 1, end_ind);
                }
            }
        }

    }
    else {
        return;
    }

}


int partition_tasks(int* arr, int low, int high) {
    int pivot = arr[high];
    int idx = low;

    #pragma omp parallel for  schedule(static, 1)
    for (int i = low; i < high; i++) {
        if (arr[i] <= pivot) {
            swap_elem(arr[i], arr[idx]);
            idx++;
        }
    }

    swap_elem(arr[idx], arr[high]);
    return idx;

}

void quick_sort_tasks(int* arr, int start_ind, int end_ind) {

    int idx;

    if (start_ind < end_ind) {
        idx = partition_tasks(arr, start_ind, end_ind);
        #pragma omp parallel 
        {
            #pragma omp single
            {
                #pragma omp task
                {
                    quick_sort_tasks(arr, start_ind, idx - 1);
                }//omp task
                #pragma omp task
                {
                    quick_sort_tasks(arr, idx + 1, end_ind);
                }//omp task

            }//omp single

            #pragma omp taskwait

        }//omp parallel

    }
    else {
        return;
    }

}




int main(int argc, char* argv[])
{
    if (argc < 5) {
        printf("Enter the arguments in the format: program.exe input_file.txt output_file.txt num_threads");
        return 1;
    }
    string r_filename = argv[1];

    string w_filename = argv[2];

    int num = atoi(argv[3]);

    int method = atoi(argv[4]);

   

    ifstream fin;
    string file_name = "arr.txt";

    int n = 0;
    int size;
    int tmp;
    int counter = 0;



    fin.open(r_filename);
    //fin.open(file_name);
    if (!(fin.is_open())) {
        printf("Cannot open file");
        return 1;
    }

    fin >> n;
    if (fin.fail()) {
        printf("Wrong file format");
        return 1;
    }

    size = n;

    int* arr = new int[size];
    if (arr == nullptr) {
        printf("Memory cannot be allocated");
        return 1;

    }

    for (int i = 0; i < size; i++) {
            fin >> tmp;
            counter++;
            if (fin.fail() || (fin.eof() && (counter < (n)))) {
                printf("Wrong file format");
                return 1;
            }
            else {
                arr[i] = tmp;
            }

        
    }
    fin.close();
    counter = 0;

    omp_set_num_threads(num);
    if (method == 0) {
        auto start_time = chrono::steady_clock::now();
        quick_sort(arr, 0, n - 1);
        auto end_time = chrono::steady_clock::now();

        auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        cout << "time(1 thread(s)) : " << time_ms.count() << " ms\n";

    }
    else if (method == 1) {
        auto start_time = chrono::steady_clock::now();
        quick_sort_sections(arr, 0, n - 1);
        auto end_time = chrono::steady_clock::now();

        auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        cout << "time(1 thread(s)) : " << time_ms.count() << " ms\n";
    }
    else if (method == 2) {
        auto start_time = chrono::steady_clock::now();
        quick_sort_tasks(arr, 0, n - 1);
        auto end_time = chrono::steady_clock::now();

        auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        cout << "time(1 thread(s)) : " << time_ms.count() << " ms\n";
    }
    
    ofstream fout;
    fout.open(w_filename);
    //fout.open("out.txt");
    if (!(fout.is_open())) {
        printf("Cannot open file");
        return 1;
    }
    

    
    for (int i = 0; i < size; i++) {
        fout << arr[i] << " ";
    }
    fout.close();

    delete[] arr;
    return 0;
}


