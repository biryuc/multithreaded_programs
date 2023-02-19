// lab0.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <omp.h>

int main()
{
    
   // #pragma omp parallel
   // {
        //int n = omp_get_num_threads();
        //int k = omp_get_thread_num();
        //int st = (N * k) / n;
        //int end = N * (k + 1) / n;

        //for (int i = k; i < N; i+=n) {
        //    printf("%i\n", i);
        //}
        
        //printf("Hello %u/%u\n", omp_get_thread_num(), omp_get_num_threads());
        //#pragma omp  parallel  for shedule(static,1)  //for shedule(dynamic)
        //for (int i = st; i < end; i++) {
        //    printf("%i\n",i);
        //}
       
        //int N = 10000;
        //int m = 0;
        //#pragma omp  parallel  for
        //for (int i = 0; i < N; i++) {
           // #pragma omp atomic
            //#pragma omp critical
            //{
            //    m += i;
            //}
        //}

     //   printf("%i\n", m);

   // }
    int m = 0;
    int N = 10000;
     #pragma omp parallel
        {
            int mx = 0;
            #pragma omp for 
            for (int i = 0; i < N; i++) {
                mx += i;
            }
            #pragma omp atomic
            m += mx;
            
        }
        printf("%i\n", m);
//распараллелить списки, на каждую секцию свой тред.
        #pragma omp parallel
        {
            #pragma omp sections
            {
                #pragma omp section
                {

                }
                #pragma omp section
                {

                }
            }
        }

         #pragma omp parallel
        {
            #pragma omp single //чтобы в for каждый выполнился один раз
            //+ можно через for задавать динамически
            #pragma omp task
            //A
            #pragma omp task
            //B

        }

}

//ВНутри {} для каждого треда свои, вне переменные общие
//shedule???
//raise condition - к общим данным на запись (несколько тредов обращаются к одной общей переменной. - решение- атомарные операции
// #pragma omp atomic - атомарные операции. ( не все можно обозвать атомарными и только одно можно обозначить атомарным, дорого)
//  #pragma omp critical - гарантируется что будет выполнять один тред. ( можно на блок, еще более тяжелая)

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
