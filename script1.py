#Create an array of static_1 files of increasing thread number

import os
import re
import matplotlib.pyplot as plt
import numpy as np

thread_numbers = [1, 8, 16, 32, 64, 96]
#Get row size by looking to filenames in output_my_impl folder
files = os.listdir("output_my_impl")
row_size = [f for f in files if f.startswith("1_")]


#add to a set the second number after the first underscore
row_size = set(int(f.split("_")[1].split(".")[0]) for f in row_size)

row_size = list(row_size)
row_size.sort()
#if doesn't exist create pics/mtx folders
for mtx in row_size:
    dir_path = f"pics_my_impl/{mtx}"
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)
print(row_size)
array_of_times = []
for mtx in row_size:
    times = []
    for tn in thread_numbers:
        filename = f"output_my_impl/{tn}_{mtx}.bgn"
        try:
            with open(filename, "r") as f:
                content = f.read()
        except FileNotFoundError:
            print(f"Warning: {filename} not found; inserting NaN")
            times.append(np.nan)
            continue

        # extract floating number (e.g. "123.45 ms")
        m = re.search(r'([-+]?\d*\.\d+|\d+)', content)
        if m:
            time_ms = float(m.group(0))
            times.append(time_ms)
        else:
            print(f"Warning: No time found in {filename}; inserting NaN")
            times.append(np.nan)

        print(mtx)
        print(thread_numbers)
        print(times)
    plt.scatter(thread_numbers, times, s=100)
    plt.xlabel("Number of Threads")
    plt.ylabel("Time (ms)")
    plt.title("My implementation")
    plt.xticks(thread_numbers)
    plt.grid()
    plt.savefig(f"pics_my_impl/{mtx}/my_impl_times.png", dpi=150, bbox_inches='tight')
    plt.clf()  # Clear figure for next type
