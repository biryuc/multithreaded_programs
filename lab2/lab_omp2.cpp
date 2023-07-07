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
#include <memory.h>

using namespace std;

#include <iostream>


int* get_min_max(unsigned char* channel, int size) {
   
    int minCh = 255;
    int maxCh = 0;
    int mm[2];

    for (int i = 0; i < size; i++) {
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





int* get_min_max_ignore(unsigned char* channel, int size, float coef) {
    int mmIgn[2];
    int hist[256] = { 0 };
   
    int Ign = size * coef;
    
    int* mmCh = get_min_max(channel, size);

    int max = mmCh[1];
    int min = mmCh[0];

   
    for (int i = 0; i < size; i++) {
        hist[channel[i]]++;
    }
   
    while (Ign != 0) {
        if (hist[min] < hist[max]) {
            
            Ign -= hist[min];

            if (Ign < 0) {
                Ign = 0;
            }
            else {
                min++;
            }
        }
        else {
            
            Ign -= hist[max];

            if (Ign < 0) {
                Ign = 0;
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

int* get_min_max_ignore_omp(unsigned char* channel, int size, float coef) {
    int mmIgn[2];
   
    int hist[256] = { 0 };

    int Ign = size * coef;

    int* mmCh = get_min_max(channel, size);

    int max = mmCh[1];
    int min = mmCh[0];

   #pragma omp parallel
    {
      int hist_tmp[256] = { 0 };
      #pragma omp parallel for schedule(guided, 1)
        for (int i = 0; i < size; i++) {
         
               hist_tmp[channel[i]]++;
          
        }
       
#pragma omp critical
        {
            for (int i = 0; i < 256; i++) {

               
                        hist[i] =  hist_tmp[i];
                

            }
        }
         
       
        
       
    }

    while (Ign != 0) {
        if (hist[min] < hist[max]) {

            Ign -= hist[min];

            if (Ign < 0) {
                Ign = 0;
            }
            else {
                min++;
            }
        }
        else {

            Ign -= hist[max];

            if (Ign < 0) {
                Ign = 0;
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




unsigned char* change_contrast(unsigned char* data, int size, int max, int min) {

    int tmpCh = 0;
       
        for (int i = 0; i < size; i++) {
            tmpCh = ((int(data[i]) - min) * 255) / (max - min);
            // data[i] = unsigned char(tmpCh);
            data[i] = tmpCh;
            if (tmpCh > 255) {
                //  data[i] = unsigned char(255);
                data[i] = 255;
            }
            else if (tmpCh < 0) {
                //data[i] = unsigned char(0);
                data[i] = 0;
            }
        }
    

    return data;
}


unsigned char* change_contrast_omp(unsigned char* data, int size, int max, int min) {

    

#pragma omp parallel
    {
        int tmpCh = 0;
        #pragma omp  parallel for schedule(guided, 1)
        for (int i = 0; i < size; i++) {
            tmpCh = ((int(data[i]) - min) * 255) / (max - min);
            // data[i] = unsigned char(tmpCh);

            if (tmpCh > 255) {
                //  data[i] = unsigned char(255);
                tmpCh = 255;
            }
            else if (tmpCh < 0) {
                //data[i] = unsigned char(0);
                tmpCh = 0;
            }

            //  #pragma omp critical
             //  {
            data[i] = tmpCh;
            //  }
        }

    }

    return data;
}

int main(int argc, char* argv[])
{
    

    if (argc < 5) {
        fprintf(stderr, "enter the arguments in the format: program.exe input_file.txt output_file.txt num_threads coef");
        return 1;
    }

    string r_filename = argv[1];

    string w_filename = argv[2];

    int thrnum = atoi(argv[3]);

    float coef = atof(argv[4]);

    /*int nest = 3;
    omp_set_dynamic(0);
    omp_set_max_active_levels(nest);*/

    //string r_filename = "C:/Users/Никита/source/repos/lab2_omp/in2.pnm";
    //string w_filename = "C:/Users/Никита/source/repos/lab2_omp/out.pnm";
    //int thrnum = 8;
    //float coef = 0.0;


    if (coef < 0 || coef > 0.5) {
        fprintf(stderr, "enter coef from 0 to 0.5");
        return 1;
    }
   

    

    int size;
    unsigned char skip_n;
    int w;
    int h;
    
    string header;
    int val255;

    int idx = 0;
    int r_count = 0;
    int g_count = 0;
    int b_count = 0;
    unsigned char tmp;
    

    ifstream In(r_filename, ios::binary);
    if (!In.is_open()) {
        fprintf(stderr,"Cannot open file");
        return 1;
    }
    In >> header >> w >> h >> val255;
    skip_n = In.get();

    if (w == h == 1) {
        unsigned char ch_ = In.get();
        In.close();
        ofstream out_(w_filename, ios::binary);
        if (!out_.is_open()) {
            fprintf(stderr,"Cannot open file");
            return 1;
        }
        out_ << ch_;
        out_.close();
        return 0;
   }
    if (thrnum != -1) {

        if (thrnum < -1 || thrnum > omp_get_max_threads()) {
            thrnum = omp_get_max_threads();
            omp_set_num_threads(thrnum);
        }
        else {
            omp_set_num_threads(thrnum);
        }
    }
    
   

    if (header == "P5") {
        size = h * w;
        unsigned char* arrP5 = new(nothrow) unsigned char[size];
        if (arrP5 == nullptr) {
            fprintf(stderr, "Memory cannot be allocated");
            return 1;
        }
        unsigned char* image = new(nothrow) unsigned char[size];
        if (image == nullptr) {
            fprintf(stderr, "Memory cannot be allocated");
            return 1;
        }
        int* mmP5 = new(nothrow) int[2];
        if (arrP5 == nullptr) {
            fprintf(stderr, "Memory cannot be allocated");
            return 1;
        }

        for (int i = 0; i < size; i++) {
            tmp = In.get();
            arrP5[i] = tmp;
        }
        In.close();
        if (thrnum == -1) {
            mmP5 = get_min_max_ignore(arrP5, size, coef);
            image = change_contrast(arrP5, size, mmP5[1], mmP5[0]);

           
        }
        else {

            mmP5 = get_min_max_ignore_omp(arrP5, size, coef);
            image = change_contrast_omp(arrP5, size, mmP5[1], mmP5[0]);

        }
        
        ofstream out(w_filename, ios::binary);
        if (!out.is_open()) {
            cerr << "Cannot open file";
            return 1;
        }
        int val = 255;
        out << header << '\n' << w << " " << h << '\n' << val255 << '\n';

        for (int i = 0; i < size; i++) {
            unsigned char ch = image[i];
            out << ch;

        }
        out.close();


       
        delete[] arrP5;
        delete[] image;
        delete[] mmP5;
        
    }
    else if (header == "P6") {


        size = h * w * 3;
        int len = size / 3;

        unsigned char* arr = new(nothrow) unsigned char[size];
        if (arr == nullptr) {
            fprintf(stderr, "Memory cannot be allocated");
            return 1;
        }

        unsigned char* image = new(nothrow) unsigned char[size];
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
            tmp = In.get();
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
        In.close();



        int mmRGB[6];

        if (thrnum == -1) {

            auto start_time = chrono::steady_clock::now();

            int* mmR = get_min_max_ignore(R, size / 3, coef);
            int* mmG = get_min_max_ignore(G, size / 3, coef);
            int* mmB = get_min_max_ignore(B, size / 3, coef);

            mmRGB[0] = mmR[0];
            mmRGB[1] = mmR[1];
            mmRGB[2] = mmG[0];
            mmRGB[3] = mmG[1];
            mmRGB[4] = mmB[0];
            mmRGB[5] = mmB[1];


            int tmpmin = mmR[0];
            int tmpmax = mmR[1];

            for (int i = 0; i < 6; i++) {

                if (mmRGB[i] < tmpmin) {
                    tmpmin = mmRGB[i];
                }
                if (mmRGB[i] > tmpmax) {
                    tmpmax = mmRGB[i];
                }
            }

            image = change_contrast(arr, size, tmpmax, tmpmin);

            auto end_time = chrono::steady_clock::now();

            auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
            cout << thrnum << " " << "threads" << " " << "time : " << time_ms.count() << " ms\n";



        }
        else {

            auto start_time = chrono::steady_clock::now();

            int* mmR = get_min_max_ignore_omp(R, size / 3, coef);
            int* mmG = get_min_max_ignore_omp(G, size / 3, coef);
            int* mmB = get_min_max_ignore_omp(B, size / 3, coef);


            mmRGB[0] = mmR[0];
            mmRGB[1] = mmR[1];
            mmRGB[2] = mmG[0];
            mmRGB[3] = mmG[1];
            mmRGB[4] = mmB[0];
            mmRGB[5] = mmB[1];


            int tmpmin = mmR[0];
            int tmpmax = mmR[1];

            for (int i = 0; i < 6; i++) {

                if (mmRGB[i] < tmpmin) {
                    tmpmin = mmRGB[i];
                }
                if (mmRGB[i] > tmpmax) {
                    tmpmax = mmRGB[i];
                }
            }

            image = change_contrast_omp(arr, size, tmpmax, tmpmin);

            auto end_time = chrono::steady_clock::now();

            auto time_ms = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
            cout << thrnum << " " << "threads" << " " << "time : " << time_ms.count() << " ms\n";

           
        }

        ofstream out(w_filename, ios::binary);
        if (!out.is_open()) {
            cerr << "Cannot open file";
            return 1;
        }
        int val = 255;
        out << header << '\n' << w << " " << h << '\n' << val255 << '\n';

        for (int i = 0; i < size; i++) {
            unsigned char ch = image[i];
            out << ch;

        }


        out.close();
       
        delete[] arr;
       
        delete[] R;
        delete[] G;
        delete[] B;


    }
    else {
        fprintf(stderr, "Incorrect file header");
        return 1;
    }

   
    return 0;

}


