#include <stdio.h>
#include <stdlib.h>
#include "helpers.h"

double compute_avg_time(double times[], unsigned n)
{
    double sum = 0.0;
    for (unsigned i = n / 4; i < n; i++)
    {
        sum += times[i];
    }
    return sum / (n - n / 4);
}

double *generate_random_vector(unsigned size)
{
    double *vector = (double *)malloc(size * sizeof(double));
    if (!vector)
        return NULL;
    for (unsigned i = 0; i < size; i++)
    {
        vector[i] = (double)rand() / RAND_MAX;
    }
    return vector;
}

void *multiply_CSR_SpCoord(double *result, SpCOO *csr_matrix, double *vector)
{
#if defined(FOR) && THREAD_NUMBER != 1
    printf("Parallel region started with threads: %d\n", omp_get_max_threads());
#pragma omp parallel for
#endif
#if defined(STATIC) && CHUNKSIZE
#pragma omp parallel for schedule(static, CHUNKSIZE)
#endif
#if defined(DYNAMIC) && CHUNKSIZE
#pragma omp parallel for schedule(dynamic, CHUNKSIZE)
#endif
#if defined(GUIDED) && CHUNKSIZE
#pragma omp parallel for schedule(guided, CHUNKSIZE)
#endif
    for (unsigned row = 0; row < csr_matrix->n_rows - 1; row++)
    {
        for (unsigned idx = csr_matrix->row_indices[row]; idx < csr_matrix->row_indices[row + 1]; idx++)
        {
            unsigned col = csr_matrix->col_indices[idx];
            double val = csr_matrix->values[idx];
            result[row] += val * vector[col];
        }
    }
}

void *convert_to_CSR(SpCOO *matrix)
{
    int *row_ptr = (int *)calloc(matrix->n_rows + 1, sizeof(int));
    if (!row_ptr)
        return NULL;

    // Count non-zeros per row
    for (unsigned i = 0; i < matrix->n_nonzero; i++)
    {
        row_ptr[matrix->row_indices[i] + 1]++;
    }

    // Sums to get the offset values
    for (int i = 0; i < matrix->n_rows; i++)
    {
        row_ptr[i + 1] += row_ptr[i];
    }

    free(matrix->row_indices);
    matrix->n_rows = matrix->n_rows + 1;
    matrix->row_indices = row_ptr;
    // Now row_ptr[i] is the index in COO arrays where row i starts
}
// Convert CSR back to COO
void *convert_to_COO(SpCOO *matrix)
{
    int *row_indices = (int *)malloc(matrix->n_nonzero * sizeof(int));
    if (!row_indices)
        return NULL;

    for (int i = 0; i < matrix->n_rows - 1; i++)
    {
        for (int j = matrix->row_indices[i]; j < matrix->row_indices[i + 1]; j++)
        {
            row_indices[j] = i;
        }
    }

    free(matrix->row_indices);
    matrix->n_rows = matrix->n_rows - 1;
    matrix->row_indices = row_indices;
}
int coo_less(SpCOO *p, unsigned a, unsigned b)
{
    unsigned ra = p->row_indices[a], rb = p->row_indices[b];
    if (ra < rb)
        return 1;
    if (ra > rb)
        return 0;
    return p->col_indices[a] < p->col_indices[b];
}

void swap(SpCOO *p, unsigned a, unsigned b)
{
    unsigned i, j;
    double x;
    i = p->row_indices[a];
    j = p->col_indices[a];
    x = p->values[a];
    p->row_indices[a] = p->row_indices[b];
    p->col_indices[a] = p->col_indices[b];
    p->values[a] = p->values[b];
    p->row_indices[b] = i;
    p->col_indices[b] = j;
    p->values[b] = x;
}

/* Lexicographic quicksort by (row, col) */
void coo_quicksort(SpCOO *p, unsigned base, unsigned n)
{
    unsigned lo, hi, left, right, mid;

    if (n == 0)
        return;
    lo = base;
    hi = lo + n - 1;
    while (lo < hi)
    {
        mid = lo + ((hi - lo) >> 1);

        if (coo_less(p, mid, lo))
            swap(p, mid, lo);
        if (coo_less(p, hi, mid))
        {
            swap(p, mid, hi);
            if (coo_less(p, mid, lo))
                swap(p, mid, lo);
        }
        left = lo + 1;
        right = hi - 1;
        do
        {
            while (coo_less(p, left, mid))
                left++;
            while (coo_less(p, mid, right))
                right--;
            if (left < right)
            {
                swap(p, left, right);
                if (mid == left)
                    mid = right;
                else if (mid == right)
                    mid = left;
                left++;
                right--;
            }
            else if (left == right)
            {
                left++;
                right--;
                break;
            }
        } while (left <= right);
        if (right - lo > hi - left)
        {
            coo_quicksort(p, left, hi - left + 1);
            hi = right;
        }
        else
        {
            coo_quicksort(p, lo, right - lo + 1);
            lo = left;
        }
    }
}