#include "helpers.h"

extern int world_size, rank;
extern GhostComm ghost;
extern FILE *f;
MM_typecode matcode;

void generate_synthetic_matrix(SpCOO *matrix, int world_size)
{
    // Weak scaling: Size grows with processors.
    int nnz_per_row = 100; // keeps work per processor constant as we scale even though the matrix density is not constant
    matrix->n_rows = 10000 * world_size;
    matrix->n_cols = 10000 * world_size;
    matrix->n_nonzero = (int)matrix->n_rows * nnz_per_row;
    matrix->row_indices = (int *)malloc(matrix->n_nonzero * sizeof(int));
    matrix->col_indices = (int *)malloc(matrix->n_nonzero * sizeof(int));
    matrix->values = (double *)malloc(matrix->n_nonzero * sizeof(double));
    for (int i = 0; i < matrix->n_rows; i++)
    {
        for (int j = 0; j < nnz_per_row; j++)
        {
            int idx = i * nnz_per_row + j;
            matrix->row_indices[idx] = i;
            matrix->col_indices[idx] = (rand() % matrix->n_cols); // random column index
            matrix->values[idx] = (double)(rand() % 100) / 10.0;  // random value
        }
    }
}

void verify_result(SpCOO *matrix, double *input_vector, double *mpi_result, int rank)
{
    if (rank != 0)
        return;

    printf("\n--- Verifying Results (Sequential vs Distributed) ---\n");

    double *y_serial = (double *)calloc(matrix->n_rows, sizeof(double));
    if (!y_serial)
    {
        printf("Verification failed: Memory allocation error.\n");
        return;
    }

    for (int i = 0; i < matrix->n_nonzero; i++)
    {
        int r = matrix->row_indices[i];
        int c = matrix->col_indices[i];
        double val = matrix->values[i];

        y_serial[r] += val * input_vector[c];
    }

    int errors = 0;
    double max_diff = 0.0;
    double epsilon = 1e-9; // Tolerance

    for (int i = 0; i < matrix->n_rows; i++)
    {
        double diff = y_serial[i] - mpi_result[i];
        if (diff < 0)
            diff = -diff;

        if (diff > max_diff)
            max_diff = diff;

        if (diff > epsilon)
        {
            errors++;
            if (errors <= 5)
            {
                printf("Mismatch at Row %d: Serial=%.6f, MPI=%.6f, Diff=%e\n",
                       i, y_serial[i], mpi_result[i], diff);
            }
        }
    }

    if (errors == 0)
        printf("SUCCESS: Results match! (Max diff: %e)\n", max_diff);
    else
        printf("FAILURE: Found %d mismatches out of %d rows.\n", errors, matrix->n_rows);
    free(y_serial);
}

