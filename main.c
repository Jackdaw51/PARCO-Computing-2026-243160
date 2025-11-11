#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mmio.h"
#include "mmio.c"
#include "helpers.c"

int main(int argc, char *argv[])
{
    srand(time(NULL));
    int ret_code, i;
    MM_typecode matcode;
    SpCoord matrix;
    double *vec,* result;
    FILE *f;
    

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

    result = (double *)malloc(matrix.n_cols * sizeof(double));
    vec = generate_random_vector(matrix.n_cols);
    if (!matrix.row_indices || !matrix.col_indices || !matrix.values || !vec || !result)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }

    multiply_CSR_SpCoord(result, &matrix, vec);

    for (i = 0; i < matrix.n_nonzero; i++)
    {
        fscanf(f, "%d %d %lg\n", &matrix.row_indices[i], &matrix.col_indices[i], &matrix.values[i]);
        matrix.row_indices[i]--; /* adjust from 1-based to 0-based */
        matrix.col_indices[i]--;
    }

    coo_quicksort(&matrix, 0, matrix.n_nonzero); 
    
    FILE *a = fopen("output0.txt", "w");
    mm_write_banner(a, matcode);
    mm_write_mtx_crd_size(a, matrix.n_rows, matrix.n_cols, matrix.n_nonzero);

    for (i = 0; i < matrix.n_nonzero; i++)
        fprintf(a, "%d %d %lg\n", matrix.row_indices[i], matrix.col_indices[i], matrix.values[i]);

    fclose(a);

    convert_to_CSR(&matrix);

    if (f != stdin)
        fclose(f);



    /************************/
    /* now write out matrix */
    /************************/

    FILE *b = fopen("output1.txt", "w");

    for (i = 0; i < matrix.n_rows; i++)
        fprintf(b, "%d\n", matrix.row_indices[i]);

    fclose(b);



    return 0;
}
