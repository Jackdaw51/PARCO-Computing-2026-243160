#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <sys/stat.h>
#include <valgrind/callgrind.h>
#include "mmio.h"
#include "mmio.c"
#include "helpers.h"
#include "permutation.c"

#define NUMBEROFITERATIONS 20

#ifndef THREAD_NUMBER
#define THREAD_NUMBER 4
#endif
#define CHUNKSIZE 1
#define STATIC

int main(int argc, char *argv[])
{
    srand(time(NULL));
    int ret_code, i;
    MM_typecode matcode;
    SpCOO matrix;
    double *vec, *result;
    FILE *f;
    double times[NUMBEROFITERATIONS];
    omp_set_num_threads(THREAD_NUMBER);

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s [martix-market-filename]\n", argv[0]);
        exit(1);
    }
    else
    {
        if ((f = fopen(argv[1], "r")) == NULL)
            exit(1);
    }

    int code = mm_read_banner(f, &matcode);
    if (code != 0)
    {
        printf("Could not process Matrix Market banner. Code: %d\n", code);
        exit(1);
    }

    /*  This is how one can screen matrix types if their application */
    /*  only supports a subset of the Matrix Market data types.      */

    if (mm_is_complex(matcode) && mm_is_matrix(matcode) &&
        mm_is_sparse(matcode))
    {
        printf("Sorry, this application does not support ");
        printf("Market Market type: [%s]\n", mm_typecode_to_str(matcode));
        exit(1);
    }

    /* find out size of sparse matrix .... */

    if ((ret_code = mm_read_mtx_crd_size(f, &matrix.n_rows, &matrix.n_cols, &matrix.n_nonzero)) != 0)
        exit(1);

    /* reseve memory for matrices */

    matrix.row_indices = (int *)malloc(matrix.n_nonzero * sizeof(int));
    matrix.col_indices = (int *)malloc(matrix.n_nonzero * sizeof(int));
    matrix.values = (double *)malloc(matrix.n_nonzero * sizeof(double));
    matrix.perm = (int *)malloc(matrix.n_rows * sizeof(int));
    matrix.inv_perm = (int *)malloc(matrix.n_rows * sizeof(int));

    result = (double *)calloc(matrix.n_cols, sizeof(double));
    vec = generate_random_vector(matrix.n_cols);
    if (!matrix.row_indices || !matrix.col_indices || !matrix.values || !vec || !result)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }

    for (i = 0; i < matrix.n_nonzero; i++)
    {
        fscanf(f, "%d %d %lg\n", &matrix.row_indices[i], &matrix.col_indices[i], &matrix.values[i]);
        matrix.row_indices[i]--; /* adjust from 1-based to 0-based */
        matrix.col_indices[i]--;
    }
    double start_perm_time, end_perm_time, start_time, end_time, start_sorting, end_sorting, elapsed_ms;

    coo_quicksort(&matrix, 0, matrix.n_nonzero);

    start_perm_time = omp_get_wtime();
    build_perm_by_nnz(&matrix); // 2.71
    row_perm(&matrix);          // 0.061
    end_perm_time = omp_get_wtime();
    start_sorting = omp_get_wtime();
    coo_quicksort(&matrix, 0, matrix.n_nonzero); // 5.51
    end_sorting = omp_get_wtime();
    convert_to_CSR(&matrix);

    char filename[256];
    mkdir("output_my_impl", 0777);
    sprintf(filename, "output_my_impl/%d_%d.bgn", omp_get_max_threads(), matrix.n_rows);

    FILE *a = fopen(filename, "w");
    if (a == NULL)
    {
        printf("Error opening file %s for writing\n", filename);
        exit(1);
    }
    CALLGRIND_START_INSTRUMENTATION;
    for (unsigned i = 0; i < NUMBEROFITERATIONS; i++)
    {
        start_time = omp_get_wtime();
        multiply_CSR_SpCoord(result, &matrix, vec);
        end_time = omp_get_wtime();
        elapsed_ms = (end_time - start_time) * 1000.0;
        times[i] = elapsed_ms;
    }
    CALLGRIND_STOP_INSTRUMENTATION;
    double start_inv_perm_time = omp_get_wtime();
    convert_to_COO(&matrix);
    row_inv_perm(&matrix);
    double end_inv_perm_time = omp_get_wtime();
    double perm_time = (end_perm_time - start_perm_time) * 1000;
    double inv_perm_time = (end_inv_perm_time - start_inv_perm_time) * 1000;
    double sorting_time = (end_sorting - start_sorting) * 1000;

    // fprintf(a, "%f\n%f\n%f\n%f\n", compute_avg_time(times, NUMBEROFITERATIONS), perm_time, sorting_time, inv_perm_time);
    printf("Wrote results to file %s\n", filename);
    fclose(a);

    // FILE *a = fopen("output0.txt", "w");
    // mm_write_banner(a, matcode);
    // mm_write_mtx_crd_size(a, matrix.n_rows, matrix.n_cols, matrix.n_nonzero);

    // for (i = 0; i < matrix.n_nonzero; i++)
    //     fprintf(a, "%d %d %lg\n", matrix.row_indices[i], matrix.col_indices[i], matrix.values[i]);

    // fclose(a);

    if (f != stdin)
        fclose(f);

    /************************/
    /* now write out matrix */
    /************************/

    // FILE *b = fopen("output1.txt", "w");

    // for (i = 0; i < matrix.n_rows; i++)
    //     fprintf(b, "%d\n", matrix.row_indices[i]);

    // fclose(b);

    return 0;
}