void scatter_initial_vector(double **my_vector, int my_vec_count, int base, int rem, double *global_vector)
{
    *my_vector = (double *)malloc(my_vec_count * sizeof(double));

    int *v_cnts = (int *)malloc(world_size * sizeof(int));
    int *v_displs = (int *)malloc(world_size * sizeof(int));
    int sum = 0;
    for (int i = 0; i < world_size; i++)
    {
        v_cnts[i] = base + (i < rem ? 1 : 0);
        v_displs[i] = sum;
        sum += v_cnts[i];
    }
    MPI_Scatterv(global_vector, v_cnts, v_displs, MPI_DOUBLE, *my_vector, my_vec_count, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    free(v_cnts);
    free(v_displs);
}
void randomly_fill_vector(double **vec, int size)
{
    *vec = (double *)malloc(size * sizeof(double));

    for (int i = 0; i < size; i++)
    {
        (*vec)[i] = (double)(rand() % 100) / 10.0;
    }
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
void prepare_ghost_communication(int local_nnz, int *rows, int *cols, int my_vec_start, int my_vec_end, int total_cols)
{
    int *needed_flags = (int *)calloc(total_cols, sizeof(int));
    int my_count = my_vec_end - my_vec_start;

    // find ghosts
    for (int i = 0; i < local_nnz; i++)
    {
        if (cols[i] < my_vec_start || cols[i] >= my_vec_end)
        {
            needed_flags[cols[i]] = 1;
        }
    }

    // count
    ghost.recv_counts = (int *)calloc(world_size, sizeof(int));
    ghost.recv_displs = (int *)calloc(world_size, sizeof(int));
    ghost.send_counts = (int *)calloc(world_size, sizeof(int));
    ghost.send_displs = (int *)calloc(world_size, sizeof(int));

    int base = total_cols / world_size;
    int rem = total_cols % world_size;
    int **requested_indices = (int **)calloc(world_size, sizeof(int *));

    int ghost_counter = 0;
    for (int r = 0; r < world_size; r++)
    {
        if (r == rank)
            continue;

        int r_start = r * base + (r < rem ? r : rem);
        int r_count = base + (r < rem ? 1 : 0);
        int r_end = r_start + r_count;

        int count = 0;
        for (int c = r_start; c < r_end; c++)
            if (needed_flags[c])
                count++;

        ghost.recv_counts[r] = count;
        requested_indices[r] = (int *)malloc(count * sizeof(int));

        int idx = 0;
        for (int c = r_start; c < r_end; c++)
        {
            if (needed_flags[c])
            {
                requested_indices[r][idx++] = c;
                // global col index -> combined vector index
                needed_flags[c] = my_count + ghost_counter;
                ghost_counter++;
            }
        }
    }

    //  map
    for (int i = 0; i < local_nnz; i++)
    {
        if (cols[i] < my_vec_start || cols[i] >= my_vec_end)
            cols[i] = needed_flags[cols[i]]; // Map to Ghost Area
        else
            cols[i] = cols[i] - my_vec_start; // Map to Local Area
    }
    free(needed_flags);

    ghost.num_ghosts = ghost_counter;

    // Exchange coutns
    MPI_Alltoall(ghost.recv_counts, 1, MPI_INT, ghost.send_counts, 1, MPI_INT, MPI_COMM_WORLD);

    // Exchange Lists
    int *s_disp = (int *)calloc(world_size, sizeof(int));
    int *r_disp = (int *)calloc(world_size, sizeof(int));
    int total_req_send = 0;
    int total_req_recv = 0;

    for (int i = 0; i < world_size; i++)
    {
        s_disp[i] = total_req_send;
        total_req_send += ghost.recv_counts[i];

        r_disp[i] = total_req_recv;
        total_req_recv += ghost.send_counts[i];

        if (i > 0)
        {
            ghost.recv_displs[i] = ghost.recv_displs[i - 1] + ghost.recv_counts[i - 1];
            ghost.send_displs[i] = ghost.send_displs[i - 1] + ghost.send_counts[i - 1];
        }
    }
    ghost.total_send = total_req_recv;
    ghost.items_to_send = (int *)malloc(total_req_recv * sizeof(int));
    ghost.send_buffer = (double *)malloc(total_req_recv * sizeof(double));

    int *flat_requests = (int *)malloc(total_req_send * sizeof(int));
    int p = 0;
    for (int r = 0; r < world_size; r++)
    {
        for (int k = 0; k < ghost.recv_counts[r]; k++)
            flat_requests[p++] = requested_indices[r][k];
        free(requested_indices[r]);
    }
    free(requested_indices);

    MPI_Alltoallv(flat_requests, ghost.recv_counts, s_disp, MPI_INT,
                  ghost.items_to_send, ghost.send_counts, r_disp, MPI_INT, MPI_COMM_WORLD);

    // requested global indices -> local indices
    for (int i = 0; i < total_req_recv; i++)
        ghost.items_to_send[i] -= my_vec_start;

    free(flat_requests);
    free(s_disp);
    free(r_disp);
}

void coo2csr(int n_local_rows, int n_nnz, int *coo_rows, int *coo_cols, double *coo_vals, SpCSR *csr, int world_size)
{
    csr->n_local_rows = n_local_rows;
    csr->n_nnz = n_nnz;
    csr->row_ptr = (int *)calloc(n_local_rows + 1, sizeof(int));
    csr->cols = (int *)malloc(n_nnz * sizeof(int));
    csr->vals = (double *)malloc(n_nnz * sizeof(double));

    for (int i = 0; i < n_nnz; i++)
    {
        int local_r = coo_rows[i] / world_size;
        csr->row_ptr[local_r + 1]++;
    }
    for (int i = 0; i < n_local_rows; i++)
    {
        csr->row_ptr[i + 1] += csr->row_ptr[i];
    }
    for (int i = 0; i < n_nnz; i++)
    {
        csr->cols[i] = coo_cols[i];
        csr->vals[i] = coo_vals[i];
    }
}

int setup(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (argc < 1)
        return -1;
    if (argc != 2)
    {
        if (rank == 0)
            printf("Usage: %s <matrix_name>\n", argv[0]);
        MPI_Finalize();
        return -1;
    }
    if (rank == 0 && strcmp(argv[1], "WEAK") != 0 && (f = fopen(argv[1], "r")) == NULL)
    {
        printf("Could not open file.\n");
        MPI_Finalize();
        return -1;
    }
    return 0;
}

int process_file(SpCOO *matrix)
{
    if (mm_read_banner(f, &matcode) != 0)
        return -1;
    if (mm_read_mtx_crd_size(f, &matrix->n_rows, &matrix->n_cols, &matrix->n_nonzero) != 0)
        return -1;
    return 0;
}

void matrix_initialize(SpCOO *matrix)
{
    if (process_file(matrix) == -1)
        exit(-1);
    matrix->row_indices = (int *)malloc(matrix->n_nonzero * sizeof(int));
    matrix->col_indices = (int *)malloc(matrix->n_nonzero * sizeof(int));
    matrix->values = (double *)malloc(matrix->n_nonzero * sizeof(double));
    for (int i = 0; i < matrix->n_nonzero; i++)
    {
        fscanf(f, "%d %d %lg\n", &matrix->row_indices[i], &matrix->col_indices[i], &matrix->values[i]);
        matrix->row_indices[i]--;
        matrix->col_indices[i]--;
    }
    fclose(f);
    coo_quicksort(matrix, 0, matrix->n_nonzero - 1);
}

void owner_and_count_initialize(SpCOO *matrix, int **send_counts, int **displacement)
{
    *send_counts = (int *)calloc(world_size, sizeof(int));
    *displacement = (int *)calloc(world_size, sizeof(int));
    int *cnts = *send_counts;
    int *displs = *displacement;
    for (int i = 0; i < matrix->n_nonzero; i++)
        cnts[matrix->row_indices[i] % world_size]++;
    displs[0] = 0;
    for (int i = 1; i < world_size; i++)
        displs[i] = displs[i - 1] + cnts[i - 1];
}

void data_reorder(SpCOO *matrix, int *displacement, int **ordered_rows, int **ordered_cols, double **ordered_values)
{
    *ordered_rows = (int *)malloc(matrix->n_nonzero * sizeof(int));
    *ordered_cols = (int *)malloc(matrix->n_nonzero * sizeof(int));
    *ordered_values = (double *)malloc(matrix->n_nonzero * sizeof(double));
    int *current_position = (int *)calloc(world_size, sizeof(int));
    for (int i = 0; i < matrix->n_nonzero; i++)
    {
        int owner = matrix->row_indices[i] % world_size;
        int pos = displacement[owner] + current_position[owner];
        (*ordered_rows)[pos] = matrix->row_indices[i];
        (*ordered_cols)[pos] = matrix->col_indices[i];
        (*ordered_values)[pos] = matrix->values[i];
        current_position[owner]++;
    }
    free(current_position);
}