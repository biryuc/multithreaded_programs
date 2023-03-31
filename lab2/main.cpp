// lab2_omp.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>

using namespace std;

#include <iostream>


void swap_elem(int& a, int& b) {
    int tmp = a;
    a = b;
    b = tmp;
}


bool read_num(FILE* file, int& number)
{
    char c = 0;
    std::string buf;
    while (true)
    {
        fread(&c, 1, 1, file);
        if (c == ' ' || feof(file) || c =='\n')
        {
            if (!buf.empty())
            {
                number = atoi(buf.c_str());
                return true;
            }
        }
        else
        {
            buf += c;
        }
    }

    return false;
}

int* get_min_max(unsigned char* channel, int size) {
    int len = size / 3;
    int minCh = int(channel[0]);
    int maxCh = int(channel[0]);
    int mm[2];

    for (int i = 0; i < len; i++) {
        if (int(channel[i]) < minCh) {
            minCh = int(channel[i]);
        }
        if (int(channel[i]) > maxCh) {
            maxCh = int(channel[i]);
        }
    }
    mm[0] = minCh;
    mm[1] = maxCh;
    return mm;
}

int* ignore(unsigned char* channel,int size,int coef) {
    int mmIgn[2];
    int hist[256] = {0};
    int ign = size * coef;
    int len = size / 3;
    int* mmCh = get_min_max(channel, size);

    int max = mmCh[1];
    int min = mmCh[0];
    
    for (int i = 0; i < len; i++) {
        hist[int(channel[i])]++;
    }


    while (ign != 0) {
        if (hist[min] < hist[max]) {
            ign -= hist[min];
            if (ign < 0) {
                ign = 0;
            }
            else {
                min++;
            }
            
        }
        else {
            ign -= hist[max];
            if (ign < 0) {
                ign = 0;
            }
            else {
                max--;
            }
        }
    }

    mmIgn[0] = min;
    mmIgn[1] = max;

    return mmIgn;
}

int* get_min_max_with_ignore(unsigned char* channel, int size,int coef) {
    int len = size / 3;
    int minCh = int(channel[0]);
    int maxCh = int(channel[0]);
    int mm[2];
    int* mmIgn = ignore(channel, size, coef);
    int maxIgn = mmIgn[1];
    int minIgn = mmIgn[0];

    for (int i = 0; i < len; i++) {
        if (int(channel[i]) > minIgn && int(channel[i]) < maxIgn) {
            if (int(channel[i]) < minCh) {
                minCh = int(channel[i]);
            }
            if (int(channel[i]) > maxCh) {
                maxCh = int(channel[i]);
            }
        }
       
    }
    mm[0] = minCh;
    mm[1] = maxCh;
    return mm;
}



unsigned char* change_contrast(unsigned char* channel, int size,int coef) {
    int len = size / 3;
    int tmpCh = 0;
   // int* mm = get_min_max_with_ignore(channel, size, coef);
    int* mm = get_min_max(channel, size);

    for (int i = 0; i < len; i++) {
        tmpCh = ((int(channel[i]) - mm[0]) * 255) / (mm[1] - mm[0]);
        channel[i] = unsigned char(tmpCh);
        if (tmpCh > 255) {
            channel[i] = unsigned char(255);
        }
        else if (tmpCh < 0) {
            channel[i] = unsigned char(0);
        }
    }

    return channel;
}

   
int main(int argc, char* argv[])
{

    //string r_filename = argv[1];

    //string w_filename = argv[2];

    //int coef = atoi(argv[3]);

    //int thrnum = atoi(argv[4]);

    string file_name = "C:/Users/Никита/source/repos/lab2_omp/arr.txt";
    ifstream fin;

    int n = 0;
    string buf;
    int size;
    unsigned char tmp;
    int w;
    int h;
    int x = 0;
    int min = 0;
    int max = 0;
    int counter = 0;
    int coef = 0;

    fin.open(file_name);
    FILE* file = fopen("C:/Users/Никита/source/repos/lab2_omp/in.pnm", "r");

    if (!file) {
        printf("Cannot open file");
        return 1;
    }

    for (int i = 0; i < 4; i++) {
        int num;
        if (read_num(file, num)) {
            if (i == 1) {
                w = num;
            }
            else if(i == 2) {
                h = num;
            }
        }        
    }

    size = h * w * 3;
 
    //int* arr = new(nothrow) int[size];
    unsigned char* arr = new(nothrow) unsigned char[size];
    if (arr == nullptr) {
        // printf("Memory cannot be allocated");
        fprintf(stderr, "Memory cannot be allocated");
        return 1;

    }

    unsigned char* R = new(nothrow) unsigned char[size/3];;
    unsigned char* G = new(nothrow) unsigned char[size/3];;
    unsigned char* B = new(nothrow) unsigned char[size/3];;
    int idx = 0;
    int r_count = 0;
    int g_count = 0;
    int b_count = 0;
    //Записали пиксели
    while (!feof(file)){
        fread(&tmp, 1, 1, file);
      //  int x = int(tmp);
        if (idx == 0) {
            R[r_count] = tmp;
            r_count++;
            idx++;
        }
        else if (idx == 1) {
            G[g_count] = tmp;
            g_count++;
            idx++;
        }
        else {
            B[b_count] = tmp;
            b_count++;
            idx = 0;
        }

        arr[counter] = tmp;
        counter++;
    }

    size = counter - 1;

    fclose(file);

    unsigned char* newR =  change_contrast(R, size, coef);
    unsigned char* newG =  change_contrast(G, size, coef);
    unsigned char* newB =  change_contrast(B, size, coef);

    idx = 0;
    r_count = 0;
    g_count = 0;
    b_count = 0;
   

    for (int j = 0; j < size; j++) {
        if (idx == 0) {
            arr[j] = R[r_count];
            r_count++;
            idx++;
        }
        else if (idx == 1) {
            arr[j] = G[g_count];
            g_count++;
            idx++;
        }
        else {
            arr[j] = B[b_count];
            b_count++;
            idx = 0;
        }

    }


   

    ofstream out("C:/Users/Никита/source/repos/lab2_omp/out.pnm", ios::binary);
    int val = 255;
    out << "P6" << '\n' << h << " " << w << '\n' << val << '\n';

    for (int i = 0; i < size; i++) {
        char ch = arr[i];
        out << ch;

    }
    

    out.close();
  
}


