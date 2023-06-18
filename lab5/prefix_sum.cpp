#include <iostream>
#include <cmath>
using namespace std;



int main()
{
    float  a[] = { 1, 2, 3, 4, 5, 6, 7 ,8,0};
    int n = sizeof(a)/sizeof(a[0]) - 1 ;
    float tmp = a[n - 1];

    float t = 0;
    int size = log2(n );
    for (int i = 0; i <size; i++) {
        for (int j = 0; j < n - 1; j+=pow(2,i+1)) {
            a[(int)(j + pow(2, i+1) - 1)] =a[(int)(j + pow(2, i) - 1)] + a[(int)(j + pow(2, i + 1) - 1)];
        }
       
    }
    a[n - 1] = 0;
    for (int i = size-1; i >= 0; i--) {
        for (int j = 0; j < n - 1; j += pow(2, i + 1)) {
            t = a[(int)(j + pow(2, i) - 1)];
            a[(int)(j + pow(2, i) - 1)] = a[(int)(j + pow(2, i + 1) - 1)];
            a[(int)(j + pow(2, i + 1) - 1)] =t +  a[(int)(j + pow(2, i+1) - 1)];
        }

    }
    a[ n ] = a[n-1] + tmp;
    

    return 0;
}
