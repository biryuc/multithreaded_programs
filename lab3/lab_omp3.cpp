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

int * partition(int* arr, int start_ind, int end_ind) {
    int pivot = (arr[start_ind] + arr[end_ind]) / 2;
    int st = start_ind;
    int end = end_ind;
    int idx[2];

    while(1){
        while (arr[st] < pivot) {
            st ++;
        }
        while (arr[end] > pivot) {
            end --;
        }
        if (st >= end) {
            idx[0] = end;
            idx[1] = st;
            return idx;
        }

        swap_elem(arr[st++], arr[end--]);
    }
   
      
}


void shell_sort(int* arr, int size) {
    for (int pivot = size / 2; pivot > 0; pivot /= 2) {
        for (int i = pivot; i < size; ++i) {
            for (int j = i - pivot; j >= 0 && arr[j] > arr[j + pivot]; j -= pivot) {
                swap_elem(arr[j], arr[j + pivot]);
            }
        }
    }
}

void quick_sort(int* arr, int start_ind, int end_ind) {
    int* idx;

    if (start_ind < end_ind) {
        idx = partition(arr, start_ind, end_ind);
        quick_sort(arr, start_ind, idx[0]);
        quick_sort(arr, idx[0] + 1, end_ind);
    }
       

}


void quick_sort_sections(int* arr, int start_ind, int end_ind) {
    
    int* idx;
    if ((end_ind - start_ind) < 500) {
        idx = partition(arr, start_ind, end_ind);
        if (start_ind < idx[0]) {
            quick_sort_sections(arr, start_ind, idx[0]);
        }
        else if (end_ind > idx[1]) {
            quick_sort_sections(arr, idx[0] + 1, end_ind);
        }
    }
    else {
        idx = partition(arr, start_ind, end_ind);
        #pragma omp parallel 
        {
            
            #pragma omp sections 
            {
                
                #pragma omp section
                {
                    quick_sort_sections(arr, start_ind, idx[0]);
                }
                #pragma omp section
                {
                    quick_sort_sections(arr, idx[0] + 1, end_ind);
                }
            }
        }
    }
       
    

}



void quick_sort_tasks(int* arr, int start_ind, int end_ind) {
    
    int *idx;
    if ((end_ind - start_ind) < 500) {
        idx = partition(arr, start_ind, end_ind);
        if (start_ind < idx[0]) {
            quick_sort_sections(arr, start_ind, idx[0]);
        }
        else if (end_ind > idx[1]) {
            quick_sort_sections(arr, idx[0] + 1, end_ind);
        }
    }
    else {
        idx = partition(arr, start_ind, end_ind);
        #pragma omp parallel 
        {
           
            #pragma omp single
            {
               
                #pragma omp task
                {
                    quick_sort_tasks(arr, start_ind, idx[0]);
                }
                #pragma omp task
                {
                    quick_sort_tasks(arr, idx[0] + 1, end_ind);
                }
                
            }
            #pragma omp taskwait
        }
    }

    
}




int main(int argc, char* argv[])
{
    if (argc < 5) {
       // printf("Enter the arguments in the format: program.exe input_file.txt output_file.txt num_threads");
        fprintf(stderr, "Enter the arguments in the format: program.exe input_file.txt output_file.txt num_threads method");
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
    int thr;


    fin.open(r_filename);
    //fin.open(file_name);
    if (!(fin.is_open())) {
       // printf("Cannot open file");
        fprintf(stderr, "Cannot open file");
        return 1;
    }

    fin >> n;
    if (fin.fail()) {
       // printf("Wrong file format");
        fprintf(stderr, "Wrong file format");
        return 1;
    }

    size = n;

    int* arr = new(nothrow) int[size];
    if (arr == nullptr) {
       // printf("Memory cannot be allocated");
        fprintf(stderr, "Memory cannot be allocated");
        return 1;

    }

    for (int i = 0; i < size; i++) {
            fin >> tmp;
            counter++;
            if (fin.fail() || (fin.eof() && (counter < (n)))) {
               // printf("Wrong file format");
                fprintf(stderr, "Wrong file format");
                delete[] arr;
                return 1;
            }
            else {
                arr[i] = tmp;
            }

        
    }
    fin.close();
    counter = 0;
    if (num <= 0 || num > omp_get_max_threads() ) {
        num = omp_get_max_threads();
        omp_set_num_threads(num);
    }
    else {
        omp_set_num_threads(num);
    }
   
   
    if (method == 0) {
        auto start_time = chrono::steady_clock::now();
        quick_sort(arr, 0, n - 1);
        auto end_time = chrono::steady_clock::now();

        auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        cout << "1 thread time : " << time_ms.count() << " ms\n";

    }
    else if (method == 1) {
        auto start_time = chrono::steady_clock::now();
        quick_sort_sections(arr, 0, n - 1);
        auto end_time = chrono::steady_clock::now();

        auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        cout << num << " "<<"threads"<< " " << "time : " << time_ms.count() << " ms\n";
    }
    else if (method == 2) {
        auto start_time = chrono::steady_clock::now();
        quick_sort_tasks(arr, 0, n - 1);
        auto end_time = chrono::steady_clock::now();

        auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        cout <<  num << " " << "threads" << " " << "time : " << time_ms.count() << " ms\n";
    }
    else {
        fprintf(stderr, "Method should be from 0 to 2");
        delete[] arr;
        return 1;
    }
    
    ofstream fout;
    fout.open(w_filename);
    //fout.open("out.txt");
    if (!(fout.is_open())) {
      //  printf("Cannot open file");
        fprintf(stderr, "Cannot open file");
        return 1;
    }
    

    
    for (int i = 0; i < size; i++) {
        fout << arr[i] << " ";
        
    }
    fout << endl;
    fout.close();

    delete[] arr;
    return 0;
}

