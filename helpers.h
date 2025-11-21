#ifndef HELPERS_H
#define HELPERS_H
typedef struct
{
    unsigned n_rows;
    unsigned n_cols;
    unsigned n_nonzero;
    unsigned *row_indices;
    unsigned *col_indices;
    double *values;
    unsigned *perm;
    unsigned *inv_perm;
} SpCOO;

/* utility */
double compute_avg_time(double times[], unsigned n);
double *generate_random_vector(unsigned size);

/* sparse ops: signatures match implementations in helpers.c */
void *multiply_CSR_SpCoord(double *result, SpCOO *csr_matrix, double *vector);
void *convert_to_CSR(SpCOO *matrix);
void *convert_to_COO(SpCOO *matrix);

int coo_less(SpCOO *p, unsigned a, unsigned b);
void swap(SpCOO *p, unsigned a, unsigned b);
void coo_quicksort(SpCOO *p, unsigned base, unsigned n);

#endif /* HELPERS_H */
