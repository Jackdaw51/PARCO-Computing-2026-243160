#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include "mmio.h"
#include "mmio.c"
#include "helpers.c"
#include "permutation.c"

#define NUMBEROFITERATIONS 20

#ifndef THREAD_NUMBER
#define THREAD_NUMBER 4
#endif

int main(int argc, char *argv[])
{
    srand(time(NULL));
    int ret_code, i;
    MM_typecode matcode;
    SpCoord matrix;
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

    coo_quicksort(&matrix, 0, matrix.n_nonzero);
    convert_to_CSR(&matrix);

    double start_time, end_time, elapsed_ms;
    char filename[256];
    sprintf(filename, "output/1_%d.bgn", matrix.n_rows);

#if defined(FOR) && THREAD_NUMBER != 1
    sprintf(filename, "output/%d_%d_%s.bgn", omp_get_max_threads(), matrix.n_rows, "FOR");
#endif
#if defined(STATIC) && CHUNKSIZE
    sprintf(filename, "output/%d_%d_%s_%d.bgn", omp_get_max_threads(), matrix.n_rows, "STATIC", CHUNKSIZE);
#endif
#if defined(DYNAMIC) && CHUNKSIZE
    sprintf(filename, "output/%d_%d_%s_%d.bgn", omp_get_max_threads(), matrix.n_rows, "DYNAMIC", CHUNKSIZE);
#endif
#if defined(GUIDED) && CHUNKSIZE
    sprintf(filename, "output/%d_%d_%s_%d.bgn", omp_get_max_threads(), matrix.n_rows, "GUIDED", CHUNKSIZE);
#endif
    FILE *a = fopen(filename, "w");
    if (a == NULL)
    {
        printf("Error opening file %s for writing\n", filename);
        exit(1);
    }

    for (unsigned i = 0; i < NUMBEROFITERATIONS; i++)
    {
        start_time = omp_get_wtime();
        multiply_CSR_SpCoord(result, &matrix, vec);
        end_time = omp_get_wtime();
        elapsed_ms = (end_time - start_time) * 1000.0;
        times[i] = elapsed_ms;
    }
    fprintf(a, "%f\n", compute_avg_time(times, NUMBEROFITERATIONS));
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
