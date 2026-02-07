#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "deliverable_1/mmio.h" 
#include <time.h>
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
} SpCOO;
typedef struct
{
    double *vals;
    int *cols;
    int *row_ptr;
    int n_local_rows;
    int n_nnz;
} SpCSR;
typedef struct
{
    int num_ghosts;       
    double *ghost_vector; 

    int *send_counts;
    int *send_displs;    
    int *items_to_send;  
    double *send_buffer; 
    int total_send;      

    int *recv_counts; 
    int *recv_displs; 
} GhostComm;

void coo_quicksort(SpCOO *p, unsigned base, unsigned n);
int setup(int argc, char *argv[]);
void matrix_initialize(SpCOO *matrix);
void owner_and_count_initialize(SpCOO *matrix, int **send_counts, int **displacement);
void data_reorder(SpCOO *matrix, int *displacement, int **ordered_rows, int **ordered_cols, double **ordered_values);
void prepare_ghost_communication(int local_nnz, int *rows, int *cols, int my_vec_start, int my_vec_end, int total_cols);
void coo2csr(int n_local_rows, int n_nnz, int *coo_rows, int *coo_cols, double *coo_vals, SpCSR *csr, int world_size);
void randomly_fill_vector(double **vec, int size);
void scatter_initial_vector(double **my_vector, int my_vec_count, int base, int rem, double *global_vector);
void verify_result(SpCOO *matrix, double *input_vector, double *mpi_result, int rank);
void generate_synthetic_matrix(SpCOO *matrix, int world_size);
#endif
