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




int* get_min_max(unsigned char* channel, int size) {
    int len = size / 3;
    int minCh = 255;
    int maxCh = 0;
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

int* ignore(unsigned char* channel, int size, float coef) {
    int mmIgn[2];
    int hist[256] = { 0 };
    int len = size / 3;
    int ignoreNum = len * coef;
    
    int* mmCh = get_min_max(channel, size);

    int max = mmCh[1];
    int min = mmCh[0];

    //histogram shows how many pixels have every level or brightness from 0 to 255
    int histogram[256] = { 0 };
    for (int i = 0; i < len; i++) {
        histogram[channel[i]]++;
    }
    //applying pixel ignoring
    while (ignoreNum != 0) {
        if (histogram[min] < histogram[max]) {
            //ignoring pixels from the left (the brightest ones)
            ignoreNum -= histogram[min];
            if (ignoreNum < 0) {
                ignoreNum = 0;
            }
            else {
                min++;
            }
        }
        else {
            //ignoring pixels from the right (the darkest ones)
            ignoreNum -= histogram[max];
            if (ignoreNum < 0) {
                ignoreNum = 0;
            }
            else {
                max--;
            }
        }
    }
    //min and max should be returned
    return new(nothrow) int[2] {min, max};
}

int* get_min_max_with_ignore(unsigned char* channel, int size, int coef) {
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



unsigned char* change_contrast(unsigned char* channel, int size, int max, int min) {

    int tmpCh = 0;

    for (int i = 0; i < size; i++) {
        tmpCh = ((int(channel[i]) - min) * 255) / (max - min);
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

    //float coef = atoi(argv[3]);

    //int thrnum = atoi(argv[4]);

    int size;
    unsigned char skip_n;
    int w;
    int h;
    float coef = 0.5;
    string header;
    int val255;

    int idx = 0;
    int r_count = 0;
    int g_count = 0;
    int b_count = 0;
    unsigned char tmp;
    

    ifstream fileIn("C:/Users/Никита/source/repos/lab2_omp/in.pnm", ios::binary);
    if (!fileIn.is_open()) {
        cerr << "Cannot open file";
        return 1;
    }
    fileIn >> header >> w >> h >> val255;
    skip_n = fileIn.get();


    size = h * w * 3;
    int len = size / 3;

    unsigned char* arr = new(nothrow) unsigned char[size];
    if (arr == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        return 1;
    }

    unsigned char* R = new(nothrow) unsigned char[size / 3];
    if (R == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        return 1;
    }
    unsigned char* G = new(nothrow) unsigned char[size / 3];
    if (G == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        return 1;
    }
    unsigned char* B = new(nothrow) unsigned char[size / 3];
    if (B == nullptr) {
        fprintf(stderr, "Memory cannot be allocated");
        return 1;
    }

 
    for (int i = 0; i < size; i++) {
        //R[i] = fileIn.get();
        //G[i] = fileIn.get();
        //B[i] = fileIn.get();
        //arr[i * 3] = R[i];
        //arr[i * 3 + 1] = G[i];
        //arr[i * 3 + 2] = B[i];

        tmp = fileIn.get();
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
        arr[i] = tmp; 
    }
    fileIn.close();




   
    int* mmR = ignore(R, size, coef);
    int min = mmR[0];
    int max = mmR[1];
    int* mmG = ignore(G, size, coef);

    if (mmG[0] < min) {
        min = mmG[0];
    }
    if (mmG[1] > max) {
        max = mmG[1];
    }
    int* mmB = ignore(B, size, coef);
    if (mmB[0] < min) {
        min = mmB[0];
    }
    if (mmB[1] > max) {
        max = mmB[1];
    }

    unsigned char* newarr = change_contrast(arr, size, max, min);






    ofstream out("C:/Users/Никита/source/repos/lab2_omp/out.pnm", ios::binary);
    if (!out.is_open()) {
        cerr << "Writing file error";
        exit(1);
    }
    int val = 255;
    out << header << '\n' << w << " " << h << '\n' << val255 << '\n';

    for (int i = 0; i < size; i++) {
        unsigned char ch = newarr[i];
        out << ch;

    }


    out.close();

}


