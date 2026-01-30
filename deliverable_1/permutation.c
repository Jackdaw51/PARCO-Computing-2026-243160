#include <stdlib.h>
#define NNZ 4
#include "helpers.h"
typedef struct
{
    int row;
    int nnz;
} RowNNZ;
void print_matrix(SpCOO *matrix);
void print_csr_matrix(SpCOO *matrix);

int cmp_desc(const void *a, const void *b)
{
    const RowNNZ *x = (const RowNNZ *)a;
    const RowNNZ *y = (const RowNNZ *)b;
    return (y->nnz - x->nnz);
}

void build_perm_by_nnz(SpCOO *matrix)
{
    int *row_ptr = (int *)calloc(matrix->n_rows + 1, sizeof(int));
    if (!row_ptr)
        return;

    // Count non-zeros per row
    for (unsigned i = 0; i < matrix->n_nonzero; i++)
    {
        row_ptr[matrix->row_indices[i]]++;
    }

    RowNNZ *tmp = malloc(matrix->n_rows * sizeof(RowNNZ));
    for (int i = 0; i < matrix->n_rows; i++)
    {
        tmp[i].row = i;
        tmp[i].nnz = row_ptr[i];
        // printf("Row %d has %d non-zeros\n", i, tmp[i].nnz);
    }

    qsort(tmp, matrix->n_rows, sizeof(RowNNZ), cmp_desc);

    for (int i = 0; i < matrix->n_rows; i++)
    {
        // printf("Permuted position %d gets row %d with %d non-zeros\n", i, tmp[i].row, tmp[i].nnz);
        matrix->perm[i] = tmp[i].row;
    }

    for (int i = 0; i < matrix->n_rows; i++)
    {
        // printf("Inverse perm position for row %d is %d\n", matrix->perm[i], i);
        matrix->inv_perm[matrix->perm[i]] = i;
    }
    free(row_ptr);
    free(tmp);
}
void row_perm(SpCOO *matrix)
{
    for (unsigned i = 0; i < matrix->n_nonzero; i++)
    {
        matrix->row_indices[i] = matrix->perm[matrix->row_indices[i]];
    }
}
void row_inv_perm(SpCOO *matrix)
{
    for (unsigned i = 0; i < matrix->n_nonzero; i++)
    {
        matrix->row_indices[i] = matrix->inv_perm[matrix->row_indices[i]];
    }
}

// void test_matrix()
// {
//     SpCOO s;
//     s.row_indices = (int *)malloc(NNZ * sizeof(int));
//     s.col_indices = (int *)malloc(NNZ * sizeof(int));
//     s.values = (double *)malloc(NNZ * sizeof(double));
//     s.perm = (int *)malloc(NNZ * sizeof(int));
//     s.inv_perm = (int *)malloc(NNZ * sizeof(int));
//     s.n_rows = 3;
//     s.n_cols = 3;

//     s.row_indices[0] = 0;
//     s.row_indices[1] = 1;
//     s.row_indices[2] = 1;
//     s.row_indices[3] = 2;

//     s.col_indices[0] = 0;
//     s.col_indices[1] = 2;
//     s.col_indices[2] = 1;
//     s.col_indices[3] = 2;

//     s.values[0] = 2.0;
//     s.values[1] = 3.0;
//     s.values[2] = 4.0;
//     s.values[3] = 5.0;

//     s.n_nonzero = NNZ;

//     print_matrix(&s);

//     //before permutation
//     build_perm_by_nnz(&s);
//     row_perm(&s);
//     coo_quicksort(&s, 0, s.n_nonzero);

//     print_matrix(&s);

//     convert_to_CSR(&s);
//     print_csr_matrix(&s);
    
//     convert_to_COO(&s);
    
//     print_matrix(&s);
//     row_perm(&s);
//     print_matrix(&s);
    
//     free(s.row_indices);
//     free(s.col_indices);
//     free(s.values);
// }
// void print_matrix(SpCOO *matrix)
// {
//     /* allocate dense buffer and initialize to zero */
//     double *dense = (double *)calloc((size_t)matrix->n_rows * matrix->n_cols, sizeof(double));
//     if (!dense)
//     {
//         fprintf(stderr, "Memory allocation failed\n");
//         return;
//     }

//     /* fill dense matrix with COO entries (if duplicates, values are added) */
//     for (int k = 0; k < matrix->n_nonzero; k++)
//     {
//         int r = matrix->row_indices[k], c = matrix->col_indices[k];
//         if (r < 0 || r >= matrix->n_rows || c < 0 || c >= matrix->n_cols)
//         {
//             fprintf(stderr, "COO entry %d has out-of-range index (%d,%d)\n", k, r, c);
//             continue;
//         }
//         dense[r * matrix->n_cols + c] += matrix->values[k];
//     }

//     /* print matrix */
//     printf("Dense matrix (%d x %d):\n", matrix->n_rows, matrix->n_cols);
//     for (int i = 0; i < matrix->n_rows; ++i)
//     {
//         for (int j = 0; j < matrix->n_cols; ++j)
//         {
//             printf("% .1g ", dense[i * matrix->n_cols + j]);
//         }
//         printf("\n");
//     }

//     free(dense);
// }
// void print_csr_matrix(SpCOO *matrix)
// {
//     printf("CSR Pointers:\n");
//     for (int i = 0; i < matrix->n_rows; i++)
//     {
//         printf("%d %d %lg\n", matrix->row_indices[i],matrix->col_indices[i],matrix->values[i]);
//     }
// }
// int main()
// {
//     test_matrix();
//     return 0;
// }