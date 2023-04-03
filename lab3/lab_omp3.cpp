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

void swap_elem(int& a, int& b) {
    int tmp = a;
    a = b;
    b = tmp;
}


void report_num_threads(int level)
{
#pragma omp single
    {
        printf("Level %d: number of threads in the team - %d\n",
            level, omp_get_num_threads());
    }
}
void test() {
    omp_set_dynamic(0);
    #pragma omp parallel num_threads(2)
    {
        report_num_threads(1);
        #pragma omp parallel num_threads(2)
        {
            report_num_threads(2);
            #pragma omp parallel num_threads(2)
            {
                report_num_threads(3);
            }
        }
    }

}


int* partition(int* arr, int start_ind, int end_ind) {
    int pivot = (arr[start_ind] + arr[end_ind]) / 2;
    int st = start_ind;
    int end = end_ind;
    int idx[2];

    while (1) {
        while (arr[st] < pivot) {
            st++;
        }
        while (arr[end] > pivot) {
            end--;
        }
        if (st >= end) {
            idx[0] = end;
            idx[1] = st;
            return idx;
        }

        swap_elem(arr[st++], arr[end--]);
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

void sort_with_tasks(int* array, int start_ind, int end_ind) {

    if (start_ind < end_ind) {

        int end = end_ind;
        int st = start_ind;
        int pivot = array[(int)((st + end) / 2)];

        while (st <= end) {
            while (array[st] < pivot) {
                st++;
            }
            while (array[end] > pivot) {
                end--;
            }
            if (st <= end) {
                swap_elem(array[st], array[end]);
                st++;
                end--;
            }
        }

        //int idx = st;
        int idx = end;
       
        if ((end_ind - start_ind) < 500) {
            if (start_ind < end) {
                //sort_with_tasks(array, start_ind, idx - 1);
                sort_with_tasks(array, start_ind, idx );
            }
            if (st < end_ind) {
               // sort_with_tasks(array, idx, end_ind);
                sort_with_tasks(array, idx+1, end_ind);
            }
        }
        else {
            #pragma omp parallel 
            {
                #pragma omp single
                {

                    #pragma omp task
                    {
                        // sort_with_tasks(array, start_ind, idx - 1);
                        sort_with_tasks(array, start_ind, idx);
                    }
                  
                    #pragma omp task
                    {
                        //sort_with_tasks(array, idx, end_ind);
                        sort_with_tasks(array, idx + 1, end_ind);
                    }
                    
                }
            }
        }

    }

}



void sort_with_sections(int* array, int startIdx, int endIdx) {

    if (startIdx < endIdx) {

        int right = endIdx;
        int left = startIdx;
        int pivot = array[(int)((left + right) / 2)];

        while (left <= right) {
            while (array[left] < pivot) {
                left++;
            }
            while (array[right] > pivot) {
                right--;
            }
            if (left <= right) {
                int tmp = array[left];
                array[left] = array[right];
                array[right] = tmp;
                left++;
                right--;
            }
        }

        int divideIdx = left;

        if ((endIdx - startIdx) < 500) {
            if (startIdx < right) {
                sort_with_sections(array, startIdx, divideIdx - 1);
            }
            if (left < endIdx) {
                sort_with_sections(array, divideIdx, endIdx);
            }
        }
        else {
            
            #pragma omp parallel 
            {
                
                    #pragma omp sections 
                    {
                  // report_num_threads(1);
                        #pragma omp section
                        {
                          //  report_num_threads(1);
                            
                               // report_num_threads(2);
                                sort_with_sections(array, startIdx, divideIdx - 1);
                            
                        }
                        #pragma omp section
                        {
                            //report_num_threads(1);
                            
                                //report_num_threads(2);
                                sort_with_sections(array, divideIdx, endIdx);
                            
                        }
                        
                    }
                }
            
        }

    }

}





int main(int argc, char* argv[])
{
    //omp_set_dynamic(0);
   // omp_set_nested(1);
    //test();
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
    if (num <= 0 || num > omp_get_max_threads()) {
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
        sort_with_sections(arr, 0, n - 1);
        auto end_time = chrono::steady_clock::now();

        auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        cout << num << " " << "threads" << " " << "time : " << time_ms.count() << " ms\n";
    }
    else if (method == 2) {
        auto start_time = chrono::steady_clock::now();
        sort_with_tasks(arr, 0, n - 1);
        auto end_time = chrono::steady_clock::now();

        auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        cout << num << " " << "threads" << " " << "time : " << time_ms.count() << " ms\n";
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
