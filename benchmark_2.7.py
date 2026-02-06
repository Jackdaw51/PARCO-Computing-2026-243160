#!/usr/bin/env python
from __future__ import print_function
import subprocess
import glob
import os
import math
import sys

SOURCE_FILE = "main.c"
EXECUTABLE = "./spmv"
MTX_FOLDER = "mtxs"
OUTPUT_CSV = "benchmark_results.csv"
REPEATS = 10  
RANKS_LIST = [1, 2, 4, 8, 16, 32, 64, 128]

# py2 and py3
def to_str(bytes_or_str):
    if sys.version_info[0] >= 3:
        # Python 3: Decode bytes to string
        if isinstance(bytes_or_str, bytes):
            return bytes_or_str.decode('utf-8', errors='ignore')
    # Python 2: It is already a string (str is bytes)
    return bytes_or_str

def compile_code():
    print("--> Compiling {}...".format(SOURCE_FILE))
    cmd = ["mpicc", "-O3", "-march=native", SOURCE_FILE, "helpers.c", "deliverable_1/mmio.c", "-o", "spmv"]
    
    try:
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p.communicate()
        
        if p.returncode != 0:
            print("Error: Compilation failed!")
            print(to_str(err))
            sys.exit(1)
        print("--> Compilation successful.\n")
    except OSError as e:
        print("Error: Could not find 'mpicc'. Did you run 'module load mpi'?")
        print("System error: {}".format(e))
        sys.exit(1)

def calculate_mean(data):
    if not data: return 0.0
    return sum(data) / float(len(data))

def calculate_stdev(data):
    if len(data) < 2: return 0.0
    avg = calculate_mean(data)
    variance = sum([pow(x - avg, 2) for x in data]) / float(len(data) - 1)
    return math.sqrt(variance)

def parse_mpi_output(stdout_raw):
    stdout_str = to_str(stdout_raw)
    
    for line in stdout_str.split('\n'):
        if line.startswith("csv,"):
            # Line format: csv, ranks, time, gflops, min_nnz, max_nnz, comm, bw
            parts = line.split(',')
            return {
                "time": float(parts[2]),
                "gflops": float(parts[3]),
                "comm_vol": int(parts[6]),
                "bw": float(parts[7])
            }
    return None

def run_experiment(ranks, arg, description):
    times = []
    gflops_list = []
    comm_list = []
    
    sys.stdout.write("Benchmarking {} (np={}): ".format(description, ranks))
    sys.stdout.flush()
    
    for i in range(REPEATS):
        cmd = ["mpirun", "--allow-run-as-root", "-np", str(ranks), EXECUTABLE, arg]
        
        try:
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = p.communicate()
            
            if p.returncode == 0:
                data = parse_mpi_output(stdout)
                if data:
                    times.append(data['time'])
                    gflops_list.append(data['gflops'])
                    comm_list.append(data['comm_vol'])
                    sys.stdout.write(".") # Success
                else:
                    sys.stdout.write("x") # parsing error
            else:
                sys.stdout.write("E") # MPI Error
        except Exception as e:
            sys.stdout.write("!") # Python execution error
        
        sys.stdout.flush()

    sys.stdout.write(" Done.\n")
    
    if not times:
        return None

    return {
        "avg_time": calculate_mean(times),
        "stdev_time": calculate_stdev(times),
        "avg_gflops": calculate_mean(gflops_list),
        "avg_comm": calculate_mean(comm_list)
    }

if __name__ == "__main__":
    
    compile_code()

    with open(OUTPUT_CSV, "w") as f:
        f.write("Dataset,Type,Ranks,AvgTime_ms,StdevTime_ms,AvgGFLOPS,AvgCommBytes\n")

    # STRONG SCALING
    matrix_files = glob.glob(os.path.join(MTX_FOLDER, "*.mtx"))
    
    if not matrix_files:
        print("Warning: No .mtx files found in {}/".format(MTX_FOLDER))
    
    print("=== Starting Strong Scaling ({} files) ===".format(len(matrix_files)))
    
    for matrix_path in matrix_files:
        matrix_name = os.path.basename(matrix_path)
        
        for r in RANKS_LIST:
            stats = run_experiment(r, matrix_path, matrix_name)
            
            if stats:
                with open(OUTPUT_CSV, "a") as f:
                    f.write("{},Strong,{},{:.6f},{:.6f},{:.2f},{:.0f}\n".format(
                        matrix_name, r, stats['avg_time'], stats['stdev_time'], 
                        stats['avg_gflops'], stats['avg_comm']))

    # WEAK SCALING
    print("\n=== Starting Weak Scaling (Synthetic) ===")
    
    for r in RANKS_LIST:
        stats = run_experiment(r, "WEAK", "Synthetic")
        
        if stats:
            with open(OUTPUT_CSV, "a") as f:
                 f.write("Synthetic,Weak,{},{:.6f},{:.6f},{:.2f},{:.0f}\n".format(
                     r, stats['avg_time'], stats['stdev_time'], 
                     stats['avg_gflops'], stats['avg_comm']))

    print("\nResults saved to {}".format(OUTPUT_CSV))