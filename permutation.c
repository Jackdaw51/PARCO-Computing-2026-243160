#include <stdlib.h>

typedef struct {
    int row;
    int nnz;
} RowNNZ;

int cmp_desc(const void *a, const void *b) {
    const RowNNZ *x = (const RowNNZ*)a;
    const RowNNZ *y = (const RowNNZ*)b;
    return (y->nnz - x->nnz);
}

void build_perm_by_nnz(const int *row_ptr, int n,
                       int *perm, int *inv_perm)
{
    RowNNZ *tmp = malloc(n * sizeof(RowNNZ));
    for (int i = 0; i < n; i++) {
        tmp[i].row = i;
        tmp[i].nnz = row_ptr[i+1] - row_ptr[i];
    }

    qsort(tmp, n, sizeof(RowNNZ), cmp_desc);

    for (int i = 0; i < n; i++)
        perm[i] = tmp[i].row;

    for (int i = 0; i < n; i++)
        inv_perm[perm[i]] = i;

    free(tmp);
}
void apply_row_perm_csr(const int *row_ptr, const int *col_ind,
                        const double *val,
                        int n, const int *perm,
                        int *row_ptr_out, int *col_ind_out,
                        double *val_out)
{
    row_ptr_out[0] = 0;
    int pos = 0;

    for (int new_i = 0; new_i < n; new_i++) {

        int old_i = perm[new_i];
        int start = row_ptr[old_i];
        int end   = row_ptr[old_i + 1];
        int len   = end - start;

        for (int k = 0; k < len; k++) {
            col_ind_out[pos] = col_ind[start + k];
            val_out[pos]     = val[start + k];
            pos++;
        }

        row_ptr_out[new_i + 1] = pos;
    }
}
