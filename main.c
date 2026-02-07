#include "helpers.h"
#ifndef CHECK_RESULTS
#define CHECK_RESULTS 0
#endif

char matrix_name[256];
int world_size, rank;
FILE *f;
SpCOO matrix;
GhostComm ghost;

int main(int argc, char *argv[])
{
    srand(time(NULL) + rank); 
    double start_time, end_time;
    double *global_vector = NULL;

    if (setup(argc, argv) == -1)
        return -1;

    int *send_counts = NULL, *displacement = NULL;
    int *ordered_rows = NULL, *ordered_cols = NULL;
    double *ordered_values = NULL;

    int is_weak_scaling = (strcmp(argv[1], "WEAK") == 0);

    // INPUT & SCATTER SETUP
    if (rank == 0)
    {
        if (is_weak_scaling)
        {
            printf("Weak scaling mode. Matrix size: %d\n", world_size);
            generate_synthetic_matrix(&matrix, world_size);
        }
        else
        {
            snprintf(matrix_name, sizeof(matrix_name), "%s", argv[1]);
            printf("Matrix name: %s\n", matrix_name);
            matrix_initialize(&matrix);
        }
        owner_and_count_initialize(&matrix, &send_counts, &displacement);

        randomly_fill_vector(&global_vector, matrix.n_cols);
        data_reorder(&matrix, displacement, &ordered_rows, &ordered_cols, &ordered_values);

        start_time = MPI_Wtime();
    }

    // SCATTER MATRIX DATA

    int local_nnz = 0;
    MPI_Scatter(send_counts, 1, MPI_INT, &local_nnz, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int *local_rows = (int *)malloc(local_nnz * sizeof(int));
    int *local_cols = (int *)malloc(local_nnz * sizeof(int));
    double *local_vals = (double *)malloc(local_nnz * sizeof(double));

    MPI_Scatterv(ordered_rows, send_counts, displacement, MPI_INT, local_rows, local_nnz, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(ordered_cols, send_counts, displacement, MPI_INT, local_cols, local_nnz, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(ordered_values, send_counts, displacement, MPI_DOUBLE, local_vals, local_nnz, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    MPI_Bcast(&matrix.n_rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&matrix.n_cols, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // SCATTER VECTOR DATA (Initial x vector)

    int base = matrix.n_cols / world_size;
    int rem = matrix.n_cols % world_size;
    int my_vec_count = base + (rank < rem ? 1 : 0);
    int my_vec_start = rank * base + (rank < rem ? rank : rem);

    double *my_vector = NULL;

    scatter_initial_vector(&my_vector, my_vec_count, base, rem, global_vector);

    // GHOST SETUP & RENUMBERING

    prepare_ghost_communication(local_nnz, local_rows, local_cols, my_vec_start, my_vec_start + my_vec_count, matrix.n_cols);

    int *int_rows = malloc(local_nnz * sizeof(int));
    int *int_cols = malloc(local_nnz * sizeof(int));
    double *int_vals = malloc(local_nnz * sizeof(double));
    int int_nnz = 0;

    int *ext_rows = malloc(local_nnz * sizeof(int));
    int *ext_cols = malloc(local_nnz * sizeof(int));
    double *ext_vals = malloc(local_nnz * sizeof(double));
    int ext_nnz = 0;

    // Filter data into internal and external
    for (int i = 0; i < local_nnz; i++)
    {
        if (local_cols[i] < my_vec_count)
        {
            int_rows[int_nnz] = local_rows[i];
            int_cols[int_nnz] = local_cols[i];
            int_vals[int_nnz] = local_vals[i];
            int_nnz++;
        }
        else
        {
            ext_rows[ext_nnz] = local_rows[i];
            ext_cols[ext_nnz] = local_cols[i];
            ext_vals[ext_nnz] = local_vals[i];
            ext_nnz++;
        }
    }

    SpCSR csr_internal, csr_external;
    int max_local_rows = (matrix.n_rows / world_size) + 1;

    coo2csr(max_local_rows, int_nnz, int_rows, int_cols, int_vals, &csr_internal, world_size);
    coo2csr(max_local_rows, ext_nnz, ext_rows, ext_cols, ext_vals, &csr_external, world_size);

    // PREPARE BUFFERS
    double *combined_vector = (double *)malloc((my_vec_count + ghost.num_ghosts) * sizeof(double));

    if (combined_vector == NULL)
    {
        printf("Rank %d Malloc failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    memcpy(combined_vector, my_vector, my_vec_count * sizeof(double));

    // OVERLAPPED EXECUTION LOOP
    MPI_Request *reqs = (MPI_Request *)malloc(2 * world_size * sizeof(MPI_Request));
    int n_reqs = 0;

    // start communication
    for (int i = 0; i < ghost.total_send; i++)
        ghost.send_buffer[i] = my_vector[ghost.items_to_send[i]];

    for (int i = 0; i < world_size; i++)
    {
        if (ghost.recv_counts[i] > 0)
        {
            MPI_Irecv(&combined_vector[my_vec_count + ghost.recv_displs[i]],
                      ghost.recv_counts[i], MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &reqs[n_reqs++]);
        }
    }
    for (int i = 0; i < world_size; i++)
    {
        if (ghost.send_counts[i] > 0)
        {
            MPI_Isend(&ghost.send_buffer[ghost.send_displs[i]],
                      ghost.send_counts[i], MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &reqs[n_reqs++]);
        }
    }

    // compute internal while communication is ongoing
    double *y_local = (double *)calloc(max_local_rows, sizeof(double));
    for (int i = 0; i < csr_internal.n_local_rows; i++)
    {
        double sum = 0.0;
        for (int k = csr_internal.row_ptr[i]; k < csr_internal.row_ptr[i + 1]; k++)
        {
            sum += csr_internal.vals[k] * combined_vector[csr_internal.cols[k]];
        }
        y_local[i] = sum;
    }

    // wait for communication to complete
    MPI_Waitall(n_reqs, reqs, MPI_STATUSES_IGNORE);

    // compute external
    for (int i = 0; i < csr_external.n_local_rows; i++)
    {
        double sum = 0.0;
        for (int k = csr_external.row_ptr[i]; k < csr_external.row_ptr[i + 1]; k++)
        {
            sum += csr_external.vals[k] * combined_vector[csr_external.cols[k]];
        }
        y_local[i] += sum;
    }
    // GATHER RESULTS

    double *y_packed = NULL;
    int *recv_cnts = NULL, *displs = NULL;

    if (rank == 0)
    {
        y_packed = (double *)malloc(world_size * max_local_rows * sizeof(double));
        recv_cnts = (int *)malloc(world_size * sizeof(int));
        displs = (int *)malloc(world_size * sizeof(int));
    }

    MPI_Gather(&max_local_rows, 1, MPI_INT, recv_cnts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        displs[0] = 0;
        for (int i = 1; i < world_size; i++)
            displs[i] = displs[i - 1] + recv_cnts[i - 1];
    }

    MPI_Gatherv(y_local, max_local_rows, MPI_DOUBLE, y_packed, recv_cnts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        double *y_final = (double *)calloc(matrix.n_rows, sizeof(double));
        int *cur_off = (int *)calloc(world_size, sizeof(int));

        // Unpack the gathered results into the correct final order
        for (int i = 0; i < matrix.n_rows; i++)
        {
            int owner = i % world_size;
            int pos = displs[owner] + cur_off[owner];
            y_final[i] = y_packed[pos];
            cur_off[owner]++;
        }

        end_time = MPI_Wtime();

#if defined(CHECK_RESULTS) && CHECK_RESULTS == 1
        verify_result(&matrix, global_vector, y_final, rank);
#endif

        free(y_final);
        free(y_packed);
        free(recv_cnts);
        free(displs);
        free(cur_off);
        free(matrix.row_indices);
        free(matrix.col_indices);
        free(matrix.values);
        free(ordered_rows);
        free(ordered_cols);
        free(ordered_values);
        free(send_counts);
        free(displacement);
        free(global_vector);
    }
    // PERFORMANCE METRICS
    long local_nnz_long = local_nnz;
    long min_nnz, max_nnz, sum_nnz;

    // 1. Load Balance (NNZ)
    MPI_Reduce(&local_nnz_long, &min_nnz, 1, MPI_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_nnz_long, &max_nnz, 1, MPI_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_nnz_long, &sum_nnz, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    // 2. Communication Volume (Bytes)
    // 8 bytes per double. Ghost data received + Sent.
    long local_comm_vol = (ghost.num_ghosts + ghost.total_send) * 8;
    long total_comm_vol;
    MPI_Reduce(&local_comm_vol, &total_comm_vol, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        double time_sec = end_time - start_time;
        double gflops = (2.0 * sum_nnz) / (time_sec * 1e9);
        double bw_gbs = (total_comm_vol / (1024.0 * 1024.0 * 1024.0)) / time_sec;

        printf("csv, %d, %f, %f, %ld, %ld, %ld, %f\n",
               world_size, time_sec * 1000, gflops, min_nnz, max_nnz, total_comm_vol, bw_gbs);

        // printf("Ranks: %d\n", world_size);
        // printf("Time: %f s\n", time_sec);
        // printf("GFLOPs: %f\n", gflops);
        // printf("Load Balance (NNZ): Min=%ld, Max=%ld (Imbalance: %.2f%%)\n",
        //        min_nnz, max_nnz, ((double)max_nnz / min_nnz - 1.0) * 100);
        // printf("Total Comm Volume: %.2f GB\n", total_comm_vol / (1024.0 * 1024.0 * 1024.0));
    }

    free(y_local);
    free(my_vector);
    free(combined_vector);
    free(reqs);

    free(local_rows);
    free(local_cols);
    free(local_vals);
    free(int_rows);
    free(int_cols);
    free(int_vals);
    free(ext_rows);
    free(ext_cols);
    free(ext_vals);
    if (csr_internal.row_ptr)
        free(csr_internal.row_ptr);
    if (csr_internal.cols)
        free(csr_internal.cols);
    if (csr_internal.vals)
        free(csr_internal.vals);

    if (csr_external.row_ptr)
        free(csr_external.row_ptr);
    if (csr_external.cols)
        free(csr_external.cols);
    if (csr_external.vals)
        free(csr_external.vals);

    // Cleanup Ghost Structs
    free(ghost.recv_counts);
    free(ghost.recv_displs);
    free(ghost.send_counts);
    free(ghost.send_displs);
    free(ghost.items_to_send);
    free(ghost.send_buffer);
    if (ghost.ghost_vector)
        free(ghost.ghost_vector);

    MPI_Finalize();
    return 0;
}
