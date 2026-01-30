// #include <stdio.h>
// #include <omp.h>
// int main()
// {
//     int x = 0;
// #pragma omp parallel for lastprivate(x)
//     for (int i = 0; i < 20; i++)
//     {
//         x = i * 10;
//         printf("Thread %d: x = %d\n",omp_get_thread_num(),x);
//     }
//     printf("After parallel for: x = %d\n",x);
    
// }